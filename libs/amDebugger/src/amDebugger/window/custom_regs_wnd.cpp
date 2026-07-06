#include "custom_regs_wnd.h"
#include "amDebugger/debuggerWndApp.h"
#include "qd/imGui/imGui.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/string.h"
#include "qd/stl/utility.h"
#include "amDebugger/ui/uiStyle.h"
#include <cstdint>
#include <cstdio>


namespace amD {
namespace window {


// Amiga custom chip identification.
// Chip ID per register: index = (addr & 0x1FE) >> 1, giving 256 entries.
enum ChipId : uint8_t { CH_A = 0, CH_D = 1, CH_P = 2, CH_N = 3 };

// 256-entry lookup: offset >> 1  ->  owning chip (A=Agnus, D=Denise, P=Paula, N=none)
// Addresses from Amiga Hardware Reference Manual register map.
static const ChipId REG_CHIP[256] = {
//  00   02   04   06   08   0A   0C   0E
    CH_A,CH_A,CH_A,CH_A,CH_P,CH_P,CH_P,CH_D, // 00 BLTDDAT..CLXDAT
    CH_P,CH_P,CH_P,CH_P,CH_P,CH_P,CH_P,CH_P, // 10 ADKCONR..INTREQR
    CH_A,CH_A,CH_P,CH_P,CH_A,CH_A,CH_A,CH_A, // 20 DSKPTH..COPCON
    CH_P,CH_P,CH_P,CH_P,CH_A,CH_A,CH_A,CH_A, // 30 SERDAT..STRLONG
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, // 40 BLTCON0..BLTBPTL
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, // 50 BLTAPTH..BLTSIZH
    CH_A,CH_A,CH_A,CH_A,CH_N,CH_N,CH_N,CH_N, // 60 BLTCMOD..unused
    CH_A,CH_A,CH_A,CH_N,CH_N,CH_N,CH_D,CH_P, // 70 BLTCDAT..DSKSYNC
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, // 80 COP1LCH..DIWSTRT
    CH_A,CH_A,CH_A,CH_A,CH_D,CH_P,CH_P,CH_P, // 90 DIWSTOP..ADKCON
    CH_A,CH_A,CH_P,CH_P,CH_P,CH_P,CH_N,CH_N, // A0 AUD0LCH..unused
    CH_A,CH_A,CH_P,CH_P,CH_P,CH_P,CH_N,CH_N, // B0 AUD1LCH..unused
    CH_A,CH_A,CH_P,CH_P,CH_P,CH_P,CH_N,CH_N, // C0 AUD2LCH..unused
    CH_A,CH_A,CH_P,CH_P,CH_P,CH_P,CH_N,CH_N, // D0 AUD3LCH..unused
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, // E0 BPL1PTH..BPL4PTL
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, // F0 BPL5PTH..BPL8PTL
    CH_D,CH_D,CH_D,CH_D,CH_A,CH_A,CH_D,CH_D, //100 BPLCON0..CLXCON2
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //110 BPL1DAT..BPL8DAT
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, //120 SPR0PTH..SPR3PTL
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A, //130 SPR4PTH..SPR7PTL
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //140 SPR0POS..SPR1DATB
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //150 SPR2POS..SPR3DATB
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //160 SPR4POS..SPR5DATB
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //170 SPR6POS..SPR7DATB
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //180 COLOR00..COLOR07
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //190 COLOR08..COLOR15
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //1A0 COLOR16..COLOR23
    CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D,CH_D, //1B0 COLOR24..COLOR31
    CH_A,CH_D,CH_D,CH_D,CH_A,CH_A,CH_A,CH_A, //1C0 HTOTAL..VBSTOP
    CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_A,CH_D, //1D0 SPRHSTRT..HSSTRT
    CH_D,CH_D,CH_A,CH_N,CH_N,CH_N,CH_N,CH_N, //1E0 VSSTRT..unused
    CH_N,CH_N,CH_N,CH_N,CH_N,CH_N,CH_A,CH_N, //1F0 reserved..FMODE
};

// Chip variant per chipset level [OCS/ECS/AGA][chipId]
struct ChipVariant { const char* name; const char* partno; const char* role; const char* models; };

static const ChipVariant CHIP_VARIANTS[3][4] = {
    // OCS (A1000 / A500 / A2000)
    {
        {"Agnus",   "8361/8371", "DMA, blitter, copper, memory", "A1000, A500, A2000"},
        {"Denise",  "8362",      "Video, bitplanes, sprites, colors, collisions", "A1000, A500, A2000"},
        {"Paula",   "8364",      "Audio, disk, serial, interrupts, I/O", "All models"},
        {"",        "",          "Reserved or unused", ""},
    },
    // ECS (A500+ / A600 / A3000)
    {
        {"Fat Agnus",    "8373", "DMA, blitter, copper, memory (ECS)", "A500+, A600, A3000"},
        {"Super Denise", "8373", "Video, bitplanes, sprites, colors, collisions (ECS)", "A500+, A600, A3000"},
        {"Paula",        "8364", "Audio, disk, serial, interrupts, I/O", "All models"},
        {"",             "",     "Reserved or unused", ""},
    },
    // AGA (A1200 / A4000)
    {
        {"Alice", "8374", "DMA, blitter, copper, memory (AGA)", "A1200, A4000"},
        {"Lisa",  "8374", "Video, bitplanes, sprites, colors, collisions (AGA)", "A1200, A4000"},
        {"Paula", "8364", "Audio, disk, serial, interrupts, I/O", "All models"},
        {"",      "",     "Reserved or unused", ""},
    },
};


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
    IVm::CustomRegs* custRegs = nullptr;
    float itemValW = 0.0f;  // SetNextItemWidth for value InputText
    int csLevel = 0;        // 0=OCS, 1=ECS, 2=AGA

