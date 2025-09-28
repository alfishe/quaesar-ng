#include "memory_graph_wnd.h"
#include <EASTL/span.h>
#include <SDL.h>
#include "amDebugger/debuggerWndApp.h"
#include <amDebugger/vm/vmInterface.h>
#include "qd/imGui/imGui.h"


namespace amD {
namespace window {


void MemoryGraphWnd::drawContentImp() {

    amD::Debugger* dbg = getDbg();
    IVm::VM* vm = dbg->getVm();

    m_newTextureSize.y = (int)ImGui::GetWindowHeight() - 150;

    float curTime = (float)ImGui::GetTime();

    if (!m_textureId || m_textureSize != m_newTextureSize) {
        if (m_textureId) {
            SDL_DestroyTexture((SDL_Texture*)m_textureId);
            m_textureId = 0;
        }
        if (m_newTextureSize.y > 1 && m_newTextureSize.y > 0) {
            m_textureSize = m_newTextureSize;
            SDL_Texture* scrTexture = nullptr;
            scrTexture = SDL_CreateTexture(dbg->getDbgApp()->getRenderer(), SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, m_textureSize.x, m_textureSize.y);
            if (scrTexture) {
                m_textureId = (ImTextureID)(scrTexture);
                SDL_SetTextureBlendMode(scrTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(scrTexture, SDL_ScaleModeLinear);
                m_lastTextureCreateTime = curTime;
            } else {
                SDL_Log("Can't not create texture: %s", SDL_GetError());
            }
        }
    }

    const MemBank* pCurBank = vm->mem->getBankByInd(m_curBank);
    if (!pCurBank) {
        m_curBank = EMemSrc::CHIP;
        return;
    }

    //AddrRef addrPtr = m_bankOffset + pCurBank->m_startAddr;
    qd::InlineString addrStr(m_exprAddr.getStrVal().begin(), m_exprAddr.getStrVal().end());
    if (ImGui::InputText("Address", &addrStr)) {
        m_exprAddr.setStrVal(addrStr);
        qd::Var16 val;
        if (m_exprAddr.evaluate(vm, val))
            m_bankOffset = val.getU32();
    }

    qd::InlineString selBankName = "null";
    if (pCurBank) {
        selBankName.assign(pCurBank->m_name.begin(), pCurBank->m_name.end());
        selBankName.append_sprintf(" (%06Xh - %06Xh)", (uint32_t)pCurBank->m_startAddr,
                                   (uint32_t)pCurBank->m_startAddr + pCurBank->m_size);
    }
    if (ImGui::BeginCombo("Memory bank", selBankName.c_str(), ImGuiComboFlags_None)) {
        eastl::span<const IVm::MemBank> banks = vm->mem->m_banks;
        for (size_t nBank = 0; nBank < banks.size(); ++nBank) {
            const IVm::MemBank& curBank = banks[nBank];
            selBankName.assign(curBank.m_name.begin(), curBank.m_name.end());
            selBankName.append_sprintf(" (%06Xh-%06Xh)", (uint32_t)curBank.m_startAddr,
                                       (uint32_t)curBank.m_startAddr + curBank.m_size);
            if (ImGui::Selectable(selBankName.c_str(), nBank == (size_t)m_curBank)) {
                m_curBank = (int)nBank;
                m_textureMod = 0;
                pCurBank = vm->mem->getBankByInd(m_curBank);
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
                m_curBank = IVm::EMemSrc::CHIP;
                pCurBank = vm->mem->getBankByInd(m_curBank);
                m_bankOffset = bplPtr - pCurBank->m_startAddr;
                CustReg modReg = (nb & 1) == 0 ? CustReg::BPL1MOD : CustReg::BPL2MOD;
                m_textureMod = (short)vm->custom->getRegVal(modReg);
                // FIXME: How to calculate real bitplane width from ddfstart/ddfstop
                // int res = 161 / 2 - ddfstrt;
                int bpWidth = ((ddfstop - ddfstrt) + 8) * 2;
                if (bpWidth > 320)
                    bpWidth -= ddfstrt;
                m_newTextureSize.x = bpWidth;
            }
        }
        ImGui::EndCombo();
    }

    if (m_textureId && (curTime - m_lastTextureCreateTime) > 0.1f) {
        SDL_Texture* scrTexture = (SDL_Texture*)(m_textureId);
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(scrTexture, nullptr, &pixels, &pitch) == 0) {
            uint8_t* memPtr = (uint8_t*)vm->mem->getRealAddr(m_bankOffset + pCurBank->m_startAddr);

            // Change pixels
            uint32_t* dest = ((uint32_t*)pixels);
            const int rowBytes = m_textureSize.x / 8;
            for (int y = 0; y < m_textureSize.y; y++) {
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
                    memPtr += m_textureMod;
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
        int addrPtr = pCurBank->m_size - m_bankOffset;
        if (ImGui::VSliderInt("##AddrSlider", ImVec2(32.0f, (float)m_newTextureSize.y), &addrPtr, 0, pCurBank->m_size,
                              "")) {
            m_bankOffset = pCurBank->m_size - addrPtr;
        }
    }
    ImGui::SameLine();

    ImVec2 scrollingChildSize = ImVec2(ImGui::GetContentRegionAvail().x - 00.f, m_textureSize.y + 32.f);
    if (auto p = qIm::LockChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::Image(m_textureId, ImVec2((float)m_textureSize.x, (float)m_textureSize.y), ImVec2(0.f, 0.f),
            ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), ImGui::GetStyleColorVec4(ImGuiCol_Border));
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
            {
                mStartDragBankOffset = m_bankOffset;
            }
            if (ImGui::IsMouseDragging(0))
            {
                ImVec2 dragDelta = ImGui::GetMouseDragDelta();
                m_bankOffset = mStartDragBankOffset - int(dragDelta.y / 8.f) * (m_textureSize.x + m_textureMod) -
                              int(dragDelta.x / 8.f);
            }
        }
    }


    // texture width
    {
        int& txSize = m_newTextureSize.x;
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
            m_newTextureSize.x = txSize;
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
        ImGui::InputInt("mod", &m_textureMod, 2);

        txSize = qd::clamp(txSize, 1, 320 * 8);
    }
}

};  // namespace window
};  // namespace amD
