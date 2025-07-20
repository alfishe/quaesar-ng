// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on

#include "uae_app_part.h"
#include "SDL.h"
#include "amDebugger/commonOperations.h"
#include "cli11/CLI11.hpp"
#include "parse_options.h"
#include "qd/app/appMessages.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/qimGui/controls/qimMenu.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "quaesar.h"
#include "quasar_app/ui/uae_wnd_desktop.h"
#include "ui/uae_options_wnd.h"


void UaeAppPart::onPartCreate(AppPartBase::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);
    setPartActive(true);
    setPartVisisble(true);

    createUaeWindow();

    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pImGui = pImGuiMgr->createContextImGui(m_pWindow, m_pUaeRenderer);
    m_pImGui->getIO().IniFilename = "";


    qd::UiNodeCreator mk;
    m_pDesktop = mk.make_<UaeWndDesktop>();
    auto pDlg = m_pDesktop->addChild_<UaeOptionsDlg>("options");
}


void UaeAppPart::createUaeWindow() {
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;

    SDL_AtomicSet(&m_scrFrameNo, 0);

    m_wndWidth = 754;
    m_wndHeight = 576;
    m_pAmigaBuffer = new uint32_t[m_wndWidth * m_wndHeight];

    // Create a window
    m_pWindow = SDL_CreateWindow("Quaesar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_wndWidth, m_wndHeight,
                                 window_flags);

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

    m_pUaeScrTexture = SDL_CreateTexture(m_pUaeRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                         m_wndWidth, m_wndHeight);

    if (!m_pUaeScrTexture) {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        SDL_DestroyRenderer(m_pUaeRenderer);
        SDL_DestroyWindow(m_pWindow);
        return;
    }
}


void UaeAppPart::render() {
    // render UAE texture screen
    int curFrame = SDL_AtomicGet(&m_scrFrameNo);
    if (curFrame == m_renderedFrameNo) {
        return;
    }
    m_renderedFrameNo = curFrame;

    int new_width = 0;
    int new_height = 0;
    int window_width, window_height;
    SDL_GetWindowSize(m_pWindow, &window_width, &window_height);

    // Maintain aspect ratio
    float image_aspect = (float)m_wndWidth / (float)m_wndHeight;
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
    if (m_UaeScrTextureMutex.tryLock()) {
        recreateTexture(m_wndWidth, m_wndHeight);  // Recreate texture if needed
        uint32_t* texture_pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(m_pUaeScrTexture, NULL, (void**)&texture_pixels, &pitch) == 0) {
            for (int y = 0; y < m_wndHeight; y++) {
                uint8_t* dest = (uint8_t*)&texture_pixels[y * m_wndWidth];
                memcpy(dest, &m_pAmigaBuffer[y * m_wndWidth], m_wndWidth * 4);
            }
            SDL_UnlockTexture(m_pUaeScrTexture);
        }

        SDL_RenderCopy(m_pUaeRenderer, m_pUaeScrTexture, NULL, &rect);
        m_UaeScrTextureMutex.unlock();
    }

    if (m_bShowImgui)
        m_pImGui->render();

    SDL_RenderPresent(m_pUaeRenderer);
}


// Function to recreate a dynamic texture with new dimensions
void UaeAppPart::recreateTexture(int newWidth, int newHeight) {
    int currentWidth = 0;
    int currentHeight = 0;

    // Get the format of the old texture
    Uint32 format;
    int access;
    SDL_QueryTexture(m_pUaeScrTexture, &format, &access, &currentWidth, &currentHeight);

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


uint32_t* UaeAppPart::lockUaeScreenTexBuf(int amiga_width, int amiga_height) {
    m_UaeScrTextureMutex.lock();

    if (amiga_width > m_wndWidth || amiga_height > m_wndHeight) {
        delete[] m_pAmigaBuffer;
        m_pAmigaBuffer = new uint32_t[m_wndWidth * m_wndHeight];
    }

    m_wndWidth = amiga_width;
    m_wndHeight = amiga_height;

    return m_pAmigaBuffer;
}


void UaeAppPart::unlockUaeScreenTexBuf() {
    m_UaeScrTextureMutex.unlock();
    SDL_AtomicIncRef(&m_scrFrameNo);
}


void UaeAppPart::update(float Delta, float Time) {
    m_pImGui->newFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (auto p1 = qIm::LockMenu("File")) {
            if (ImGui::MenuItem("Open DF0:")) {
                floppyslot& cfgFloppy = ::changed_prefs.floppyslots[0];
                show_image_file_open_dlg(cfgFloppy);
                qd::UiOperationMgr::get()->doOperation_<amD::operation::UaeResetAmiga>();
            }

            if (ImGui::MenuItem("Settings")) {
                UaeOptionsDlg* pOptionsDlg = m_pDesktop->findChildByIdName_<UaeOptionsDlg>("options");
                m_pDesktop->showModal(pOptionsDlg);
            }
            if (ImGui::MenuItem("Exit")) {
                g_pApp->requestAppToQuit();
            }
        }

        if (auto p2 = qIm::LockMenu("Emulator", true)) {
            qIm::menuItemOperation(STRINGIFY(amD::operation::ToggleTurboEmulation));
            qIm::menuItemOperation(STRINGIFY(amD::operation::UaeWndAlwaysOnTop));
            qIm::menuItemOperation(STRINGIFY(amD::operation::UaeResetAmiga));
        }
        ImGui::EndMainMenuBar();
    }

    if (m_bShowImgui)
        m_pDesktop->draw();

    m_pImGui->endFrame();
}


void UaeAppPart::destroyImp() {
    destroyUaeWindow();

    if (m_pDesktop) {
        m_pDesktop->destroy();
        delete m_pDesktop;
        m_pDesktop = nullptr;
    }
}


qd::EFlow UaeAppPart::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg) {
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


void UaeAppPart::onSdlEventProc(SDL_Event& event) {
    uint32_t wndId = SDL_GetWindowID(m_pWindow);
    switch (event.type) {
        case SDL_KEYDOWN: {
            if (event.key.windowID != wndId)
                break;
        }
            if (event.key.keysym.sym == SDLK_F12) {
                setShowImgui(!m_bShowImgui);
            }
        default:
            break;
    }

    if (m_bShowImgui)
        m_pImGui->onSdlEventProc(event);
}


void UaeAppPart::destroyUaeWindow() {
    SDL_DestroyTexture(m_pUaeScrTexture);
    m_pUaeScrTexture = nullptr;
    SDL_DestroyRenderer(m_pUaeRenderer);
    m_pUaeRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}
