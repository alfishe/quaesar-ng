#pragma once
#include "qd/app/moduleManager.h"
#include "qd/base/base.h"
#include "qd/math/fixedPoint.h"
#include <imgui/imgui.h>
#include "qd/base/color.h"

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;


namespace qd {

class ImGuiManager;


void ImGuiDrawForModule(qd::IModuleInterface* pBaseModInterface);



class CImGuiDrawListRender
{
public:
    SDL_Window* m_pGfx = nullptr;

public:
    CImGuiDrawListRender(SDL_Window* pGfx = nullptr)
        : m_pGfx(pGfx)
    {}

    void destroy();

}; // class CImGuiDrawListRender
   //////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
class QImGuiContext
{
    typedef QImGuiContext TThis;

public:
    ::ImGuiContext* m_pImGuiContext = nullptr;
    SDL_Renderer* m_pRenderer = nullptr;
    ImGuiManager* m_pParentModule = nullptr;
    uint32_t m_windowId = 0;

    static constexpr int NO_INT_VALUE = 0x71234567;
    int m_LastMouseZ = NO_INT_VALUE;


public:
    QImGuiContext(::ImGuiContext* pImGuiContext, qd::ImGuiManager* pImGuiMod)
        : m_pImGuiContext(pImGuiContext)
        , m_pParentModule(pImGuiMod)
    {}

    void newFrame();
    void endFrame();

    ImGuiIO& useCurrent() const;

    ImGuiIO& getIO() const;

    void fillImGuiIO(SDL_Window* pWindow, Fixed32 Delta, Fixed32 Time);

    qd::ImGuiManager* getImGuiMod() const { return m_pParentModule; }

    void render(qd::Color clear_color = qd::Color(0u));

    void destroy();

    void _CreateFontsTexture(SDL_Window* pGfx);

    void onSdlEventProc(SDL_Event& event);

}; // class CImContext
//////////////////////////////////////////////////////////////////////////




// qd::Modules::ImGuiCG::ImGuiManager
class ImGuiManager : public qd::IModuleInterface
{
    TS_REFLECT_CLASS(qd::ImGuiManager, qd::IModuleInterface);
    friend class qd::ModuleManager;

public:
    // Graphics::CGraphics* m_pGfx;
    //QImGuiContext* m_pMainContext = nullptr; // root context
    qd::vector<QImGuiContext*> m_pImContexts; // all available contexts
    ImFontAtlas* m_pSharedFontAtlas = nullptr;
    bool m_bIsInitialized = false;

public:
    virtual void onModuleStartup(qd::ModuleCreateParams* mc) override;

    bool isInitialized() const { return !m_pImContexts.empty(); }
    //void InitializeImGuiIO(SDL_Window* pGfx);

    // void RenderImGuiDrawData(Graphics::CGraphics* pGfx, ImDrawData* draw_data = nullptr);


#if (0)
    static void Example_Render(Graphics::CGraphics* pGfx)
    {
        auto pImGuiMod = qd::CModuleManager::I()->getModuleInstOrCreate_<qd::Modules::ImGuiCG::ImGuiManager>();
        if (!pImGuiMod->isInitialized())
        {
            pImGuiMod->InitializeImGuiIO(pGfx);
            m_CgTimer.Start();
        }

        static qd::CFixed32 m_PrevTime;
        static qd::Modules::ImGuiCG::QImGuiContext* m_pImGuiContext = nullptr;

        if (!m_pImGuiContext)
            m_pImGuiContext = pImGuiMod->createContextImGui();

        qd::CFixed32 curTime = m_CgTimer.GetElapsedTimeFx32();
        qd::CFixed32 deltaTime = curTime - m_PrevTime;
        m_PrevTime = curTime;

        m_pImGuiContext->fillImGuiIO(m_MfcDriver.m_pCWindow, deltaTime, curTime);

        // imGui
        {
            ImGui::NewFrame();
            // SHOW DEMO VIEW EXAMPLE
            ImGui::ShowDemoWindow();

            // Rendering
            ImGui::EndFrame();
        }
        m_pImGuiContext->render(pGfx);
    }
#endif // (0)

    QImGuiContext* createContextImGui(SDL_Window* window, SDL_Renderer* renderer);

    void destroyImContext(QImGuiContext* pGuiContext);


    virtual ~ImGuiManager() {}

public:
    virtual void destroyModule() override
    {
        while (!m_pImContexts.empty())
        {
            destroyImContext(m_pImContexts.back());
        }
    }

}; // class CImGuiModule
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
//////////////////////////////////////////////////////////////////////////
