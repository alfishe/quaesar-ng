#include "custom_regs_wnd.h"
#include "amDebugger/debuggerWndApp.h"
#include "qd/imGui/imGui.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/string.h"
#include "qd/stl/utility.h"
#include "amDebugger/ui/uiStyle.h"


namespace amD {
namespace window {


struct FlagsTooltipContent {
    void drawRegisterFlagsTooltip(IVm::CustomRegs* custRegs, IVm::CustReg reg_id)
    {
        const CustomFlagsDesc* fd = reg_id.getFlagDesc();
        if (!fd)
            return;
        uint16_t rv = custRegs->getRegVal(reg_id);
        int nRowsMax = (int)fd->bits.size() + 2 / 3;

        int flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit;
        ImVec2 rgn = {200, 200};
        if (ImGui::BeginTable("##popupFlags", 6, flags, ImVec2(rgn.x, rgn.y))) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 40);  // name
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 15);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40);  // name
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 15);
            ImGui::TableNextRow();
            int b = 0;
            for (int r = 0; r < nRowsMax; ++r) {
                ImGui::TableSetColumnIndex(0);

                _drawRegCols(fd, b, rv);
                _drawRegCols(fd, b + 3, rv);
                b++;
            }

            ImGui::EndTable();
        }
    }

private:
    void _drawRegCols(const CustomFlagsDesc* fd, int b, uint16_t rv);

};  // struct FlagsTooltipContent


void FlagsTooltipContent::_drawRegCols(const CustomFlagsDesc* fd, int b, uint16_t rv) {
    if (b >= (int)fd->bits.size())
        return;
    const CustomFlagsDesc::Bits& cb = fd->bits[b];
    ImGui::TextColored(uiGetColorF(UiStyle::CustomRegsWnd_RegName), "%02i", cb.noBeg);
    ImGui::TableNextColumn();

    ImGui::TextColored(uiGetColorF(UiStyle::CustomRegsWnd_RegName), "%s", cb.name.data());
    ImGui::TableNextColumn();

    int bitsVal = (rv >> cb.shiftL) & cb.mask;
    ImGui::TextColored(uiGetColorF(UiStyle::CustomRegsWnd_RegName), "%i", bitsVal);
    ImGui::TableNextColumn();
}


struct DrawCustomRegColumn {
    qd::InlineString stReg;
    qd::InlineString stVal, stCmd, stId;
    IVm::CustomRegs* custRegs = nullptr;

    void drawColumn(CustReg reg_id) {
        stReg.assign(reg_id.toString().data(), reg_id.toString().size());
        ImGui::TextColored(uiGetColorF(UiStyle::CustomRegsWnd_RegName), "%s", stReg.c_str());

        if (ImGui::BeginItemTooltip()) {
            FlagsTooltipContent flgContent;
            flgContent.drawRegisterFlagsTooltip(custRegs, reg_id);
            ImGui::EndTooltip();
        }

        ImGui::OpenPopupOnItemClick("#flagsPopup", ImGuiPopupFlags_MouseButtonLeft);

        ImGui::TableNextColumn();

        uint16_t regVal = custRegs->getRegVal(reg_id);

        stVal = qd::string_format("%04X", regVal).data();
        stId.assign("##") += stReg;
        ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::CustomRegsWnd_RegValue));
        if (ImGui::InputText(stId.c_str(), &stVal, ImGuiInputTextFlags_EnterReturnsTrue)) {
        }
        ImGui::PopStyleColor();
    }
};  // struct DrawCustomRegColumn


void CustomRegsWnd::drawContentImp() {
    Debugger* dbg = getDbg();
    if (!dbg)
        return;
    IVm::VM* vm = dbg->getVm();
    if (!vm)
        return;

    IVm::CustomRegs* custRegs = vm->custom;
    if (!custRegs)
        return;
    custRegs->fetch();

    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 0);

    mRegsFilter.Draw();

    qtd::fixed_vector<qtd::pair<CustReg, const char*>, CustReg::_COUNT_> regsList;
    for (int i = (CustReg)0; i != CustReg::_COUNT_; ++i) {
        qtd::string_view strReg = CustReg(i).toString();
        if (!mRegsFilter.PassFilter(strReg.data(), strReg.data() + strReg.size()))
            continue;
        regsList.emplace_back(CustReg(i), strReg.data());
    }

    if (!regsList.empty()) {
        int flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_ScrollY;
        DrawCustomRegColumn dr;
        dr.custRegs = custRegs;
        ImVec2 rgn = ImGui::GetContentRegionAvail();
        if (ImGui::BeginTable("##registers", 4, flags, ImVec2(rgn.x, rgn.y))) {
            uint32_t halfSize = ((uint32_t)regsList.size() + 1) / 2;
            for (uint32_t i = 0; i < halfSize; ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                dr.drawColumn(regsList[i].first);
                ImGui::TableNextColumn();
                if ((halfSize + i) >= regsList.size())
                    break;
                dr.drawColumn(regsList[halfSize + i].first);
            }
            ImGui::EndTable();
        }
    }
}

};  // namespace window
};  // namespace amD
