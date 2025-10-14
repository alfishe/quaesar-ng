#include "qd/app/applicationPart.h"
#include "qsr_app_interfaces.h"
#include "qsr_application.h"
#include "ui/uae_wnd_desktop.h"


struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;
FORWARD_DECLARATION_1(UaeServerThread);
FORWARD_DECLARATION_2(qd, QImGuiContext);
FORWARD_DECLARATION_2(qsr, QsrMainClientGuiDesktop);
FORWARD_DECLARATION_2(IVm, VM);
FORWARD_DECLARATION_4(amD, vm, imp, UaeVmImp);


namespace qsr {

// ApplicationPart that represents in main-thread
//
class QsrMainClientWndApp : public qd::ApplicationPart, public qsr::IOperationsVmEnvHandler {
    TS_BEGIN_REFLECT_CLASS(QsrMainClientWndApp, qd::ApplicationPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Client"));
    TS_END();

private:
    qsr::QsrMainClientGuiDesktop* m_pUaeWndGui = nullptr;
    uint32_t m_renderedFrameNo = ~0u;
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_hWndRenderer = nullptr;
    SDL_Texture* m_hDisplayTex = nullptr;
    qd::QImGuiContext* m_pQimGuiCtx = nullptr;
    qsr::IVmServerThread* m_pVmProvider = nullptr;
    bool m_bShowImgui = false;

public:
    QsrMainClientWndApp(qsr::IVmServerThread* pVmProvider = nullptr);
    virtual ~QsrMainClientWndApp() override;

    virtual void onPartCreate(qd::ApplicationPart::OnCreate_t& prm) override;

    void _createMainOsWindow();
    virtual void update(float dt, float time) override;
    virtual void render() override;

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
        return m_bShowImgui;
    }
    void setShowImgui(bool ShowImgui) {
        m_bShowImgui = ShowImgui;
    }
    QuaesarApplication* getApp() const {
        QuaesarApplication* pApp = (QuaesarApplication*)TSuper::getApp();
        return pApp;
    }

    qsr::IVmServerThread* getVmProvider() const;
    void setVmProvider(qsr::IVmServerThread* VmProvider);

private:
    void _drawGuiMenus();
    SDL_Texture* tryRecreateEmuScreenTexture(int newWidth, int newHeight);
    void destroyUaeWindow();

};  // class QsrMainClientWndApp
//////////////////////////////////////////////////////////////////////////


};  // namespace qsr
