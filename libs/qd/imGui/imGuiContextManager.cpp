#include "qd/imGui/imGuiContextManager.h"

#include "qd/stl/algorithm.h"
#include "SDL.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "qd/imGui/backends/imgui_impl_sdl2.h"
#include "qd/imGui/backends/imgui_impl_sdlrenderer2.h"



// DECLRARE MODULE
QD_MODULE_REGISTRATION(qd::ImGuiContextManager);


// const char* qd::ImGuiBase::g_pTriboolTxt = "false\0true\0unset";


namespace qd {


QImGuiContext* ImGuiContextManager::createContextImGui(SDL_Window* window, SDL_Renderer* renderer)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImFontAtlas* pSharedFontAtlas = m_pSharedFontAtlas;
    if (pSharedFontAtlas && !m_pImContexts.empty())
    {
        if (m_pImContexts[0]->m_pRenderer != renderer)
            pSharedFontAtlas = nullptr;
    }

    ::ImGuiContext* pImCon = ImGui::CreateContext(pSharedFontAtlas);

    QImGuiContext* pQContext = new QImGuiContext(pImCon, this);
    pQContext->m_windowId = SDL_GetWindowID(window);
    pQContext->m_pRenderer = renderer;

    m_pImContexts.push_back(pQContext);

    pQContext->useCurrent();
    ImGuiIO& io = pQContext->getIO();
    if (!m_pSharedFontAtlas)
        m_pSharedFontAtlas = io.Fonts;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    return pQContext;
}


void ImGuiContextManager::destroyImContext(QImGuiContext* pQContext)
{
    auto It = qd::find(m_pImContexts.begin(), m_pImContexts.end(), pQContext);
    if (It != m_pImContexts.end())
    {
        pQContext->m_pParentModule = nullptr;
        ImGuiContext* pPrevCtx = pQContext->useCurrent();
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(pQContext->m_pImGuiContext);
        pQContext->m_pImGuiContext = nullptr;
        m_pImContexts.erase(It);

        ImGui::SetCurrentContext(pPrevCtx);
    }
}



 ImGuiContextManager::~ImGuiContextManager()
{
    assert(m_pImContexts.empty());
}


void ImGuiContextManager::destroyModule()
{
    while (!m_pImContexts.empty())
    {
        destroyImContext(m_pImContexts.back());
    }
}




qd::EFlow QImGuiContext::onSdlEventProc(SDL_Event& event)
{
    ImGuiContext* pOldCtx = useCurrent();
    qd::EFlow r;
    r = ImGui_ImplSDL2_ProcessEvent(&event);
    setImGuiContext(pOldCtx);
    return r;
}


void QImGuiContext::newFrame()
{
    m_frameStarted = true;
    useCurrent();
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}


void QImGuiContext::endFrame()
{
    ImGui::EndFrame();
    m_frameEnded = true;
}


ImGuiContext* QImGuiContext::useCurrent() const
{
    assert(m_pImGuiContext);
    ImGuiContext* pOldContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_pImGuiContext);
    return pOldContext;
}



void QImGuiContext::setImGuiContext(ImGuiContext* pImGuiContext)
{
    ImGui::SetCurrentContext(pImGuiContext);
}


ImGuiIO& QImGuiContext::getIO() const
{
    assert(m_pImGuiContext);
    return m_pImGuiContext->IO;
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
}


}; // namespace qd
//////////////////////////////////////////////////////////////////////////
