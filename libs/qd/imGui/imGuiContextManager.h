#pragma once
#include "qd/app/moduleManager.h"
#include "qd/base/base.h"
#include "qd/base/color.h"
#include "qd/base/eFlow.h"
#include "qd/math/fixedPoint.h"
#include <imgui/imgui.h>

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;


namespace qd {

class ImGuiContextManager;


void ImGuiDrawForModule(qd::IModuleInterface* pBaseModInterface);



class CImGuiDrawListRender
{
public:
    SDL_Window* m_pGfx = nullptr;

public:
    CImGuiDrawListRender(SDL_Window* pGfx = nullptr)
        : m_pGfx(pGfx) {}

    void destroy();

}; // class CImGuiDrawListRender
   //////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// Wrapper under ImGuiContext
class QImGuiContext
{
    typedef QImGuiContext TThis;

public:
    ::ImGuiContext* m_pImGuiContext = nullptr; // real ImGui context
    SDL_Renderer* m_pRenderer = nullptr;
    ImGuiContextManager* m_pParentModule = nullptr;
    uint32_t m_windowId = 0;
    bool m_frameStarted = false;
    bool m_frameEnded = false;

    static constexpr int NO_INT_VALUE = 0x71234567;
    int m_LastMouseZ = NO_INT_VALUE;


public:
    QImGuiContext(::ImGuiContext* pImGuiContext, qd::ImGuiContextManager* pImGuiMod)
        : m_pImGuiContext(pImGuiContext)
        , m_pParentModule(pImGuiMod) {}

    void newFrame();
    void endFrame();

    ::ImGuiContext* useCurrent() const;
    void setImGuiContext(ImGuiContext* pImGuiContext);
    ::ImGuiIO& getIO() const;

    qd::ImGuiContextManager* getImGuiMod() const { return m_pParentModule; }

    qd::EFlow onSdlEventProc(SDL_Event& event); // handle SDL event as IO Inputs

    void render(qd::Color clear_color = qd::Color(0u));
    void skipFrame() { m_frameStarted = m_frameEnded = false; }
    void destroy();

}; // class QImGuiContext
//////////////////////////////////////////////////////////////////////////




// Module
class ImGuiContextManager : public qd::IModuleInterface
{
    TS_REFLECT_CLASS(qd::ImGuiContextManager, qd::IModuleInterface);
    friend class qd::ModuleManager;

public:
    qtd::vector<QImGuiContext*> m_pImContexts; // all available contexts
    ImFontAtlas* m_pSharedFontAtlas = nullptr;
    bool m_bIsInitialized = false;

public:
    ImGuiContextManager(const qd::ModuleCreateParams& mc);
    QImGuiContext* createContextImGui(SDL_Window* window, SDL_Renderer* renderer);

    bool isInitialized() const { return !m_pImContexts.empty(); }
    void destroyImContext(QImGuiContext* pQContext);
    virtual void destroyModule() override;
protected:
    virtual ~ImGuiContextManager() override;

}; // class ImGuiContextManager
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
//////////////////////////////////////////////////////////////////////////

