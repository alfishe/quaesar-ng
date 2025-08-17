#include "commonOperations.h"
#include "debuggerOps.h"
#include "SDL_video.h"


namespace amD::operation {


/*

qd::EFlow UaeWndAlwaysOnTop::applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg)
{
    SDL_Window* emuWindow = getEmulatorMainWindow(env);

    if (auto p = p_msg->cast_<qd::operation::args::DoOperation>())
    {
        Uint32 flags = SDL_GetWindowFlags(emuWindow);
        bool setOnTop = (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
        SDL_SetWindowAlwaysOnTop(emuWindow, (SDL_bool)(!setOnTop));
        return qd::EFlow::SUCCESS;
    }
    else if (auto p = p_msg->cast_<amD::operation::args::MenuItemStateGet>())
    {
        Uint32 flags = SDL_GetWindowFlags(emuWindow);
        p->checked = (flags & SDL_WINDOW_ALWAYS_ON_TOP) ? 1 : 0;
        return qd::EFlow::SUCCESS;
    }
    else
        return Operation::applyOperationMsgProc(env, p_msg);
}
*/


}; // namespace amD::operation
