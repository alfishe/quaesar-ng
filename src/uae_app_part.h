#include "qd/Thread/thread.h"
#include "qd/app/appPart.h"

struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;


class UaeAppPart : public qd::AppPartBase {
    TS_BEGIN_REFLECT_CLASS(UaeAppPart, qd::AppPartBase);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Emulator"));
    TS_END();

private:
    int m_wndWidth = 754;
    int m_wndHeight = 576;
    int renderedFrameNo = -1;
    uint32_t* m_pAmigaBuffer = nullptr;
    SDL_Window* mUaeWindow = nullptr;
    SDL_Renderer* m_pUaeRenderer = nullptr;
    SDL_atomic_t scrFrameNo = {};
    SDL_Texture* m_pUaeScrTexture = nullptr;
    qd::Mutex m_UaeScrTextureMutex;

public:
    void createUaeWindow();
    void renderUaeWindow();

    uint32_t* lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void unlockUaeScreenTexBuf();

    virtual void update(qd::Fixed32 Delta, qd::Fixed32 Time) override;
    virtual void destroyImp() override;


    SDL_Window* getSdlWindow() const {
        return mUaeWindow;
    }

    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) override;

private:
    void recreateTexture(int newWidth, int newHeight);
    void destroyUaeWindow();


};  // class UaeAppPart
//////////////////////////////////////////////////////////////////////////
