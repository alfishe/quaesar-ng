// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on

#include "uae_client_app_part.h"
#include <SDL.h>
#include <amDebugger/commonOperations.h>
#include "cli11/CLI11.hpp"
#include "parse_options.h"
#include "qd/app/appMessages.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/qimGui/controls/qimMenu.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "quaesar.h"
#include "quaesar_app.h"
#include "quaesar_operations.h"
#include "uae_app_imp/uae_server_thread.h"
#include "uae_app_imp/uae_vm_imp.h"
#include "uae_server_app_part.h"
#include "ui/uae_options_wnd.h"
#include "ui/uae_wnd_desktop.h"


namespace qsr {


UaeClientAppPart::UaeClientAppPart(IVm::VM* _vm) : m_pVm(_vm) {
}

UaeClientAppPart::~UaeClientAppPart() {
}

void UaeClientAppPart::onPartCreate(qd::AppPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    _createUaeWindow();
    setPartActive(true);
    setPartVisible(true);

    // independent ImGui draw context
    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pImGui = pImGuiMgr->createContextImGui(m_pWindow, m_pUaeRenderer);
    m_pImGui->getIO().IniFilename = "";

    // UAE's root ui-window
    qd::UiNodeCreator mk;
    m_pUaeWndGui = mk.make_<qsr::UaeGuiDesktop>(this);
    m_pUaeWndGui->init();
}


void UaeClientAppPart::_createUaeWindow() {
    int wndWidth, wndHeight;
    m_pVm->emu->getScreenSize(&wndWidth, &wndHeight);

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

void UaeClientAppPart::update(float Delta, float Time) {
    _drawGuiMenus();
}


void UaeClientAppPart::_drawGuiMenus() {
    if (!m_bShowImgui)
        return;
    m_pImGui->newFrame();
    m_pUaeWndGui->draw();
    m_pImGui->endFrame();
}


void UaeClientAppPart::render() {
    // render UAE texture screen
    UaeServerThread* pUae = getUaeThread();
    int curFrame = pUae->getScrFrameNo();
    if (curFrame != m_renderedFrameNo) {
        m_renderedFrameNo = curFrame;

        int new_width = 0;
        int new_height = 0;
        int window_width, window_height;
        SDL_GetWindowSize(m_pWindow, &window_width, &window_height);

        if (pUae->m_UaeScrTextureMutex.tryLock()) {
            int uaeWidth = pUae->m_scrWidth;
            int uaeHeight = pUae->m_scrHeight;
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
                    memcpy(dest, &pUae->m_pAmigaBuffer[y * uaeWidth], uaeWidth * 4);
                }
                SDL_UnlockTexture(m_pUaeScrTexture);
            }

            SDL_RenderCopy(m_pUaeRenderer, m_pUaeScrTexture, NULL, &rect);
            pUae->m_UaeScrTextureMutex.unlock();
        }
    }

    if (m_bShowImgui)
        m_pImGui->render();

    SDL_RenderPresent(m_pUaeRenderer);
}


// Function to recreate a dynamic texture with new dimensions
void UaeClientAppPart::tryRecreateEmuScreenTexture(int newWidth, int newHeight) {
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


void UaeClientAppPart::destroyImp() {
    destroyUaeWindow();

    if (m_pUaeWndGui) {
        m_pUaeWndGui->destroy();
        //delete m_pUaeWndGui;
        m_pUaeWndGui = nullptr;
    }
}


void UaeClientAppPart::bringWndToFront() {
    if (m_pWindow)
        SDL_RaiseWindow(m_pWindow);
}


qd::EFlow UaeClientAppPart::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) {
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


qd::EFlow UaeClientAppPart::onSdlEventProc(SDL_Event& event) {
    UaeServerThread* pUae = getUaeThread();
    uint32_t uaeWndId = SDL_GetWindowID(m_pWindow);
    switch (event.type) {
        case SDL_KEYDOWN: {
            if (event.key.windowID != uaeWndId)
                return qd::EFlow::CONTINUE;
            if (event.key.keysym.sym == SDLK_F12) {
                setShowImgui(!m_bShowImgui);
                return qd::EFlow::STOP;
            }
            pUae->pushSdlEvent(event);
            return qd::EFlow::STOP;
        } break;
        case SDL_KEYUP: {
            if (event.key.keysym.sym == SDLK_F12)
                return qd::EFlow::STOP;
            pUae->pushSdlEvent(event);
        } break;
        default:
            break;
    }
    if (m_bShowImgui)
        return m_pImGui->onSdlEventProc(event);

    return qd::EFlow::CONTINUE;
}


UaeServerThread* UaeClientAppPart::getUaeThread() const {
    return getApp()->m_pUaeServerAppPart->getUaeThread();
}


void* UaeClientAppPart::getOpEnvPtr(const qd::TypeInfo& classType) const {
    return nullptr;
}


qd::EFlow UaeClientAppPart::applyOperationMsgProc(qd::operation::args::Base* args) {
    if (auto p = args->cast_<qsr::operation::args::ShowDebuggerWnd>()) {
        amD::DebuggerApp* pDbg = getApp()->getDebuggerApp();
        pDbg->setWndVisible(true);
        return qd::EFlow::STOP;
    }
    return qd::EFlow::STOP;
}


IVm::VM* UaeClientAppPart::getVm() const {
    return m_pVm.get();
}


void UaeClientAppPart::destroyUaeWindow() {
    SDL_DestroyTexture(m_pUaeScrTexture);
    m_pUaeScrTexture = nullptr;
    SDL_DestroyRenderer(m_pUaeRenderer);
    m_pUaeRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}


};  // namespace qsr
