#pragma once
#include <SDL_events.h>
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/base.h"
#include "qd/stl/unique_ptr.h"

FORWARD_DECLARATION_3(qd, operation, BaseOpArgs);


namespace qsr {

class IVmClientPlayer {
public:
    virtual IVm::VM* getVm() const = 0;
    virtual int getScrFrameNo() = 0;
    virtual void pushSdlEvent(const SDL_Event&) = 0;
    virtual void pushOperationMsg(qtd::unique_ptr<qd::operation::BaseOpArgs>) = 0;
    virtual bool lockDisplayTexBuf(int* out_width, int* out_height, uint32_t** out_pixels) = 0;
    virtual void unlockDisplayTexBuf() = 0;
    // Returns SDL pixel format constant (SDL_PIXELFORMAT_ARGB8888 by default).
    // vAmiga overrides to SDL_PIXELFORMAT_ABGR8888 — GPU converts for free,
    // no per-pixel CPU swap needed.
    virtual Uint32 getDisplayPixelFormat() const {
        return SDL_PIXELFORMAT_ARGB8888;
    }
    virtual ~IVmClientPlayer() = default;

};  // class IVmClientPlayer
//////////////////////////////////////////////////////////////////////////

};  // namespace qsr
