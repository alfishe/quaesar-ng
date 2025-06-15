#include "qd/imGui/imGuiManager.h"

#include "qd/stl/algorithm.h"
#include "SDL.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "qd/imGui/backends/imgui_impl_sdl2.h"
#include "qd/imGui/backends/imgui_impl_sdlrenderer2.h"



// DECLRARE MODULE
QD_MODULE_REGISTRATION(qd::ImGuiManager);


// const char* qd::ImGuiBase::g_pTriboolTxt = "false\0true\0unset";


namespace qd {


QImGuiContext* ImGuiManager::createContextImGui(SDL_Window* window, SDL_Renderer* renderer)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ::ImGuiContext* pImCon = ImGui::CreateContext(/*m_pSharedFontAtlas*/);

    QImGuiContext* pContext = new QImGuiContext(pImCon, this);
    pContext->m_windowId = SDL_GetWindowID(window);
    pContext->m_pRenderer = renderer;

    m_pImContexts.push_back(pContext);

    pContext->useCurrent();
    pContext->_CreateFontsTexture(window);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);


    return pContext;
}


void ImGuiManager::destroyImContext(QImGuiContext* pGuiContext)
{
    auto It = qd::find(m_pImContexts.begin(), m_pImContexts.end(), pGuiContext);
    if (It != m_pImContexts.end())
    {
        pGuiContext->m_pParentModule = nullptr;
        ImGui::DestroyContext(pGuiContext->m_pImGuiContext);
        pGuiContext->m_pImGuiContext = nullptr;
        m_pImContexts.erase(It);
    }
}



void ImGuiManager::onModuleStartup(qd::ModuleCreateParams* mc) {}



void QImGuiContext::_CreateFontsTexture(SDL_Window* pGfx)
{
    assert(pGfx);

    ImGuiIO& io = useCurrent();

    if (!m_pParentModule->m_pSharedFontAtlas)
    {
        m_pParentModule->m_pSharedFontAtlas = io.Fonts;
        assert(m_pParentModule->m_pSharedFontAtlas);
    }

}



void QImGuiContext::onSdlEventProc(SDL_Event& event)
{
    useCurrent();
    ImGui_ImplSDL2_ProcessEvent(&event);
}


void QImGuiContext::newFrame()
{
    useCurrent();
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}


void QImGuiContext::endFrame()
{
    ImGui::EndFrame();
}


ImGuiIO& QImGuiContext::useCurrent() const
{
    assert(m_pImGuiContext);
    ImGui::SetCurrentContext(m_pImGuiContext);
    return getIO();
}



ImGuiIO& QImGuiContext::getIO() const
{
    assert(m_pImGuiContext);
    return m_pImGuiContext->IO;
}



void QImGuiContext::fillImGuiIO(SDL_Window* pWindow, Fixed32 Delta, Fixed32 Time)
{
    // ImGui::SetCurrentContext(pContext);
    useCurrent();

    ImGuiIO& io = ImGui::GetIO();

#if 0
	const Geometry::CPoint& wndSize = pWindow->getWindowSize();
	//assert(wndSize.x > 0 && wndSize.y > 0);
	io.DisplaySize.x = (float)wndSize.x;
	io.DisplaySize.y = (float)wndSize.y;

	io.DeltaTime = Delta.ToFloat(); // (float)pGfx->getApp()->GetSessTimeDeltaD();

	Input::CKeyboard* pKeyboard = pWindow->getKeyboard();

	// Read keyboard modifiers inputs
	io.KeyCtrl = pKeyboard->getRealKeyState(Input::EKeys::K_CONTROL); //(::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = pKeyboard->getRealKeyState(Input::EKeys::K_SHIFT_L); //(::GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = pKeyboard->getRealKeyState(Input::EKeys::K_ALT); //(::GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;

	// HERE COMES ONLY ARROWS AND TAB/DEL KEYS
	for ( Input::EKeys i = 0; i < Input::EKeys::__COUNT__; ++ i ) {
		bool bKeyState = pKeyboard->getRealKeyState(i);
		io.KeysDown[i] = bKeyState;
	}

	// ALL TEXT and LETTERS COME VIA INPUT_CHARS
	Input::CInputChars* pInputChars = pWindow->GetInputChars();
	for ( const qd::EAscii& curChar : *pInputChars ) {
		io.AddInputCharacterUTF16((ImWchar)curChar);
	}

	Input::CMouse* pMouse = pWindow->getMouse();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	Geometry::CPoint mousePos;
	pMouse->getRealPosition(mousePos);
	io.MousePos = ImVec2((float)mousePos.x, (float)mousePos.y);

	io.MouseDown[0] = pMouse->GetButtonState(Input::EMouseBtn::LEFT);
	io.MouseDown[1] = pMouse->GetButtonState(Input::EMouseBtn::RIGHT);
	io.MouseDown[2] = pMouse->GetButtonState(Input::EMouseBtn::WHEEL);

	// CALC MOUSE WHEEL
	if ( m_LastMouseZ == NO_INT_VALUE ) {
		m_LastMouseZ = pMouse->getPositionZ();
	}
	io.MouseWheel = (float)(pMouse->getPositionZ() - m_LastMouseZ) * 0.01f;
	m_LastMouseZ = pMouse->getPositionZ();
#endif //
}



void QImGuiContext::render(qd::Color clear_color)
{
    QImGuiContext* pContext = this;
    pContext->useCurrent();

    ImGuiIO& io = getIO();
    ImGui::Render();
    SDL_RenderSetScale(m_pRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    if (clear_color.a)
    {
        SDL_SetRenderDrawColor(m_pRenderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        SDL_RenderClear(m_pRenderer);
    }

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_pRenderer);
    SDL_RenderPresent(m_pRenderer);

}



void QImGuiContext::destroy()
{
    if (m_pParentModule)
    {
        m_pParentModule->destroyImContext(this);
        m_pParentModule = nullptr;
    }
}


void CImGuiDrawListRender::destroy()
{
    ImFontAtlas* pFonts = ImGui::GetIO().Fonts;
#if (0)
    if (pFonts && pFonts->TexID == m_pImFontTexture)
    {
        pFonts->TexID = nullptr;
    }
    if (m_pGfx && m_pImFontTexture)
    {
        m_pGfx->getTextureManager()->removeTexture(m_pImFontTexture);
    }
#endif // (0)
}


}; // namespace qd
//////////////////////////////////////////////////////////////////////////
