#include "registers_wnd.h"
#include "amDebugger/debuggerWndApp.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/imGui/imGui.h"
#include "amDebugger/ui/uiStyle.h"
#include "qd/stl/string.h"
#include <cstdio>

namespace amD {
namespace window {

#define REG_A 0x00
#define REG_D 0x08

static const char* s_regLookup[] = {
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
};

struct FlagDef {
    const char* name;
    const char* displayName;
    ECpuFlg_ flagType;
};

static const FlagDef s_flagDefs[] = {
    {"Z", "Z:", CpuFlg_Z},
    {"C", "C:", CpuFlg_C},
    {"N", "N:", CpuFlg_N},
    {"V", "V:", CpuFlg_V},
    {"X", "X:", CpuFlg_X}
};


void RegistersView::drawContentImp() {
    Debugger* dbg = getDbg();
    IVm::VM* vm = dbg->getVm();
    if (!vm)
        return;
    IVm::Cpu* cpu = vm->cpu;
    if (!cpu)
        return;

    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 0);

    // Raw char buffers for ImGui::InputText — one per widget, persisted
    // across frames. Only refreshed from CPU state when NOT actively editing.
    static char s_aBufs[8][16];
    static char s_dBufs[8][16];
    static char s_pcBuf[16];
    static char s_flagBufs[5][4];

    // Column widths: labels need ~24px, values need exactly 8 hex chars.
    // InputText outer width = text_width + FramePadding.x*2.
    // Column width must = InputText outer width + CellPadding.x*2.
    const float framePadX = ImGui::GetStyle().FramePadding.x;
    const float cellPadX  = ImGui::GetStyle().CellPadding.x;
    const float charW     = ImGui::GetFontSize();
    constexpr float kLabelW = 28.0f;
    const float kValItemW = charW * 8.0f;                          // text area only
    const float kValColW  = kValItemW + framePadX * 2.0f + cellPadX * 2.0f; // full column

    // Only allow editing when emulator is paused.
    const bool bPaused = vm->getVmDebugMode().isBreak();

    auto displayLabel = [](const char* name) {
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegName));
        ImGui::TextUnformatted(name);
        ImGui::PopStyleColor();
    };

    // Read-only display (running)
    auto showValue = [](uint32_t val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%08X", val);
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegValue));
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
    };

    // Editable display (paused): refreshes from VM unless this widget is active
    auto editHexValue = [&](char* buf, int bufSize, uint32_t val,
                            const char* reg_name) {
        char id[32];
        snprintf(id, sizeof(id), "##%s", reg_name);
        ImGuiID widgetId = ImGui::GetID(id);
        if (ImGui::GetActiveID() != widgetId)
            snprintf(buf, bufSize, "%08X", val);
        ImGui::SetNextItemWidth(kValItemW);
        ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::RegistersWnd_RegValue));
        if (ImGui::InputText(id, buf, bufSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "r %s %s", reg_name, buf);
            dbg->execConsoleCmd(cmd);
        }
        ImGui::PopStyleColor();
    };

    auto editFlagValue = [&](char* buf, const char* flag_name, uint32_t flag_val) {
        char id[32];
        snprintf(id, sizeof(id), "##%s", flag_name);
        ImGuiID widgetId = ImGui::GetID(id);
        if (ImGui::GetActiveID() != widgetId)
            snprintf(buf, 4, "%01X", flag_val);
        ImGui::SetNextItemWidth(kValItemW);
        if (ImGui::InputText(id, buf, 4, ImGuiInputTextFlags_EnterReturnsTrue)) {
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "r %s %s", flag_name, buf);
            dbg->execConsoleCmd(cmd);
        }
    };

    // Render a register value cell: InputText when paused, Text when running
    auto regCell = [&](char* buf, int bufSize, uint32_t val, const char* name) {
        if (bPaused)
            editHexValue(buf, bufSize, val, name);
        else
            showValue(val);
    };

    auto flagCell = [&](char* buf, const char* name, uint32_t val) {
        if (bPaused)
            editFlagValue(buf, name, val);
        else {
            char tmp[4];
            snprintf(tmp, sizeof(tmp), "%01X", val);
            ImGui::TextUnformatted(tmp);
        }
    };

    int flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##registers", 4, flags, ImVec2(0, 0))) {
        // NoResize on every column prevents user-drag width changes.
        ImGui::TableSetupColumn("##a_lbl", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kLabelW);
        ImGui::TableSetupColumn("##a_val", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kValColW);
        ImGui::TableSetupColumn("##d_lbl", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kLabelW);
        ImGui::TableSetupColumn("##d_val", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, kValColW);

        for (int i = 0; i < 8; ++i) {
            ImGui::TableNextRow();

            // A register
            ImGui::TableNextColumn();
            displayLabel(s_regLookup[REG_A + i]);
            ImGui::TableNextColumn();
            regCell(s_aBufs[i], 16, cpu->getRegA(i), s_regLookup[REG_A + i]);

            // D register
            ImGui::TableNextColumn();
            displayLabel(s_regLookup[REG_D + i]);
            ImGui::TableNextColumn();
            regCell(s_dBufs[i], 16, cpu->getRegD(i), s_regLookup[REG_D + i]);
        }

        // PC and IMASK
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        displayLabel("PC");
        ImGui::TableNextColumn();
        regCell(s_pcBuf, 16, cpu->getPC(), "PC");
        ImGui::TableNextColumn();
        displayLabel("IMASK");
        ImGui::TableNextColumn();
        {
            if (bPaused)
                editHexValue(s_flagBufs[4], 4, (uint32_t)cpu->getIntMask(), "IMASK");
            else
                showValue(cpu->getIntMask());
        }

        // CPU flags — 2 per row
        const int flagCount = sizeof(s_flagDefs) / sizeof(s_flagDefs[0]);
        for (int i = 0; i < flagCount; i++) {
            if (i % 2 == 0)
                ImGui::TableNextRow();

            ImGui::TableNextColumn();
            displayLabel(s_flagDefs[i].displayName);
            ImGui::TableNextColumn();
            flagCell(s_flagBufs[i], s_flagDefs[i].name, cpu->getFlg(s_flagDefs[i].flagType));
        }

        ImGui::EndTable();
    }
}

};  // namespace window
};  // namespace amD
