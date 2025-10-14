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
#include "ui/uae_options_wnd.h"
#include "ui/uae_wnd_desktop.h"


namespace qsr {


QsrMainClientWndApp::QsrMainClientWndApp(qsr::IVmServerThread* pVmProvider) : m_pVmProvider(pVmProvider) {
}

QsrMainClientWndApp::~QsrMainClientWndApp() {
}

void QsrMainClientWndApp::onPartCreate(qd::ApplicationPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    _createMainOsWindow();
    setPartActive(true);
    setPartRenderable(true);

    // independent ImGui draw context for UAE window
    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pQimGuiCtx = pImGuiMgr->createContextImGui(m_pWindow, m_hWndRenderer);
    m_pQimGuiCtx->getIO().IniFilename = "";

    // UAE's root ui-window
    qd::UiNodeCreator mk;
    m_pUaeWndGui = mk.make_<qsr::QsrMainClientGuiDesktop>(this);
    m_pUaeWndGui->init();
}


void QsrMainClientWndApp::_createMainOsWindow() {
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

void QsrMainClientWndApp::update(float /*dt*/, float /*time*/) {
    _drawGuiMenus();
}


void QsrMainClientWndApp::_drawGuiMenus() {
    if (!m_bShowImgui)
        return;
    m_pQimGuiCtx->newFrame();
    m_pUaeWndGui->draw();
    m_pQimGuiCtx->endFrame();
}


void QsrMainClientWndApp::render() {
    // render Display texture screen
    IVmServerThread* pVmThread = getVmProvider();
    uint32_t curFrame = pVmThread->getScrFrameNo();
    if (curFrame != m_renderedFrameNo) {
        m_renderedFrameNo = curFrame;

        int curWndSizeX, curWndSizeY;
        SDL_GetWindowSize(m_pWindow, &curWndSizeX, &curWndSizeY);

        int bufWidth, bufHeight;
        uint32_t* pSrcDisplayBuf = nullptr;
        if (pVmThread->lockDisplayTexBuf(&bufWidth, &bufHeight, &pSrcDisplayBuf)) {
            if (!bufWidth || !bufHeight)
                return;

            if (bufHeight < 350)
                bufHeight *= 2;

            // Maintain aspect ratio
            float image_aspect = (float)bufWidth / (float)bufHeight;
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

            SDL_Texture* hDisplayTex = tryRecreateEmuScreenTexture(bufWidth, bufHeight);  // Recreate texture if needed
            void* texture_pixels = nullptr;
            int pitch = 0;
            if (SDL_LockTexture(hDisplayTex, nullptr, (void**)&texture_pixels, &pitch) == 0) {
                for (int curY = 0; curY < bufHeight; curY++) {
                    uint8_t* dest = (uint8_t*)texture_pixels + (curY * pitch);
                    memcpy(dest, &pSrcDisplayBuf[curY / 2 * bufWidth], bufWidth * 4);
                }
                SDL_UnlockTexture(hDisplayTex);
            }

            SDL_RenderCopy(m_hWndRenderer, hDisplayTex, nullptr, &rect);
            pVmThread->unlockDisplayTexBuf();
        }
    }

    if (m_bShowImgui)
        m_pQimGuiCtx->render();

    SDL_RenderPresent(m_hWndRenderer);
}


// Function to recreate a dynamic texture with new dimensions
SDL_Texture* QsrMainClientWndApp::tryRecreateEmuScreenTexture(int newWidth, int newHeight) {
    // Get the format of the old texture
    int access, currentWidth, currentHeight;
    Uint32 format;
    if (SDL_QueryTexture(m_hDisplayTex, &format, &access, &currentWidth, &currentHeight) != 0)
        return m_hDisplayTex;
    if (newWidth == currentWidth && newHeight == currentHeight) {
        return m_hDisplayTex;
    }
    // Destroy the old texture
    SDL_DestroyTexture(m_hDisplayTex);
    // Create a new texture with the desired dimensions
    m_hDisplayTex = SDL_CreateTexture(m_hWndRenderer, format,
                                      access,  // Using the same access pattern as the original
                                      newWidth, newHeight);
    return m_hDisplayTex;
}


void QsrMainClientWndApp::destroyImp() {
    destroyUaeWindow();

    if (m_pUaeWndGui) {
        m_pUaeWndGui->destroy();
        //delete m_pUaeWndGui;
        m_pUaeWndGui = nullptr;
    }

    SAFE_DESTROY(m_pQimGuiCtx);
}


void QsrMainClientWndApp::bringWndToFront() {
    if (m_pWindow)
        SDL_RaiseWindow(m_pWindow);
}


qd::EFlow QsrMainClientWndApp::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) {
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


qd::EFlow QsrMainClientWndApp::onSdlEventProc(SDL_Event& event) {
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


IVmServerThread* QsrMainClientWndApp::getVmProvider() const {
    assert(m_pVmProvider);
    return m_pVmProvider;
}


void QsrMainClientWndApp::setVmProvider(qsr::IVmServerThread* VmProvider) {
    if (m_pVmProvider == VmProvider)
        return;
    m_pVmProvider = VmProvider;
}


qd::EFlow QsrMainClientWndApp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
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


IVm::VM* QsrMainClientWndApp::getVm() const {
    return m_pVmProvider->getVm();
}


void QsrMainClientWndApp::destroyUaeWindow() {
    SDL_DestroyTexture(m_hDisplayTex);
    m_hDisplayTex = nullptr;
    SDL_DestroyRenderer(m_hWndRenderer);
    m_hWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}


};  // namespace qsr
