#include "screen_wnd.h"
#include "qd/imGui/imGuiHelperClass.h"
#include <amDebugger/debuggerApp.h>
#include <amDebugger/vm/vmInterface.h>
#include <qd/imGui/imgui_eastl.h>
#include <SDL.h>

namespace amD {
namespace window {

void ScreenWnd::drawContentImp()
{
    Debugger* dbg = getDbg();
    IVm::VM* vm = dbg->getVm();

    grabScreenToTexture(dbg);

    int scrSizeX = vm->getScreenSizeX();
    int scrSizeY = vm->getScreenSizeY();
    float scrollbarSize = ImGui::GetStyle().ScrollbarSize;
    ImVec2 scrollingChildSize = ImVec2(ImGui::GetWindowWidth() - scrollbarSize, ImGui::GetWindowHeight() - 30);
    if (auto bc = qIm::LockChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None,
            ImGuiWindowFlags_HorizontalScrollbar))
    {
        int hPos = vm->getHPos();
        int vPos = vm->getVPos();
        int cycle = vm->getCurCycle();

        ImGui::Text("VPos:%i HPos:%i cycle:%i", vPos, hPos, cycle);

        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1(p0.x + scrSizeX, p0.y + scrSizeY);
        ImGui::Image(mTextureId, ImVec2((float)scrSizeX, (float)scrSizeY), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImGui::GetStyleColorVec4(ImGuiCol_Border));

        ImDrawList* dr = ImGui::GetWindowDrawList();

        // VPOS
        dr->AddLine({p0.x, p0.y + (float)vPos}, {p1.x, p0.y + (float)vPos}, qd::Color::MAGENTA75);

        // HPOS
        dr->AddLine({p0.x + (float)hPos, p0.y}, {p0.x + hPos, p1.y}, qd::Color::MAGENTA75);
    }
}


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
            SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
        }
        else
        {
            SDL_Log("Could not create texture: %s", SDL_GetError());
        }
    }

    if (mTextureId)
    {
        SDL_Texture* scrTexture = (SDL_Texture*)(mTextureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0)
        {
            int vbSizeX = 0;
            int vbSizeY = 0;
            int vbPitch = 0;
            void* scrBuf = vm->blitter->getScreenPixBuf(0, &vbSizeX, &vbSizeY, &vbPitch);

            if (scrBuf)
            {
                for (int y = 0; y < scrSizeY; y++)
                {
                    uint8_t* sptr = (uint8_t*)scrBuf + (y * vbPitch);
                    uint32_t* dest = ((uint32_t*)pixels) + (y * scrSizeX);
                    for (int x = 0; x < scrSizeX; ++x)
                    {
                        qd::Color c = *(uint32_t*)(sptr);
                        c.a = 255;
                        *dest = c;
                        ++dest;
                        sptr += 4;
                    }
                }
            }
            SDL_UnlockTexture(scrTexture);
        }
    }
}


}; // namespace window
}; // namespace amD
