#include "qd/Thread/thread.h"
#include "qd/app/appPart.h"
#include "ui/uae_wnd_desktop.h"


struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;
class UaeWndDesktop;
FORWARD_DECLARATION_2(qd, QImGuiContext);


class UaeAppPart : public qd::AppPartBase {
    TS_BEGIN_REFLECT_CLASS(UaeAppPart, qd::AppPartBase);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Emulator"));
    TS_END();

private:
    UaeWndDesktop* m_pDesktop = nullptr;
    int m_wndWidth = 754;
    int m_wndHeight = 576;
    int m_renderedFrameNo = -1;
    uint32_t* m_pAmigaBuffer = nullptr;
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pUaeRenderer = nullptr;
    SDL_atomic_t m_scrFrameNo = {};
    SDL_Texture* m_pUaeScrTexture = nullptr;
    qd::Mutex m_UaeScrTextureMutex;
    qd::QImGuiContext* m_pImGui = nullptr;
    bool m_bShowImgui = false;

public:
    virtual void onPartCreate(AppPartBase::OnCreate_t& prm) override;

    void createUaeWindow();
    virtual void update(float dt, float time) override;
    virtual void render() override;

    uint32_t* lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void unlockUaeScreenTexBuf();

    virtual void destroyImp() override;

    SDL_Window* getSdlWindow() const {
        return m_pWindow;
    }

    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) override;

    virtual void onSdlEventProc(SDL_Event& event) override;

    bool getShowImgui() const {
        return m_bShowImgui;
    }
    void setShowImgui(bool ShowImgui) {
        m_bShowImgui = ShowImgui;
    }

private:
    void recreateTexture(int newWidth, int newHeight);
    void destroyUaeWindow();


};  // class UaeAppPart
//////////////////////////////////////////////////////////////////////////
