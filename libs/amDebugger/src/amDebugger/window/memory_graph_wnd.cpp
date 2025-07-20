#include "memory_graph_wnd.h"
#include <EASTL/span.h>
#include <SDL.h>
#include <amDebugger/debugger.h>
#include <amDebugger/vm/vm.h>
#include <imgui/imgui_internal.h>
#include <qd/ImGui/imgui_eastl.h>
#include "qd/imGui/imGuiHelperClass.h"
//#include <quaesar.h>


namespace amD {
namespace window {


void MemoryGraphWnd::drawContentImp() {
    VM* vm = getDbg()->getVm();

    mNewTextureSize.y = (int)ImGui::GetWindowHeight() - 150;

    float curTime = (float)ImGui::GetTime();

    if (!mTextureId || mTextureSize != mNewTextureSize) {
        if (mTextureId) {
            SDL_DestroyTexture((SDL_Texture*)mTextureId);
            mTextureId = 0;
        }
        if (mNewTextureSize.y > 1 && mNewTextureSize.y > 0) {
            mTextureSize = mNewTextureSize;
            SDL_Texture* scrTexture = nullptr;
            scrTexture = SDL_CreateTexture(getDbg()->getRenderer(), SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, mTextureSize.x, mTextureSize.y);
            if (scrTexture) {
                mTextureId = (ImTextureID)(scrTexture);
                SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
                mLastTextureCreateTime = curTime;
            } else {
                SDL_Log("Can't not create texture: %s", SDL_GetError());
            }
        }
    }

    const MemBank* pCurBank = vm->mem->getBankByInd(mCurBank);
    if (!pCurBank) {
        mCurBank = MemBank::CHIP;
        return;
    }

    AddrRef addrPtr = mBankOffset + pCurBank->m_startAddr;
    qd::InlineString addrStr(m_exprAddr.getStrVal().begin(), m_exprAddr.getStrVal().end());
    if (ImGui::InputText("Address", &addrStr)) {
        m_exprAddr.setStrVal(addrStr);
        qd::Var16 val;
        if (m_exprAddr.evaluate(vm, val))
            mBankOffset = val.getUInt();
    }

    eastl::inline_string<255, false> selBankName = "null";
    if (pCurBank) {
        selBankName.assign(pCurBank->m_name.begin(), pCurBank->m_name.end());
        selBankName.append_sprintf(" (%06Xh - %06Xh)", (uint32_t)pCurBank->m_startAddr,
                                   (uint32_t)pCurBank->m_startAddr + pCurBank->m_size);
    }
    if (ImGui::BeginCombo("Memory bank", selBankName.c_str(), ImGuiComboFlags_None)) {
        eastl::span<const amD::MemBank> banks = vm->mem->banks;
        for (int nBank = 0; nBank < banks.size(); ++nBank) {
            const amD::MemBank& curBank = banks[nBank];
            selBankName.assign(curBank.m_name.begin(), curBank.m_name.end());
            selBankName.append_sprintf(" (%06Xh-%06Xh)", (uint32_t)curBank.m_startAddr,
                                       (uint32_t)curBank.m_startAddr + curBank.m_size);
            if (ImGui::Selectable(selBankName.c_str(), nBank == mCurBank)) {
                mCurBank = nBank;
                mTextureMod = 0;
                pCurBank = vm->mem->getBankByInd(mCurBank);
            }
        }
        vm->custom->fetch();
        uint16_t ddfstrt = vm->custom->getRegVal(CustReg::DDFSTRT);
        uint16_t ddfstop = vm->custom->getRegVal(CustReg::DDFSTOP);

        for (int nb = 0; nb < 5; ++nb) {
            AddrRef bplPtr = vm->custom->getRegVal(CustReg::BPL1PTH + nb * 2) << 16 |
                             vm->custom->getRegVal(CustReg::BPL1PTH + nb * 2 + 1);
            selBankName.sprintf("BPL %i (%06Xh)###BPL%i", nb + 1, bplPtr, nb);
            if (ImGui::Selectable(selBankName.c_str())) {
                mCurBank = MemBank::CHIP;
                pCurBank = vm->mem->getBankByInd(mCurBank);
                mBankOffset = bplPtr - pCurBank->m_startAddr;
                CustReg modReg = (nb & 1) == 0 ? CustReg::BPL1MOD : CustReg::BPL2MOD;
                mTextureMod = (short)vm->custom->getRegVal(modReg);
                // FIXME: How to calculate real bitplane width from ddfstart/ddfstop
                // int res = 161 / 2 - ddfstrt;
                int bpWidth = ((ddfstop - ddfstrt) + 8) * 2;
                if (bpWidth > 320)
                    bpWidth -= ddfstrt;
                mNewTextureSize.x = bpWidth;
            }
        }
        ImGui::EndCombo();
    }

    if (mTextureId && (curTime - mLastTextureCreateTime) > 0.1f) {
        SDL_Texture* scrTexture = (SDL_Texture*)(mTextureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0) {
            uint8_t* memPtr = (uint8_t*)vm->mem->getRealAddr(mBankOffset + pCurBank->m_startAddr);

            // Change pixels
            uint32_t* dest = ((uint32_t*)pixels);
            const int rowBytes = mTextureSize.x / 8;
            for (int y = 0; y < mTextureSize.y; y++) {
                if (memPtr + rowBytes < pCurBank->m_realAddr + pCurBank->m_size) {
                    for (int x = 0; x < rowBytes; x++) {
                        uint8_t sb = *memPtr;
                        uint8_t m = 0x80;
                        for (int b = 0; b < 8; b++) {
                            qd::Color c = (sb & m) != 0 ? qd::Color::WHITE : qd::Color::BLACK;
                            *dest = c;
                            dest++;
                            m >>= 1;
                        }
                        ++memPtr;
                    }
                    memPtr += mTextureMod;
                } else {
                    for (int x = 0; x < rowBytes; x++)
                        (*dest++) = qd::Color::BLACK;
                }
            }
            SDL_UnlockTexture(scrTexture);
        } else
            SDL_Log("Cant lock texture");
    }
    // address slider
    {
        int addrPtr = pCurBank->m_size - mBankOffset;
        if (ImGui::VSliderInt("##AddrSlider", ImVec2(32.0f, (float)mNewTextureSize.y), &addrPtr, 0, pCurBank->m_size,
                              "")) {
            mBankOffset = pCurBank->m_size - addrPtr;
        }
    }
    ImGui::SameLine();

    ImVec2 scrollingChildSize = ImVec2(ImGui::GetContentRegionAvail().x - 00.f, mTextureSize.y + 32.f);
    if (auto p = qIm::LockChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::Image(mTextureId, ImVec2((float)mTextureSize.x, (float)mTextureSize.y), ImVec2(0.f, 0.f),
            ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), ImGui::GetStyleColorVec4(ImGuiCol_Border));
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
            {
                mStartDragBankOffset = mBankOffset;
            }
            if (ImGui::IsMouseDragging(0))
            {
                ImVec2 dragDelta = ImGui::GetMouseDragDelta();
                mBankOffset = mStartDragBankOffset - int(dragDelta.y / 8.f) * (mTextureSize.x + mTextureMod) -
                              int(dragDelta.x / 8.f);
            }
        }
    }


    // texture width
    {
        int& txSize = mNewTextureSize.x;
        if (ImGui::Button("<<"))
            txSize /= 2;
        ImGui::SameLine();
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ArrowButton("##minus", ImGuiDir_Left)) {
            txSize--;
        }
        // ImGui::PopItemFlag();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
        if (ImGui::DragInt("##Width", &txSize, 1.0f, 1, 320 * 8, "%d", ImGuiSliderFlags_None)) {
            mNewTextureSize.x = txSize;
        }
        ImGui::SameLine();
        // ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ArrowButton("##plus", ImGuiDir_Right)) {
            txSize++;
        }
        ImGui::SameLine();
        if (ImGui::Button(">>"))
            txSize *= 2;
        ImGui::PopItemFlag();  // button repeat

        ImGui::SameLine();
        ImGui::TextUnformatted("Width");
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("mod", &mTextureMod, 2);

        txSize = qd::clamp(txSize, 1, 320 * 8);
    }
}

};  // namespace window
};  // namespace amD