    void drawColumn(CustReg reg_id, char* valBuf, int valBufSize) {
        const qtd::string_view& regName = reg_id.toString();
        ImGui::TextColored(uiGetColorF(UiStyle::CustomRegsWnd_RegName), "%s", regName.data());

        if (ImGui::BeginItemTooltip()) {
            const auto& rd = CustReg::cust_reg_data[reg_id];
            ChipId chipId = REG_CHIP[(rd.addr & 0x1FE) >> 1];
            const ChipVariant& chip = CHIP_VARIANTS[csLevel][chipId];

            // Header: register name
            ImGui::TextColored(uiGetColorF(UiStyle::CustomRegsWnd_RegName),
                               "%.*s", (int)regName.size(), regName.data());
            // Chip variant line
            if (chip.name[0]) {
                ImGui::TextDisabled("%s \xc2\xa7%s \xe2\x80\x94 %s", chip.name, chip.partno, chip.role);
                ImGui::TextDisabled("Models: %s", chip.models);
            } else {
                ImGui::TextDisabled("Reserved or unused");
            }
            ImGui::TextDisabled("Addr: DFF%03X", rd.addr & 0x1FF);
            if (rd.desc && *rd.desc) {
                ImGui::Separator();
                ImGui::PushTextWrapPos(280.0f);
                ImGui::TextUnformatted(rd.desc);
                ImGui::PopTextWrapPos();
            }

            // Flag bit breakdown if available
            if (reg_id.getFlagDesc()) {
                ImGui::Separator();
                FlagsTooltipContent flgContent;
                flgContent.drawRegisterFlagsTooltip(custRegs, reg_id);
            }
            ImGui::EndTooltip();
        }

        ImGui::OpenPopupOnItemClick("#flagsPopup", ImGuiPopupFlags_MouseButtonLeft);

        ImGui::TableNextColumn();

        uint16_t regVal = custRegs->getRegVal(reg_id);
        char id[32];
        snprintf(id, sizeof(id), "##%s", regName.data());

        // Only refresh buffer when widget is not being actively edited
        ImGuiID widgetId = ImGui::GetID(id);
        if (ImGui::GetActiveID() != widgetId)
            snprintf(valBuf, valBufSize, "%04X", regVal);

        ImGui::SetNextItemWidth(itemValW);
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::CustomRegsWnd_RegValue));
        if (ImGui::InputText(id, valBuf, valBufSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
        }
        ImGui::PopStyleColor();
    }
};  // struct DrawCustomRegColumn


void CustomRegsWnd::drawContentImp() {
    Debugger* dbg = getDbg();
    IVm::VM* vm = dbg->getVm();

    IVm::CustomRegs* custRegs = vm->custom;
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
        const float framePadX = ImGui::GetStyle().FramePadding.x;
        const float cellPadX  = ImGui::GetStyle().CellPadding.x;
        const float charW     = ImGui::GetFontSize();

        // Label column: longest custom reg names are 8 chars (SPR0DATA, BLTCON0L, etc.)
        constexpr float kLabelW = 96.0f;
        // Value column: 4 hex digits for InputText width
        const float kValItemW = charW * 4.0f;
        // Full value column width includes padding
        const float kValColW  = kValItemW + framePadX * 2.0f + cellPadX * 2.0f;

        // Per-register value buffers to avoid buffer aliasing
        static char s_valBufs[IVm::CustReg::_COUNT_][8];

        int flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollY;
        DrawCustomRegColumn dr;
        dr.custRegs = custRegs;
        dr.itemValW = kValItemW;
        dr.csLevel = vm->getChipsetLevel();
        ImVec2 rgn = ImGui::GetContentRegionAvail();
        if (ImGui::BeginTable("##registers", 4, flags, ImVec2(rgn.x, rgn.y))) {
            ImGui::TableSetupColumn("##lbl1", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kLabelW);
            ImGui::TableSetupColumn("##val1", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kValColW);
            ImGui::TableSetupColumn("##lbl2", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kLabelW);
            ImGui::TableSetupColumn("##val2", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kValColW);

            uint32_t halfSize = ((uint32_t)regsList.size() + 1) / 2;
            for (uint32_t i = 0; i < halfSize; ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                int ri1 = (int)regsList[i].first;
                dr.drawColumn(regsList[i].first, s_valBufs[ri1], sizeof(s_valBufs[0]));
                ImGui::TableNextColumn();
                if ((halfSize + i) >= regsList.size())
                    break;
                int ri2 = (int)regsList[halfSize + i].first;
                dr.drawColumn(regsList[halfSize + i].first, s_valBufs[ri2], sizeof(s_valBufs[0]));
            }
            ImGui::EndTable();
        }
    }
}

};  // namespace window
};  // namespace amD
