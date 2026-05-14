#include "uiView.h"
#include <amDebugger/ui/debuggerDesktop.h>
#include <amDebugger/debugger.h>
#include <amDebugger/vm/vmInterface.h>
#include <qd/base/Tribool.h>
#include <qd/typeSystem/typeInfo.h>


namespace amD {



Debugger* AmDbgWindow::getDbg() const
{
    return ui->getDbg();
}


IVm::VM* AmDbgWindow::getVm() const
{
    return getDbg()->getVm();
}


void AmDbgWindow::drawImp()
{
    // Check VM availability once per frame for all debugger windows.
    // When no VM is attached (dummy connection or before emulator starts),
    // we still render the ImGui window chrome but skip content and show
    // a disabled-state message. This centralises the null-VM guard so
    // individual windows can assume vm != nullptr in drawContentImp().
    const bool vmAvailable = (getVm() != nullptr);

    // --- Replicate UiWindow::drawImp() with VM guard around drawContentImp() ---
    assert(!m_title.empty());
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
