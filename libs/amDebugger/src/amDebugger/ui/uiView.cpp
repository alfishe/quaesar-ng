#include "uiView.h"
#include <amDebugger/ui/debuggerDesktop.h>
#include <amDebugger/debugger.h>
#include <amDebugger/vm/vmInterface.h>
#include <qd/base/Tribool.h>
#include <qd/typeSystem/typeInfo.h>
#include <SDL.h>
#include <cstdint>


namespace amD {



Debugger* AmDbgWindow::getDbg() const
{
    return ui ? ui->getDbg() : nullptr;
}


IVm::VM* AmDbgWindow::getVm() const
{
    Debugger* dbg = getDbg();
    return dbg ? dbg->getVm() : nullptr;
}


void AmDbgWindow::drawImp()
{
    // Bail out if the window has not been created yet (ui == null means
    // onCreate() has not run, so getDbg()/getVm() would return null).
    if (!ui) {
        return;
    }

    // Safety check: if title is empty, onCreate() likely didn't run properly
    if (getText().empty())
        return;

    // Check VM availability once per frame for all debugger windows.
    Debugger* dbg = getDbg();
    if (!dbg)
        return;
    
    IVm::VM* vm = dbg->getVm();
    if (!vm)
        return;
    
    // SAFETY: isReady() checks mInit sentinel first to detect corrupted VM objects
    const bool vmAvailable = vm->isReady();

    // --- Replicate UiWindow::drawImp() with VM guard around drawContentImp() ---
    const bool bModal = isModal();

    qd::Tribool vis;
    if (m_bVisible)
    {
        m_bFocus = ImGui::IsItemFocused();
        assert(!m_title.empty());
        if (m_size.isSizeValid())
        {
            ImVec2 size((float)m_size.x, (float)m_size.y);
            ImGui::SetNextWindowSize(size);
        }

        if (bModal)
        {
            ImGui::OpenPopup(m_title.c_str());
            vis = (qd::Tribool)ImGui::BeginPopupModal(m_title.c_str(), &m_bVisible, m_windowFlags);
        }
        else
            vis = (qd::Tribool)ImGui::Begin(m_title.c_str(), &m_bVisible, m_windowFlags);

        if (!m_bVisible)
            qd::c_def(0);
    }
    else
        qd::c_def(0);

    if (vis.isTrue())
    {
        if (vmAvailable)
        {
            drawContentImp();
        }
        else
        {
            // Show disabled placeholder — window chrome is visible but no content
            ImGui::TextDisabled("No VM connected");
        }
    }

    if (vis.hasBool())
    {
        if (bModal)
        {
            if (vis.isTrue())
                ImGui::EndPopup();
        }
        else
            ImGui::End();
    }
    else
        BPT();
}


void window::ImGuiDemoWindow::drawImp() {
    ImGui::ShowDemoWindow(&m_bVisible);
}


void _onUiWindowCreated(const qd::TypeInfo &/*meta*/, UiViewCreateCtx *cp, amD::AmDbgWindow * newInst)
{
//     if (auto typeIdAttr = meta.getAttribute_<qd::tsAttr::CustomClassId32>())
//         newInst->mClassId = typeIdAttr->getId32();
    newInst->onCreate(cp);
}


};  // namespace amD
