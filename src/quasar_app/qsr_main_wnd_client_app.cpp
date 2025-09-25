#include "qsr_main_wnd_client_app.h"
#include <SDL.h>
#include "qd/app/appMessages.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qsr_application.h"
#include "qsr_config.h"
#include "qsr_operations.h"
#include "quaesar.h"
#include "uae_imp/uae_server_app_part.h"
#include "uae_imp/uae_server_thread.h"
#include "uae_imp/uae_vm_imp.h"
#include "ui/uae_options_wnd.h"
#include "ui/uae_wnd_desktop.h"


namespace qsr {


UaeClientAppPart::UaeClientAppPart(qsr::IVmServerThread* pVmProvider) : m_pVmProvider(pVmProvider) {
}

UaeClientAppPart::~UaeClientAppPart() {
}

void UaeClientAppPart::onPartCreate(qd::ApplicationPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    _createMainOsWindow();
    setPartActive(true);
    setPartVisible(true);

    // independent ImGui draw context for UAE window
    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pQimGuiCtx = pImGuiMgr->createContextImGui(m_pWindow, m_hWndRenderer);
    m_pQimGuiCtx->getIO().IniFilename = "";

    // UAE's root ui-window
    qd::UiNodeCreator mk;
    m_pUaeWndGui = mk.make_<qsr::UaeClientGuiDesktop>(this);
    m_pUaeWndGui->init();
}


void UaeClientAppPart::_createMainOsWindow() {
    int wndWidth = g_cfg_main->mainWndSizeX;
    int wndHeight = g_cfg_main->mainWndSizeY;

    // Create a window
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;
    m_pWindow =
        SDL_CreateWindow("Quaesar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, wndWidth, wndHeight, window_flags);
    if (!m_pWindow) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return;
    }

    m_hWndRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!m_hWndRenderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(m_pWindow);
        return;
    }

    m_hDisplayTex =
        SDL_CreateTexture(m_hWndRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, wndWidth, wndHeight);
    if (!m_hDisplayTex) {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        SDL_DestroyRenderer(m_hWndRenderer);
        SDL_DestroyWindow(m_pWindow);
        return;
    }
}

void UaeClientAppPart::update(float /*dt*/, float /*time*/) {
    _drawGuiMenus();
}


void UaeClientAppPart::_drawGuiMenus() {
    if (!m_bShowImgui)
        return;
    m_pQimGuiCtx->newFrame();
    m_pUaeWndGui->draw();
    m_pQimGuiCtx->endFrame();
}


void UaeClientAppPart::render() {
    // render Display texture screen
    IVmServerThread* pVmThread = getVmProvider();
    uint32_t curFrame = pVmThread->getScrFrameNo();
    if (curFrame != m_renderedFrameNo) {
        m_renderedFrameNo = curFrame;

        int curWndSizeX, curWndSizeY;
        SDL_GetWindowSize(m_pWindow, &curWndSizeX, &curWndSizeY);

        int uaeWidth, uaeHeight;
        uint32_t* srcDisplayBuf;
        if (pVmThread->lockDisplayTexBuf(&uaeWidth, &uaeHeight, &srcDisplayBuf)) {
            if (!uaeWidth || !uaeHeight)
                return;

            // Maintain aspect ratio
            float image_aspect = (float)uaeWidth / (float)uaeHeight;
            float window_aspect = (float)curWndSizeX / (float)curWndSizeY;
            int new_width = 0, new_height = 0;

            if (window_aspect < image_aspect) {
                new_width = curWndSizeX;
                new_height = (int)(curWndSizeX / image_aspect);
            } else {
                new_height = curWndSizeY;
                new_width = (int)(curWndSizeY * image_aspect);
            }
            SDL_Rect rect = {(curWndSizeX - new_width) / 2, (curWndSizeY - new_height) / 2, new_width, new_height};
            SDL_RenderClear(m_hWndRenderer);

            tryRecreateEmuScreenTexture(uaeWidth, uaeHeight);  // Recreate texture if needed
            uint32_t* texture_pixels = nullptr;
            int pitch = 0;
            if (SDL_LockTexture(m_hDisplayTex, nullptr, (void**)&texture_pixels, &pitch) == 0) {
                for (int y = 0; y < uaeHeight; y++) {
                    uint8_t* dest = (uint8_t*)&texture_pixels[y * uaeWidth];
                    memcpy(dest, &srcDisplayBuf[y * uaeWidth], uaeWidth * 4);
                }
                SDL_UnlockTexture(m_hDisplayTex);
            }

            SDL_RenderCopy(m_hWndRenderer, m_hDisplayTex, nullptr, &rect);
            pVmThread->unlockDisplayTexBuf();
        }
    }

    if (m_bShowImgui)
        m_pQimGuiCtx->render();

    SDL_RenderPresent(m_hWndRenderer);
}


