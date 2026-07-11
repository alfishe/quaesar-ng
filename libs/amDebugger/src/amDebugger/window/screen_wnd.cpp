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
    if (!dbg)
        return;
    IVm::VM* vm = dbg->getVm();
    if (!vm || !vm->isReady())
        return;

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

    // Use the actual framebuffer dimensions (tracked by grabScreenToTexture)
    // rather than vm->getScreenSizeX/Y which defaults to 760x576 and may not
    // reflect the real engine output (e.g. vAmiga produces a single PAL field
    // of ~285 visible lines).
    int scrSizeX = m_texWidth > 0 ? m_texWidth : vm->getScreenSizeX();
    int scrSizeY = m_texHeight > 0 ? m_texHeight : vm->getScreenSizeY();
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    if (auto bc = qIm::LockChild("##scrolling", availSize, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar))
    {
        int hPos = vm->getHPos();
        int vPos = vm->getVPos();
        int cycle = vm->getCurCycle();

        ImGui::Text("VPos:%i HPos:%i cycle:%i", vPos, hPos, cycle);

        // Detect PAL low-res modes (e.g. 285 visible lines for a single
        // non-interlaced PAL field). Match the main window logic: use doubled
        // height for display aspect ratio so the preview isn't vertically
        // squeezed. The texture has the original lines; ImGui stretches.
        bool isLowRes = (scrSizeY < 350);
        int displaySrcH = isLowRes ? (scrSizeY * 2) : scrSizeY;

        // Calculate scaled size to fit window while preserving aspect ratio
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float displayW = (float)scrSizeX;
        float displayH = (float)displaySrcH;
        float scale = 1.0f;
        if (avail.x > 1.0f && avail.y > 1.0f && scrSizeX > 0 && displaySrcH > 0)
        {
            float scaleX = avail.x / (float)scrSizeX;
            float scaleY = avail.y / (float)displaySrcH;
            scale = (scaleX < scaleY) ? scaleX : scaleY;
            displayW = (float)scrSizeX * scale;
            displayH = (float)displaySrcH * scale;
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
    if (!dbg)
        return;
    IVm::VM* vm = dbg->getVm();

    // Read actual framebuffer dimensions first — these reflect the real engine
    // output and may differ from the IVm::VM defaults (e.g. vAmiga produces a
    // single PAL field of ~285 lines, UAE produces ~576).
    int vbSizeX = 0;
    int vbSizeY = 0;
    int vbPitch = 0;
    void* scrBuf = vm->blitter->getScreenPixBuf(0, &vbSizeX, &vbSizeY, &vbPitch);
    if (!scrBuf || vbSizeX <= 0 || vbSizeY <= 0)
        return;

    Uint32 vmFormat = vm->blitter->getScreenPixelFormat();

    // (Re)create texture when dimensions or format change (first frame, engine switch).
    if (!mTextureId || m_texWidth != vbSizeX || m_texHeight != vbSizeY || m_texFormat != vmFormat)
    {
        if (mTextureId)
            SDL_DestroyTexture((SDL_Texture*)mTextureId);

        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            return;

        SDL_Texture* scrTexture = SDL_CreateTexture(dbg->getDbgApp()->getRenderer(),
            vmFormat, SDL_TEXTUREACCESS_STREAMING, vbSizeX, vbSizeY);
        if (scrTexture)
        {
            mTextureId = (ImTextureID)scrTexture;
            m_texWidth = vbSizeX;
            m_texHeight = vbSizeY;
            m_texFormat = vmFormat;
            SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_NONE);
            SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
        }
        else
        {
            SDL_Log("Could not create texture: %s", SDL_GetError());
            return;
        }
    }

    if (mTextureId)
    {
        SDL_Texture* scrTexture = (SDL_Texture*)(mTextureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0)
        {
            // Bulk copy when pitch matches, otherwise per-scanline fallback.
            if (pitch == vbPitch)
            {
                memcpy(pixels, scrBuf, (size_t)vbSizeY * pitch);
            }
            else
            {
                for (int y = 0; y < vbSizeY; y++)
                {
                    memcpy((uint8_t*)pixels + y * pitch,
                           (uint8_t*)scrBuf + y * vbPitch,
                           vbSizeX * 4);
                }
            }
            SDL_UnlockTexture(scrTexture);
        }
    }
}


}; // namespace window
}; // namespace amD
