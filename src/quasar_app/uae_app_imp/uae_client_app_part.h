#include "qd/app/appPart.h"
//#include "uae_vm_imp.h"
#include "ui/uae_wnd_desktop.h"


struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;
FORWARD_DECLARATION_1(UaeServerThread);
FORWARD_DECLARATION_2(qd, QImGuiContext);
FORWARD_DECLARATION_2(qsr, UaeGuiDesktop);
FORWARD_DECLARATION_2(AbsVM, VM);
FORWARD_DECLARATION_4(amD, vm, imp, UaeVmImp);


namespace qsr {

// AppPart that represents UAE-emulator window in main-thread
//
class UaeClientAppPart : public qd::AppPart, public qd::IOperationEnvironment {
    TS_BEGIN_REFLECT_CLASS(UaeClientAppPart, qd::AppPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Client"));
    TS_END();

private:
    qsr::UaeGuiDesktop* m_pUaeWndGui = nullptr;
    int m_renderedFrameNo = -1;
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pUaeRenderer = nullptr;
    SDL_Texture* m_pUaeScrTexture = nullptr;
    qd::QImGuiContext* m_pImGui = nullptr;
    bool m_bShowImgui = false;
    ref_ptr<AbsVM::VM> m_pVm;

public:
    UaeClientAppPart(AbsVM::VM* _vm);
    virtual ~UaeClientAppPart();

    virtual void onPartCreate(qd::AppPart::OnCreate_t& prm) override;

    void _createUaeWindow();
    virtual void update(float dt, float time) override;
    virtual void render() override;

    virtual void destroyImp() override;

    SDL_Window* getSdlWindow() const {
        return m_pWindow;
    }
    void bringWndToFront();

    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) override;
    virtual qd::EFlow onSdlEventProc(SDL_Event& event) override;

    bool getShowImgui() const {
        return m_bShowImgui;
    }
    void setShowImgui(bool ShowImgui) {
        m_bShowImgui = ShowImgui;
    }

    UaeServerThread* getUaeThread() const;

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationMsg(qd::operation::args::Base* args) override;
    AbsVM::VM* getVm() const;

private:
    void _drawGuiMenus();
    void tryRecreateEmuScreenTexture(int newWidth, int newHeight);
    void destroyUaeWindow();

};  // class UaeClientAppPart
//////////////////////////////////////////////////////////////////////////


};  // namespace qsr
