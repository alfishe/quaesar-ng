#include "commonOperations.h"
#include "SDL_video.h"
#include "msg_list.h"


namespace amD::operation {



qd::EFlow UaeWndAlwaysOnTop::applyOperationMsgProc(qd::operation::args::Base* msg)
{
    SDL_Window *emuWindow = getEmulatorMainWindow();

    if (auto p = msg->cast_<qd::operation::args::DoOperation>())
    {
        Uint32 flags = SDL_GetWindowFlags(emuWindow);
        bool setOnTop = (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
        SDL_SetWindowAlwaysOnTop(emuWindow, (SDL_bool)(!setOnTop));
        return qd::EFlow::SUCCESS;
    }
    else if (auto p = msg->cast_<amD::operation::args::MenuItemStateGet>())
    {
        Uint32 flags = SDL_GetWindowFlags(emuWindow);
        p->checked = (flags & SDL_WINDOW_ALWAYS_ON_TOP) ? 1 : 0;
        return qd::EFlow::SUCCESS;
    }
    else
        return Operation::applyOperationMsgProc(msg);
}


};
