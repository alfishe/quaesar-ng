#pragma once
#include <SDL_events.h>
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/base.h"
#include "qd/stl/unique_ptr.h"

FORWARD_DECLARATION_3(qd, operation, BaseOpArgs);


namespace qsr {

class IVmServerThread {
public:
    virtual IVm::VM* getVm() const = 0;
    virtual uint32_t getScrFrameNo() = 0;
    virtual void pushSdlEvent(const SDL_Event&) = 0;
    virtual void pushOperationMsg(qd::unique_ptr<qd::operation::BaseOpArgs>) = 0;
    virtual bool lockDisplayTexBuf(int* out_width, int* out_height, uint32_t** out_pixels) = 0;
    virtual void unlockDisplayTexBuf() = 0;

};  // class IVmServerThread
//////////////////////////////////////////////////////////////////////////

};  // namespace qsr
