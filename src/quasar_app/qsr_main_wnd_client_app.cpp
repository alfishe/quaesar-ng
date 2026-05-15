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

    m_nCurVmPlayterId = m_vmSelector.activateVmPlayerByIdStr(getApp(), g_cfg_vm_wnd.vmPlayerId.c_str());
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

    m_hWndRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
    // Always clear the backbuffer first (SDL docs: backbuffer is undefined after Present)
    SDL_RenderClear(m_hWndRenderer);

    bool hasNewEmuFrame = false;

    // render VM display texture screen
    IVmClientPlayer* pVmPlayer = getVmProvider();
    if (pVmPlayer) {
        uint32_t curFrame = pVmPlayer->getScrFrameNo();
        if (curFrame != m_renderedFrameNo) {
            m_renderedFrameNo = curFrame;
            hasNewEmuFrame = true;

            int curWndSizeX, curWndSizeY;
            SDL_GetRendererOutputSize(m_hWndRenderer, &curWndSizeX, &curWndSizeY);

            int bufWidth, bufHeight;
            uint32_t* pSrcDisplayBuf = nullptr;
            if (pVmPlayer->lockDisplayTexBuf(&bufWidth, &bufHeight, &pSrcDisplayBuf)) {
                if (!bufWidth || !bufHeight) {
                    pVmPlayer->unlockDisplayTexBuf();
                    return;
                }

                // Amiga PAL non-interlaced produces ~288 visible lines (half-frame).
                // Double the source lines to fill the display properly.
                const bool bNeedsLineDoubling = (bufHeight < 350);
                const int srcHeight = bNeedsLineDoubling ? bufHeight : bufHeight;
                const int dstHeight = bNeedsLineDoubling ? bufHeight * 2 : bufHeight;

                // Maintain aspect ratio
                float image_aspect = (float)bufWidth / (float)dstHeight;
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

                SDL_Texture* hDisplayTex =
                    tryRecreateEmuScreenTexture(bufWidth, dstHeight);
                void* texture_pixels = nullptr;
                int pitch = 0;
                if (SDL_LockTexture(hDisplayTex, nullptr, (void**)&texture_pixels, &pitch) == 0) {
                    if (bNeedsLineDoubling) {
                        // Line-doubling: each source line is written twice
                        for (int curY = 0; curY < srcHeight; curY++) {
                            uint8_t* dest1 = (uint8_t*)texture_pixels + (curY * 2 * pitch);
                            uint8_t* dest2 = (uint8_t*)texture_pixels + ((curY * 2 + 1) * pitch);
                            memcpy(dest1, &pSrcDisplayBuf[curY * bufWidth], bufWidth * 4);
                            memcpy(dest2, &pSrcDisplayBuf[curY * bufWidth], bufWidth * 4);
                        }
                    } else {
                        // 1:1 copy
                        for (int curY = 0; curY < srcHeight; curY++) {
                            uint8_t* dest = (uint8_t*)texture_pixels + (curY * pitch);
                            memcpy(dest, &pSrcDisplayBuf[curY * bufWidth], bufWidth * 4);
                        }
                    }
                    SDL_UnlockTexture(hDisplayTex);
                }

                SDL_RenderCopy(m_hWndRenderer, hDisplayTex, nullptr, &rect);
                pVmPlayer->unlockDisplayTexBuf();
            }
        }
    }

    if (m_bShowGui)
        m_pQimGuiCtx->render();

    // Always present — with VSync enabled, SDL_RenderPresent blocks until the
    // next vertical blank (~16ms at 60Hz), providing natural frame-rate limiting.
    // Skipping it would cause a busy-spin consuming 100% of one CPU core.
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
                    doOperation_<qsr::operations::ShowDebuggerWnd>();
                } else
                    setShowImgui(!m_bShowGui);
                return qd::EFlow::STOP;
            } else if (sym.sym == SDLK_ESCAPE) {
                if (g_cfg_vm_wnd.quitByEsc) {
                    getApp()->requestAppToQuit();
                    return qd::EFlow::STOP;
                }
            }
            if (pVmProvider)
                pVmProvider->pushSdlEvent(event);
            return qd::EFlow::STOP;
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
