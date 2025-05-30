#include "uae_actions.h"


namespace qd {
namespace action {
namespace uae {

void DebugDmaOption::onDrawMainMenuItem(UiDrawEvent::Type event) {
    switch (event) {
        case UiDrawEvent::MainMenu_Debug: {
            const char* options =
                "off\0"
                "mode 2\0"
                "mode 3\0"
                "mode 4\0"
                "\0";
            int n = ::debug_dma > 0 ? ::debug_dma - 1 : 0;
            if (ImGui::Combo("Debug DMA", &n, options)) {
                eastl::string buf(eastl::string::CtorSprintf(), "v -%d", n + 1);
                getDbg()->applyImmediateConsoleCmd(eastl::move(buf));
            }
        } break;
        default:
            break;
    }
}


qd::EFlow DebugDmaOption::applyActionMsgProc(action::msg::Base* p_msg) {
    if (p_msg->id == msg::OnDrawMainMenuItem::CID) {
        auto p = p_msg->cast_<msg::OnDrawMainMenuItem>();
        onDrawMainMenuItem(p->drawElement);
    }
    return TSuper::applyActionMsgProc(p_msg);
}


};  // namespace uae
};  // namespace action
};  // namespace qd
