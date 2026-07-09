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


QsrMainClientWndApp::QsrMainClientWndApp(qsr::IVmClientPlayer* pVmProvider) : m_pVmClientPlayer(pVmProvider) {
}

QsrMainClientWndApp::~QsrMainClientWndApp() {
}


void QsrMainClientWndApp::init() {
    m_vmSelector.init();
    _createMainOsWindow();
    setPartActive(true);
    setPartRenderable(true);

    // independent ImGui draw context for UAE window
    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pQimGuiCtx = pImGuiMgr->createContextImGui(m_pWindow, m_hWndRenderer);
    m_pQimGuiCtx->getIO().IniFilename = "";

    // UAE's root ui-window
    qd::UiNodeCreator mk;
    m_pDesktop = mk.make_<qsr::QsrVmClientPlayerGuiDesktop>(this);
    m_pDesktop->init();

    m_nCurVmPlayterId = m_vmSelector.activateVmPlayerByIdStr(getApp(), engineIdToStr(g_cfg_vm_wnd.engine));
}


void QsrMainClientWndApp::onPartCreate(qd::ApplicationPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);
    init();
}


void QsrMainClientWndApp::_createMainOsWindow() {
    int wndWidth = g_cfg_vm_wnd.mainWndSizeX;
    int wndHeight = g_cfg_vm_wnd.mainWndSizeY;

    // Create a window
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;
    m_pWindow =
        SDL_CreateWindow("Quaesar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, wndWidth, wndHeight, window_flags);
    if (!m_pWindow) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return;
    }

    // VSYNC pacing for the entire application.
    //
    // This is the ONLY renderer in the application that requests
    // SDL_RENDERER_PRESENTVSYNC. The debugger window renderer (see
    // DebuggerApp::createRenderWindow in debuggerWndApp.cpp) intentionally
    // does NOT use vsync.
    //
    // The main loop calls updateAppPart + renderAppPart for BOTH windows
    // in a single iteration. If both renderers had vsync, each iteration
    // would block on two independent vsync waits (~33ms total at 60Hz),
    // cutting the effective frame rate in half. With vsync only here,
    // the loop blocks once (~16.6ms) and the debugger's RenderPresent
    // returns immediately.
    //
    // Without vsync, SDL_RenderPresent returns instantly and the main
    // loop would spin at 100% CPU re-rendering the same frame thousands
    // of times per second.
    m_hWndRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!m_hWndRenderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(m_pWindow);
        return;
    }

    m_hVmDisplayTx =
        SDL_CreateTexture(m_hWndRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, wndWidth, wndHeight);
    if (!m_hVmDisplayTx) {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        SDL_DestroyRenderer(m_hWndRenderer);
        SDL_DestroyWindow(m_pWindow);
        return;
    }
}

void QsrMainClientWndApp::updateAppPart(float /*dt*/, float /*time*/) {
    if (!m_pVmClientPlayer && m_nCurVmPlayterId >= 0) {
        m_pVmClientPlayer = m_vmSelector.getVmPlayer(m_nCurVmPlayterId);
    }
    _drawGuiMenus();
}


void QsrMainClientWndApp::_drawGuiMenus() {
    if (!m_bShowGui)
        return;
    m_pQimGuiCtx->newFrame();
    m_pDesktop->draw();
    m_pQimGuiCtx->endFrame();
}