// Function to recreate a dynamic texture with new dimensions
void UaeClientAppPart::tryRecreateEmuScreenTexture(int newWidth, int newHeight) {
    // Get the format of the old texture
    int access, currentWidth, currentHeight;
    Uint32 format;
    if (SDL_QueryTexture(m_hDisplayTex, &format, &access, &currentWidth, &currentHeight) != 0)
        return;
    if (newWidth == currentWidth && newHeight == currentHeight) {
        return;
    }
    // Destroy the old texture
    SDL_DestroyTexture(m_hDisplayTex);
    // Create a new texture with the desired dimensions
    m_hDisplayTex = SDL_CreateTexture(m_hWndRenderer, format,
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

    SAFE_DESTROY(m_pQimGuiCtx);
}


void UaeClientAppPart::bringWndToFront() {
    if (m_pWindow)
        SDL_RaiseWindow(m_pWindow);
}


qd::EFlow UaeClientAppPart::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) {
    switch (in_msg.id) {
        case qd::appMsg::OnAppRequestToQuit::CID: {
            doOperation_<qsr::operations::QuitQuasarApp>();
            break;
        }
        default:
            break;
    }
    return qd::EFlow::NO_RESULT;
}


qd::EFlow UaeClientAppPart::onSdlEventProc(SDL_Event& event) {
    IVmServerThread* pVmProvider = getVmProvider();
    uint32_t uaeWndId = SDL_GetWindowID(m_pWindow);
    switch (event.type) {
        case SDL_KEYDOWN: {
            if (event.key.windowID != uaeWndId)
                return qd::EFlow::CONTINUE;

            SDL_Keysym sym = event.key.keysym;
            if (sym.sym == SDLK_F12) {
                if (sym.mod & KMOD_SHIFT) {
                    // Handle shift + F12
                    doOperation_<qsr::operations::ShowDebuggerWnd>();
                } else
                    setShowImgui(!m_bShowImgui);
                return qd::EFlow::STOP;
            } else if (sym.sym == SDLK_ESCAPE) {
                if (g_cfg_main->quitByEsc) {
                    getApp()->requestAppToQuit();
                    return qd::EFlow::STOP;
                }
            }
            pVmProvider->pushSdlEvent(event);
            return qd::EFlow::STOP;
        } break;

        case SDL_KEYUP: {
            if (event.key.keysym.sym == SDLK_F12)
                return qd::EFlow::STOP;
            pVmProvider->pushSdlEvent(event);
        } break;

        case SDL_WINDOWEVENT: {
            if (event.window.windowID != uaeWndId)
                return qd::EFlow::CONTINUE;
            uint8_t wndEvent = event.window.event;
            if (wndEvent == SDL_WINDOWEVENT_CLOSE) {
                getApp()->requestAppToQuit();
            }
            break;
        }

        default:
            break;
    }
    if (m_bShowImgui)
        return m_pQimGuiCtx->onSdlEventProc(event);

    return qd::EFlow::CONTINUE;
}


IVmServerThread* UaeClientAppPart::getVmProvider() const {
    assert(m_pVmProvider);
    return m_pVmProvider;
}


void UaeClientAppPart::setVmProvider(qsr::IVmServerThread* VmProvider) {
    if (m_pVmProvider == VmProvider)
        return;
    m_pVmProvider = VmProvider;
}


qd::EFlow UaeClientAppPart::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
    if (auto p = args->cast_<qsr::operations::ShowDebuggerWnd>()) {
        unused(p);
        amD::DebuggerApp* pDbg = getApp()->getDebuggerApp();
        pDbg->setWndVisible(true);
        return qd::EFlow::STOP;
    }
    // send operation to UAE thread
    if (IVmServerThread* pUaeThread = getVmProvider()) {
        qd::operation::BaseOpArgs* pClonedArgs = args->clone();
        pUaeThread->pushOperationMsg(qd::unique_ptr<qd::operation::BaseOpArgs>(pClonedArgs));
    }
    return qd::EFlow::STOP;
}


IVm::VM* UaeClientAppPart::getVm() const {
    return m_pVmProvider->getVm();
}


void UaeClientAppPart::destroyUaeWindow() {
    SDL_DestroyTexture(m_hDisplayTex);
    m_hDisplayTex = nullptr;
    SDL_DestroyRenderer(m_hWndRenderer);
    m_hWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}


};  // namespace qsr
