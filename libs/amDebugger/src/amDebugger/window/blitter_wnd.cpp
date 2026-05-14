#include <amDebugger/debuggerWndApp.h>
#include <amDebugger/debuggerOps.h>
#include <amDebugger/ui/uiView.h>
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vmInterface.h>
#include "qd/stl/string.h"
#include "qd/stl/fixed_vector.h"
#include <qd/base/Color.h>
#include <qd/imGui/imGui.h>


namespace amD {
namespace window {
class BlitterWnd : public amD::AmDbgWindow
{
    QDB_WINDOW_REGISTER(WndId::BlitterWnd, amD::window::BlitterWnd, amD::AmDbgWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override
    {
        AmDbgWindow::onCreate(cp);
        m_title = "Blitter";
    }

    virtual void drawContentImp() override;
};
//////////////////////////////////////////////////////////////////////////



struct DeclareDmaSrcUiArgs {
    uint16_t bltCon0;
    uint16_t bltCon1;
    IVm::CustomRegs* custRegs;
    char dmaLetter;
    BC0F::Type srcEnFlag;
    CustReg bltXPtH;
    CustReg bltXPtL;
    CustReg bltXMod;
    CustReg bltXDat;

    void declareDmaSrcUi();
};

void DeclareDmaSrcUiArgs::declareDmaSrcUi()
{
    qd::InlineString strTmp;
    ImGui::PushID((int)dmaLetter);
    strTmp = "##DMA_EN_";
    strTmp += dmaLetter;
    bool bSrcEnable = bltCon0 & srcEnFlag;
    ImGui::Checkbox(strTmp.c_str(), &bSrcEnable);
    ImGui::SameLine(30);
    strTmp = dmaLetter;
    strTmp += " DMA";
    ImGuiTreeNodeFlags_ fl = ImGuiTreeNodeFlags_AllowOverlap;
    if (ImGui::CollapsingHeader(strTmp.c_str(), fl | ImGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        const int ident = 8;
        ImGui::Indent(ident);
        uint32_t bltPtr = custRegs->getRegVal(bltXPtH) << 16 | custRegs->getRegVal(bltXPtL);
        strTmp = bltXPtH.toStringC();
        strTmp.pop_back();
        ImGui::TextUnformatted(strTmp.c_str());
        ImGui::SameLine();
        // ImGui::SetNextItemWidth(-1.f);
        strTmp.insert(0, "##", 2);
        ImGui::InputScalar(strTmp.c_str(), ImGuiDataType_U32, &bltPtr, nullptr, nullptr, "%06X",
            ImGuiInputTextFlags_CharsHexadecimal);

        int modVal = custRegs->getRegVal(bltXMod);
        ImGui::TextUnformatted(bltXMod.toString().data(), bltXMod.toString().data() + bltXMod.toString().size());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        strTmp = "##";
        strTmp.append(bltXMod.toString().data(), bltXMod.toString().size());
        ImGui::InputInt(strTmp.c_str(), &modVal, 0);

        // SHIFT
        if (dmaLetter == 'A' || dmaLetter == 'B')
        {
            strTmp = dmaLetter;
            strTmp += " shift";
            ImGui::TextUnformatted(strTmp.c_str());
            ImGui::SameLine();
            int shift = dmaLetter == 'A' ? (bltCon0 >> BC0F::SHIFTA) : (bltCon1 >> BC1F::SHIFTB);
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderInt("##shift", &shift, 0, 15);
        }

        // BLTxDAT
        int datVal = custRegs->getRegVal(bltXDat);
        ImGui::TextUnformatted(bltXDat.toString().data(), bltXDat.toString().data() + bltXDat.toString().size());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        strTmp = "##";
        strTmp.append(bltXDat.toString().data(), bltXDat.toString().size());
        ImGui::InputScalar(strTmp.c_str(), ImGuiDataType_U32, &datVal, nullptr, nullptr, "%04X",
            ImGuiInputTextFlags_CharsHexadecimal);

        ImGui::Unindent(ident);
    }
    ImGui::PopID();
}


void BlitterWnd::drawContentImp()
{
    Debugger* dbg = getDbg();
    if (!dbg)
        return;
    IVm::VM* vm = dbg->getVm();
    if (!vm)
        return;

    IVm::CustomRegs* custRegs = vm->custom;
    custRegs->fetch();


    qd::InlineString strAddr;
    qd::InlineString strTmp;

    uint16_t bltCon0 = custRegs->getRegVal(CustReg::BLTCON0);
    uint16_t bltCon1 = custRegs->getRegVal(CustReg::BLTCON1);
    uint16_t bltSize = custRegs->getRegVal(CustReg::BLTSIZE);
    uint16_t rDmaCon = custRegs->getRegVal(CustReg::DMACONR);

    bool bBltEn = rDmaCon & (DMAC::BLTEN | DMAC::DMAEN) && vm->blitter->isBlitterActive();
    ImGui::Checkbox("##DMAEN", &bBltEn);
    ImGui::SameLine(30);

    int bltSizeW = bltSize & ((1 << 6) - 1u);
    int bltSizeH = bltSize >> 6;
    ImGui::TextUnformatted("BltSizeW");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(30);
    ImGui::InputInt("##BLTSIZEW", &bltSizeW, 0);
    ImGui::SameLine();
    ImGui::TextUnformatted("BltSizeH");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(30);
    ImGui::InputInt("##BLTSIZEH", &bltSizeH, 0);

    DeclareDmaSrcUiArgs ar;
    ar.bltCon0 = bltCon0;
    ar.bltCon1 = bltCon1;
    ar.custRegs = custRegs;

    ImVec2 rgn = ImGui::GetContentRegionAvail();
    ImVec2 wndL = ImVec2(rgn.x * 0.5f, rgn.y);

    // left column
    uint32_t cldFlg = ImGuiChildFlags_None | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX;
    if (auto bc = qIm::LockChild("##LEFT_COL", wndL, cldFlg, ImGuiWindowFlags_None))
    {
        ImGui::TextUnformatted("BLTCON0");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputScalar("##BLTCON0", ImGuiDataType_U16, &bltCon0, nullptr, nullptr, "%04X",
            ImGuiInputTextFlags_CharsHexadecimal);

        ar.dmaLetter = 'A';
        ar.srcEnFlag = BC0F::SRCA;
        ar.bltXMod = CustReg::BLTAMOD;
        ar.bltXDat = CustReg::BLTADAT;
        ar.bltXPtH = CustReg::BLTAPTH;
        ar.bltXPtL = CustReg::BLTAPTL;
        ar.declareDmaSrcUi();

        ar.dmaLetter = 'B';
        ar.srcEnFlag = BC0F::SRCB;
        ar.bltXMod = CustReg::BLTBMOD;
        ar.bltXDat = CustReg::BLTBDAT;
        ar.bltXPtH = CustReg::BLTBPTH;
        ar.bltXPtL = CustReg::BLTBPTL;
        ar.declareDmaSrcUi();
    }

    ImGui::SameLine();

    // right column
    rgn = ImGui::GetContentRegionAvail();
    ImVec2 wndR = ImVec2(rgn.x * 0.0f, rgn.y);
    if (auto bc = qIm::LockChild("##RIGHT_COL", wndR, ImGuiChildFlags_None | ImGuiChildFlags_Borders, ImGuiWindowFlags_None))
    {
        ImGui::TextUnformatted("BLTCON1");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputScalar("##BLTCON1", ImGuiDataType_U16, &bltCon1, nullptr, nullptr, "%04X",
            ImGuiInputTextFlags_CharsHexadecimal);
        //
        ar.dmaLetter = 'C';
        ar.srcEnFlag = BC0F::SRCC;
        ar.bltXMod = CustReg::BLTCMOD;
        ar.bltXDat = CustReg::BLTCDAT;
        ar.bltXPtH = CustReg::BLTCPTH;
        ar.bltXPtL = CustReg::BLTCPTL;
        ar.declareDmaSrcUi();

        ar.dmaLetter = 'D';
        ar.srcEnFlag = BC0F::DEST;
        ar.bltXMod = CustReg::BLTDMOD;
        ar.bltXDat = CustReg::BLTDDAT;
        ar.bltXPtH = CustReg::BLTDPTH;
        ar.bltXPtL = CustReg::BLTDPTL;
        ar.declareDmaSrcUi();
    }
}


}; // namespace window
}; // namespace amD