void QsrMainClientWndApp::renderAppPart() {
    // render VM display texture screen
    IVmClientPlayer* pVmPlayer = getVmProvider();
    if (!pVmPlayer)
        return;

    uint32_t curFrame = pVmPlayer->getScrFrameNo();
    if (curFrame != m_renderedFrameNo)
    {
        // New emulator frame available — upload it to the texture.
        int bufWidth, bufHeight;
        uint32_t* pSrcDisplayBuf = nullptr;
        if (pVmPlayer->lockDisplayTexBuf(&bufWidth, &bufHeight, &pSrcDisplayBuf)) {
            m_renderedFrameNo = curFrame;
            if (bufWidth && bufHeight) {
                int curWndSizeX, curWndSizeY;
                SDL_GetWindowSize(m_pWindow, &curWndSizeX, &curWndSizeY);

                // Detect PAL low-res modes (e.g. 640x256) where the Amiga only
                // outputs half the vertical lines. Instead of CPU-side scanline
                // doubling (which doubled both the texture upload size and the
                // memcpy work), we upload only the original lines and let the
                // GPU scale vertically for free via SDL_RenderCopy's src/dst rects.
                bool isLowRes = (bufHeight < 350);
                int origHeight = bufHeight;
                int texHeight = isLowRes ? (bufHeight * 2) : bufHeight;

                // Maintain aspect ratio using the doubled height (display size)
                float image_aspect = (float)bufWidth / (float)texHeight;
                float window_aspect = (float)curWndSizeX / (float)curWndSizeY;
                int new_width = 0, new_height = 0;

                if (window_aspect < image_aspect) {
                    new_width = curWndSizeX;
                    new_height = (int)(curWndSizeX / image_aspect);
                } else {
                    new_height = curWndSizeY;
                    new_width = (int)(curWndSizeY * image_aspect);
                }
                m_lastDstRect = {(curWndSizeX - new_width) / 2, (curWndSizeY - new_height) / 2, new_width, new_height};
                m_lastTexW = bufWidth;
                m_lastTexH = origHeight;

                SDL_Texture* hDisplayTex =
                    tryRecreateEmuScreenTexture(bufWidth, origHeight);
                void* texture_pixels = nullptr;
                int pitch = 0;
                if (SDL_LockTexture(hDisplayTex, nullptr, (void**)&texture_pixels, &pitch) == 0) {
                    if (pitch == bufWidth * 4) {
                        memcpy(texture_pixels, pSrcDisplayBuf, (size_t)origHeight * pitch);
                    } else {
                        for (int curY = 0; curY < origHeight; curY++) {
                            uint8_t* dest = (uint8_t*)texture_pixels + (curY * pitch);
                            memcpy(dest, &pSrcDisplayBuf[curY * bufWidth], bufWidth * 4);
                        }
                    }
                    SDL_UnlockTexture(hDisplayTex);
                }
            }
            pVmPlayer->unlockDisplayTexBuf();
        }
    }

    // ALWAYS render: clear + copy texture + present.
    // On macOS Metal, presenting without draw commands shows black.
    // When no new frame exists, the texture still holds the last frame's
    // data so RenderCopy redraws it — keeping the drawable alive.
    SDL_RenderClear(m_hWndRenderer);
    if (m_hVmDisplayTx && m_lastTexW > 0)
        SDL_RenderCopy(m_hWndRenderer, m_hVmDisplayTx, nullptr, &m_lastDstRect);

    if (m_bShowGui)
        m_pQimGuiCtx->render();

    SDL_RenderPresent(m_hWndRenderer);
}


// Function to recreate a dynamic texture with new dimensions
SDL_Texture* QsrMainClientWndApp::tryRecreateEmuScreenTexture(int newWidth, int newHeight) {
    // Get the format of the old texture
    int access, currentWidth, currentHeight;
    Uint32 format;
    if (SDL_QueryTexture(m_hVmDisplayTx, &format, &access, &currentWidth, &currentHeight) != 0)
        return m_hVmDisplayTx;
    if (newWidth == currentWidth && newHeight == currentHeight) {
        return m_hVmDisplayTx;
    }
    // Destroy the old texture
    SDL_DestroyTexture(m_hVmDisplayTx);
    // Create a new texture with the desired dimensions
    m_hVmDisplayTx = SDL_CreateTexture(m_hWndRenderer, format,
                                       access,  // Using the same access pattern as the original
                                       newWidth, newHeight);
    return m_hVmDisplayTx;
}


