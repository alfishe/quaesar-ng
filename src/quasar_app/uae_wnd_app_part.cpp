// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on

#include "uae_wnd_app_part.h"
#include <SDL.h>
#include <amDebugger/commonOperations.h>
#include "amdbg_uae/uae_server_thread.h"
#include "cli11/CLI11.hpp"
#include "parse_options.h"
#include "qd/app/appMessages.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/qimGui/controls/qimMenu.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "quaesar.h"
#include "ui/uae_options_wnd.h"
#include "ui/uae_wnd_desktop.h"


UaeWndAppPart::UaeWndAppPart() {
}


UaeWndAppPart::~UaeWndAppPart() {
}


void UaeWndAppPart::onPartCreate(AppPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    m_pUaeServer = new UaeServerThread();
    m_pUaeServer->initialize();

    setPartActive(true);
    setPartVisisble(true);

    _createUaeWindow();

    // independent ImGui draw context
    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pImGui = pImGuiMgr->createContextImGui(m_pWindow, m_pUaeRenderer);
    m_pImGui->getIO().IniFilename = "";

    // UAE's root ui-window
    qd::UiNodeCreator mk;
    m_pDesktop = mk.make_<UaeWndDesktop>();
    m_pDesktop->setup();
}


void UaeWndAppPart::_createUaeWindow() {
    int wndWidth = m_pUaeServer->m_scrWidth;
    int wndHeight = m_pUaeServer->m_scrHeight;

    // Create a window
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;
    m_pWindow =
        SDL_CreateWindow("Quaesar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, wndWidth, wndHeight, window_flags);
    if (!m_pWindow) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return;
    }

    m_pUaeRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!m_pUaeRenderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(m_pWindow);
        return;
    }

    m_pUaeScrTexture =
        SDL_CreateTexture(m_pUaeRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, wndWidth, wndHeight);
    if (!m_pUaeScrTexture) {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        SDL_DestroyRenderer(m_pUaeRenderer);
        SDL_DestroyWindow(m_pWindow);
        return;
    }
}

void UaeWndAppPart::update(float Delta, float Time) {
    _drawGuiMenus();
}


void UaeWndAppPart::_drawGuiMenus() {
    if (!m_bShowImgui)
        return;
    m_pImGui->newFrame();
    m_pDesktop->draw();
    m_pImGui->endFrame();
}


void UaeWndAppPart::render() {
    // render UAE texture screen
    int curFrame = m_pUaeServer->getScrFrameNo();
    if (curFrame == m_renderedFrameNo) {
        return;
    }
    m_renderedFrameNo = curFrame;

    int new_width = 0;
    int new_height = 0;
    int window_width, window_height;
    SDL_GetWindowSize(m_pWindow, &window_width, &window_height);

    if (m_pUaeServer->m_UaeScrTextureMutex.tryLock()) {
        int uaeWidth = m_pUaeServer->m_scrWidth;
        int uaeHeight = m_pUaeServer->m_scrHeight;
        if (!uaeWidth || !uaeHeight)
            return;

        // Maintain aspect ratio
        float image_aspect = (float)uaeWidth / (float)uaeHeight;
        float window_aspect = (float)window_width / (float)window_height;

        if (window_aspect < image_aspect) {
            new_width = window_width;
            new_height = (int)(window_width / image_aspect);
        } else {
            new_height = window_height;
            new_width = (int)(window_height * image_aspect);
        }
        SDL_Rect rect = {(window_width - new_width) / 2, (window_height - new_height) / 2, new_width, new_height};
        SDL_RenderClear(m_pUaeRenderer);

        tryRecreateEmuScreenTexture(uaeWidth, uaeHeight);  // Recreate texture if needed
        uint32_t* texture_pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(m_pUaeScrTexture, NULL, (void**)&texture_pixels, &pitch) == 0) {
            for (int y = 0; y < uaeHeight; y++) {
                uint8_t* dest = (uint8_t*)&texture_pixels[y * uaeWidth];
                memcpy(dest, &m_pUaeServer->m_pAmigaBuffer[y * uaeWidth], uaeWidth * 4);
            }
            SDL_UnlockTexture(m_pUaeScrTexture);
        }

        SDL_RenderCopy(m_pUaeRenderer, m_pUaeScrTexture, NULL, &rect);
        m_pUaeServer->m_UaeScrTextureMutex.unlock();
    }

    if (m_bShowImgui)
        m_pImGui->render();

    SDL_RenderPresent(m_pUaeRenderer);
}


// Function to recreate a dynamic texture with new dimensions
void UaeWndAppPart::tryRecreateEmuScreenTexture(int newWidth, int newHeight) {
    // Get the format of the old texture
    int access, currentWidth, currentHeight;
    Uint32 format;
    if (SDL_QueryTexture(m_pUaeScrTexture, &format, &access, &currentWidth, &currentHeight) != 0)
        return;
    if (newWidth == currentWidth && newHeight == currentHeight) {
        return;
    }
    // Destroy the old texture
    SDL_DestroyTexture(m_pUaeScrTexture);
    // Create a new texture with the desired dimensions
    m_pUaeScrTexture = SDL_CreateTexture(m_pUaeRenderer, format,
                                         access,  // Using the same access pattern as the original
                                         newWidth, newHeight);
}


void UaeWndAppPart::destroyImp() {
    destroyUaeWindow();

    if (m_pDesktop) {
        m_pDesktop->destroy();
        delete m_pDesktop;
        m_pDesktop = nullptr;
    }
}


void UaeWndAppPart::bringWndToFront() {
    if (m_pWindow)
        SDL_RaiseWindow(m_pWindow);
}


qd::EFlow UaeWndAppPart::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) {
    switch (in_msg.id) {
        case qd::appMsg::OnAppRequestToQuit::CID: {
            ::quit_program = UAE_QUIT;
            ::currprefs.cpu_cycle_exact = 0;
            break;
        }
        default:
            break;
    }
    return qd::EFlow::NO_RESULT;
}


qd::EFlow UaeWndAppPart::onSdlEventProc(SDL_Event& event) {
    uint32_t uaeWndId = SDL_GetWindowID(m_pWindow);
    switch (event.type) {
        case SDL_KEYDOWN: {
            if (event.key.windowID != uaeWndId)
                return qd::EFlow::CONTINUE;
            if (event.key.keysym.sym == SDLK_F12) {
                setShowImgui(!m_bShowImgui);
                return qd::EFlow::STOP;
            }
            m_pUaeServer->pushSdlEvent(event);
            return qd::EFlow::STOP;
        } break;
        case SDL_KEYUP: {
            if (event.key.keysym.sym == SDLK_F12)
                return qd::EFlow::STOP;
            m_pUaeServer->pushSdlEvent(event);
        } break;
        default:
            break;
    }
    if (m_bShowImgui)
        return m_pImGui->onSdlEventProc(event);

    return qd::EFlow::CONTINUE;
}


void UaeWndAppPart::destroyUaeWindow() {
    m_pUaeServer->destroy();
    SAFE_DELETE(m_pUaeServer);

    SDL_DestroyTexture(m_pUaeScrTexture);
    m_pUaeScrTexture = nullptr;
    SDL_DestroyRenderer(m_pUaeRenderer);
    m_pUaeRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}
