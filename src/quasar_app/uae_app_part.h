#include "qd/Thread/thread.h"
#include "qd/app/appPart.h"
#include "ui/uae_wnd_desktop.h"


struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;
class UaeWndDesktop;
FORWARD_DECLARATION_2(qd, QImGuiContext);


// AppPart that represents UAE-emulator window in main-thread
//
class UaeAppPart : public qd::AppPart {
    TS_BEGIN_REFLECT_CLASS(UaeAppPart, qd::AppPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Emulator"));
    TS_END();

private:
    class UaeWorker* m_pUaeWorker = nullptr;
    UaeWndDesktop* m_pDesktop = nullptr;
    int m_renderedFrameNo = -1;
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pUaeRenderer = nullptr;
    SDL_Texture* m_pUaeScrTexture = nullptr;
    qd::QImGuiContext* m_pImGui = nullptr;
    bool m_bShowImgui = false;

public:
    virtual void onPartCreate(AppPart::OnCreate_t& prm) override;

    void createUaeWindow();
    virtual void update(float dt, float time) override;

    void updGuiMenus();
    virtual void render() override;

    virtual void destroyImp() override;

    SDL_Window* getSdlWindow() const {
        return m_pWindow;
    }

    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) override;
    virtual qd::EFlow onSdlEventProc(SDL_Event& event) override;

    bool getShowImgui() const {
        return m_bShowImgui;
    }
    void setShowImgui(bool ShowImgui) {
        m_bShowImgui = ShowImgui;
    }

private:
    void tryRecreateEmuScreenTexture(int newWidth, int newHeight);
    void destroyUaeWindow();


};  // class UaeAppPart
//////////////////////////////////////////////////////////////////////////