void QsrMainClientWndApp::destroyImp() {
    destroyUaeWindow();

    if (m_pDesktop) {
        m_pDesktop->destroy();
        //delete m_pDesktop;
        m_pDesktop = nullptr;
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
    IVmClientPlayer* pVmProvider = getVmProvider();
    uint32_t uaeWndId = SDL_GetWindowID(m_pWindow);
    switch (event.type) {
        case SDL_KEYDOWN: {
            if (event.key.windowID != uaeWndId)
                return qd::EFlow::CONTINUE;

            SDL_Keysym sym = event.key.keysym;
            if (sym.sym == SDLK_F12) {
                if (sym.mod & KMOD_SHIFT) {
                    // Handle shift + F12
                    SDL_SetRelativeMouseMode(SDL_FALSE); // Release mouse for debugger
                    doOperation_<qsr::operations::ShowDebuggerWnd>();
                } else {
                    setShowImgui(!m_bShowGui);
                    if (m_bShowGui) {
                        SDL_SetRelativeMouseMode(SDL_FALSE); // Release mouse when UI opens
                    }
                }
                return qd::EFlow::STOP;
            } else if (sym.sym == SDLK_ESCAPE) {
                if (g_cfg_vm_wnd.quitByEsc) {
                    getApp()->requestAppToQuit();
                    return qd::EFlow::STOP;
                }
                // Also release mouse on ESC
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
            if (pVmProvider)
                pVmProvider->pushSdlEvent(event);
            return qd::EFlow::STOP;
        } break;

        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL: {
            uint32_t eventWndId = 0;
            if (event.type == SDL_MOUSEMOTION) eventWndId = event.motion.windowID;
            else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) eventWndId = event.button.windowID;
            else if (event.type == SDL_MOUSEWHEEL) eventWndId = event.wheel.windowID;

            // For simplicity, if the UI is open, let ImGui consume the mouse entirely.
            if (m_bShowGui) {
                return m_pQimGuiCtx->onSdlEventProc(event);
            }

            if (eventWndId != uaeWndId) {
                return qd::EFlow::CONTINUE;
            }

            // Capture mouse if clicked inside the emulator screen
            if (event.type == SDL_MOUSEBUTTONDOWN && !SDL_GetRelativeMouseMode()) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }

            // If captured (or wheel event), forward to UAE core
            if (SDL_GetRelativeMouseMode() || event.type == SDL_MOUSEWHEEL) {
                if (pVmProvider)
                    pVmProvider->pushSdlEvent(event);
                return qd::EFlow::STOP;
            }
            return qd::EFlow::CONTINUE;
        } break;

        case SDL_KEYUP: {
            if (event.key.keysym.sym == SDLK_F12)
                return qd::EFlow::STOP;
            if (pVmProvider)
                pVmProvider->pushSdlEvent(event);
        } break;

        case SDL_WINDOWEVENT: {
            if (event.window.windowID != uaeWndId)
                return qd::EFlow::CONTINUE;
            uint8_t wndEvent = event.window.event;
            if (wndEvent == SDL_WINDOWEVENT_CLOSE) {
                getApp()->requestAppToQuit();
            } else if (wndEvent == SDL_WINDOWEVENT_FOCUS_LOST) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
            break;
        }

        default:
            break;
    }
    if (m_bShowGui)
        return m_pQimGuiCtx->onSdlEventProc(event);

    return qd::EFlow::CONTINUE;
}


IVmClientPlayer* QsrMainClientWndApp::getVmProvider() const {
    return m_pVmClientPlayer;
}


void QsrMainClientWndApp::setVmPlayer(qsr::IVmClientPlayer* VmProvider) {
    if (m_pVmClientPlayer == VmProvider)
        return;
    m_pVmClientPlayer = VmProvider;
}


qd::EFlow QsrMainClientWndApp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
    if (auto p = args->cast_<qsr::operations::ShowDebuggerWnd>()) {
        qd::unused(p);
        amD::DebuggerApp* pDbg = getApp()->getDebuggerApp();
        pDbg->setWndVisible(true);
        return qd::EFlow::STOP;
    }
    // send operation to UAE thread
    if (IVmClientPlayer* pUaeThread = getVmProvider()) {
        qd::operation::BaseOpArgs* pClonedArgs = args->clone();
        pUaeThread->pushOperationMsg(qtd::unique_ptr<qd::operation::BaseOpArgs>(pClonedArgs));
    }
    return qd::EFlow::STOP;
}


IVm::VM* QsrMainClientWndApp::getVm() const {
    return m_pVmClientPlayer ? m_pVmClientPlayer->getVm() : nullptr;
}


void QsrMainClientWndApp::destroyUaeWindow() {
    SDL_DestroyTexture(m_hVmDisplayTx);
    m_hVmDisplayTx = nullptr;
    SDL_DestroyRenderer(m_hWndRenderer);
    m_hWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}


};  // namespace qsr
