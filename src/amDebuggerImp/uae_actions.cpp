#include "uae_actions.h"


namespace qd {
namespace action {

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
    //     if (auto p = p_msg->cast_<msg::OnDrawMainMenuItem>()) {
    //         onDrawMainMenuItem(p->drawElement);
    //     }
    return TSuper::applyActionMsgProc(p_msg);
}


qd::EFlow UaeWndAlwaysOnTop::applyActionMsgProc(action::msg::Base* msg) {
    if (auto p = msg->cast_<action::msg::DoAction>()) {
        Uint32 flags = SDL_GetWindowFlags(app->mUaeWindow);
        bool setOnTop = (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
        SDL_SetWindowAlwaysOnTop(app->mUaeWindow, (SDL_bool)(!setOnTop));
        return EFlow::SUCCESS;
    } else if (auto p = msg->cast_<action::msg::MenuItemStateGet>()) {
        Uint32 flags = SDL_GetWindowFlags(app->mUaeWindow);
        p->checked = (flags & SDL_WINDOW_ALWAYS_ON_TOP) ? 1 : 0;
        return EFlow::SUCCESS;
    } else
        return Action::applyActionMsgProc(msg);
}


};  // namespace action
};  // namespace qd
