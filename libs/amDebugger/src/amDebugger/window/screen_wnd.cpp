#include "screen_wnd.h"
#include <qd/imGui/imGui.h>
#include "amDebugger/debuggerWndApp.h"
#include <amDebugger/vm/vmInterface.h>
#include <SDL.h>
#include "qd/qui/controls/menuItemOperation.h"

namespace amD {
namespace window {

void ScreenWnd::drawContentImp()
{
    Debugger* dbg = getDbg();
    IVm::VM* vm = dbg->getVm();

    m_windowFlags |= ImGuiWindowFlags_MenuBar;
    if (ImGui::BeginMenuBar())
    {
        if (auto pm = qIm::LockMenu("View"))
        {
            ImGui::MenuItem("Show VPos/HPos", nullptr, &g_dbg_cfg->showVHPopsLines);
        }
        ImGui::EndMenuBar();
    }

    // Detect resize in progress
    ImVec2 curWndSize = ImGui::GetWindowSize();
    bool isResizing = (mLastWndSize.x != curWndSize.x || mLastWndSize.y != curWndSize.y) && mLastWndSize.x > 0;
    mLastWndSize = curWndSize;

    if (!isResizing)
        grabScreenToTexture(dbg);

    int scrSizeX = vm->getScreenSizeX();
    int scrSizeY = vm->getScreenSizeY();
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    if (auto bc = qIm::LockChild("##scrolling", availSize, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar))
    {
        int hPos = vm->getHPos();
        int vPos = vm->getVPos();
        int cycle = vm->getCurCycle();

        ImGui::Text("VPos:%i HPos:%i cycle:%i", vPos, hPos, cycle);

        // Calculate scaled size to fit window while preserving aspect ratio
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float displayW = (float)scrSizeX;
        float displayH = (float)scrSizeY;
        float scale = 1.0f;
        if (avail.x > 1.0f && avail.y > 1.0f && scrSizeX > 0 && scrSizeY > 0)
        {
            float scaleX = avail.x / (float)scrSizeX;
            float scaleY = avail.y / (float)scrSizeY;
            scale = (scaleX < scaleY) ? scaleX : scaleY;
            displayW = (float)scrSizeX * scale;
            displayH = (float)scrSizeY * scale;
        }

        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1(p0.x + displayW, p0.y + displayH);

        if (isResizing)
        {
            // During resize: show placeholder frame
            ImDrawList* dr = ImGui::GetWindowDrawList();
            dr->AddRect(p0, p1, IM_COL32(128, 128, 128, 255), 0.0f, 0, 2.0f);
            ImGui::Dummy(ImVec2(displayW, displayH));
        }
        else
        {
            // Normal: show scaled image
            ImGui::Image(mTextureId, ImVec2(displayW, displayH), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImGui::GetStyleColorVec4(ImGuiCol_Border));

            if (g_dbg_cfg->showVHPopsLines)
            {
                ImDrawList* dr = ImGui::GetWindowDrawList();
                dr->AddLine({p0.x, p0.y + vPos * scale}, {p1.x, p0.y + vPos * scale}, qd::Color::MAGENTA75);
                dr->AddLine({p0.x + hPos * scale, p0.y}, {p0.x + hPos * scale, p1.y}, qd::Color::MAGENTA75);
            }
        }
    }
}


// grabScreenToTexture — reads the emulator framebuffer snapshot.
// Called from drawContentImp() which is already gated by the centralized
// 15fps refresh trigger in DebuggerApp::updateAppPart(). No separate
// frame-skip logic needed here — the whole ImGui frame only runs when
// the trigger fires.
void ScreenWnd::grabScreenToTexture(Debugger* dbg)
{
    IVm::VM* vm = dbg->getVm();

    int scrSizeX = vm->getScreenSizeX();
    int scrSizeY = vm->getScreenSizeY();

    if (!mTextureId)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            return;

        SDL_Texture* scrTexture = SDL_CreateTexture(dbg->getDbgApp()->getRenderer(), SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, scrSizeX, scrSizeY);
        if (scrTexture)
        {
            mTextureId = (ImTextureID)scrTexture;
            // Blend mode none: the framebuffer is fully opaque. This avoids
            // the old per-pixel alpha fixup loop.
            SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_NONE);
            SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
        }
        else
        {
            SDL_Log("Could not create texture: %s", SDL_GetError());
        }
    }

    if (mTextureId)
    {
        // --- Lock-free framebuffer snapshot read ---
        // getScreenPixBuf() returns m_pAmigaBuffer — the stable snapshot
        // captured at the last vsync boundary. We do NOT acquire the display
        // texture mutex, avoiding all contention with the main window render
        // path and the emulator thread.
        //
        // There is a ~1% chance of catching a partially-updated buffer (rare
        // horizontal tearing), which is acceptable for a debugger preview.
        int vbSizeX = 0;
        int vbSizeY = 0;
        int vbPitch = 0;
        void* scrBuf = vm->blitter->getScreenPixBuf(0, &vbSizeX, &vbSizeY, &vbPitch);

        if (scrBuf)
        {
            SDL_Texture* scrTexture = (SDL_Texture*)(mTextureId);
            void* pixels = nullptr;
            int pitch;
            if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0)
            {
                // Bulk copy when pitch and dimensions match, otherwise
                // per-scanline fallback for dimension mismatches.
                if (pitch == vbPitch && pitch == scrSizeX * 4 && vbSizeX == scrSizeX && vbSizeY == scrSizeY)
                {
                    memcpy(pixels, scrBuf, (size_t)scrSizeY * pitch);
                }
                else
                {
                    int copyW = (vbSizeX < scrSizeX) ? vbSizeX : scrSizeX;
                    int copyH = (vbSizeY < scrSizeY) ? vbSizeY : scrSizeY;
                    for (int y = 0; y < copyH; y++)
                    {
                        memcpy((uint8_t*)pixels + y * pitch,
                               (uint8_t*)scrBuf + y * vbPitch,
                               copyW * 4);
                    }
                }
                SDL_UnlockTexture(scrTexture);
            }
        }
    }
}


}; // namespace window
}; // namespace amD
