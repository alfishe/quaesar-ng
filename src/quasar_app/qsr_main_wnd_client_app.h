#include "amDebugger/config.h"
#include "qd/app/applicationPart.h"
#include "qsr_app_interfaces.h"
#include "qsr_application.h"
#include "ui/uae_wnd_desktop.h"
#include "vm_player_selector.h"


struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;
FORWARD_DECLARATION_1(UaeServerThread);
FORWARD_DECLARATION_2(qd, QImGuiContext);
FORWARD_DECLARATION_2(qsr, QsrVmClientPlayerGuiDesktop);
FORWARD_DECLARATION_2(IVm, VM);
FORWARD_DECLARATION_4(amD, vm, imp, UaeVmImp);

namespace qsr {

struct CfgQsrMain : public CfgBase {
    CFG_DECLARE(qsr::CfgQsrMain);
    bool quitByEsc = false;

    int mainWndSizeX = 754;
    int mainWndSizeY = 576;

    EngineId engine = EngineId::WinUae;  // default emulation engine
};
inline static CfgQsrMain& g_cfg_vm_wnd = CfgQsrMain::get();


//------------------------------------------------------------------------
// ApplicationPart that represents VM player window with UIDesktop
//
class QsrMainClientWndApp : public qd::ApplicationPart, public qsr::IVmOperationsHandler {
    TS_BEGIN_REFLECT_CLASS(QsrMainClientWndApp, qd::ApplicationPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("Main Quaesar VM player window"));
    TS_END();

private:
    qsr::IVmClientPlayer* m_pVmClientPlayer = nullptr;
    qsr::QsrVmClientPlayerGuiDesktop* m_pDesktop = nullptr;
    bool m_bShowGui = false;
    uint32_t m_renderedFrameNo = ~0u;
    SDL_Rect m_lastDstRect = {0, 0, 0, 0};  // Cached dst rect for re-drawing last frame when no new emulator frame exists
    int m_lastTexW = 0;
    int m_lastTexH = 0;
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_hWndRenderer = nullptr;
    SDL_Texture* m_hVmDisplayTx = nullptr;
    Uint32 m_displayFormat = SDL_PIXELFORMAT_ARGB8888;  // updated from IVmClientPlayer::getDisplayPixelFormat()
    qd::QImGuiContext* m_pQimGuiCtx = nullptr;
    VmPlayersSelector m_vmSelector;
    int m_nCurVmPlayterId = -1;

    // Temporal blending buffer for scandoubler flicker reduction
    uint32_t* m_pPrevFrameBuf = nullptr;
    int m_prevBufWidth = 0;
    int m_prevBufHeight = 0;

public:
    QsrMainClientWndApp(qsr::IVmClientPlayer* pVmProvider = nullptr);
    virtual ~QsrMainClientWndApp() override;

    void init();

    void _createMainOsWindow();
    virtual void updateAppPart(float dt, float time) override;
    virtual void renderAppPart() override;

    virtual void destroyImp() override;

    SDL_Window* getSdlWindow() const {
        return m_pWindow;
    }
    void bringWndToFront();

    virtual IVm::VM* getVm() const override;
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;
    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) override;
    virtual qd::EFlow onSdlEventProc(SDL_Event& event) override;

    bool getShowImgui() const {
        return m_bShowGui;
    }
    void setShowImgui(bool ShowImgui) {
        m_bShowGui = ShowImgui;
    }
    QuaesarApplication* getApp() const {
        QuaesarApplication* pApp = (QuaesarApplication*)TSuper::getApp();
        return pApp;
    }

    qsr::IVmClientPlayer* getVmProvider() const;
    void setVmPlayer(qsr::IVmClientPlayer* VmProvider);


    virtual void onPartCreate(qd::ApplicationPart::OnCreate_t& prm) override;

    qsr::VmPlayersSelector& getVmSelector() const {
        return const_cast<qsr::VmPlayersSelector&>(m_vmSelector);
    }

    int getCurVmPlayerId() const { return m_nCurVmPlayterId; }

private:
    void _drawGuiMenus();
    SDL_Texture* tryRecreateEmuScreenTexture(int newWidth, int newHeight);
    void destroyUaeWindow();

};  // class QsrMainClientWndApp
//////////////////////////////////////////////////////////////////////////


};  // namespace qsr
