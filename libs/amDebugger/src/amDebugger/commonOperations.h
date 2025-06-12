#include "amDebugger/dbgOperation.h"
#include "qd/UI/uiOperationMessages.h"


struct SDL_Window;


namespace qd::operation {


struct DebugDmaOption : public Operation {
    QDB_REG_OPERATION(qd::operation::DebugDmaOption);

    void setup() { m_name = "Debug DMA"; }

    qd::EFlow applyOperationMsgProc(operation::msg::Base* p_msg) override
    {
        if (auto p = p_msg->cast_<operation::msg::DoOperation>())
        {
            changeDebugDmaMode(p->arg0.getInt());
            return EFlow::SUCCESS;
        }
        else
            return Operation::applyOperationMsgProc(p_msg);

    }

    int getCurDebugDmaMode();
    void changeDebugDmaMode(int nMode);
};
//////////////////////////////////////////////////////////////////////////



struct UaeWndAlwaysOnTop : public Operation {
    QDB_REG_OPERATION(qd::operation::UaeWndAlwaysOnTop);
    void setup()
    {
        m_name = "Always on Top";
        // supportMtd.set(UiDrawEvent::MainMenu_Emul).set(UiDrawEvent::MenuItemStateChecked);
        addShortcut(qd::shortcut::EId::AlwaysOnTopEmu);
    }

    virtual EFlow applyOperationMsgProc(operation::msg::Base*p_msg) override;

    SDL_Window* getEmulatorMainWindow();
};
//////////////////////////////////////////////////////////////////////////


}; // namespace qd::operation
