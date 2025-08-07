#include "vm_uae_imp.h"
// clang-format off
#include <sysconfig.h>
#include <sysdeps.h>
#include <options.h>
#include <keyboard.h>
#include <inputdevice.h>
#include <inputrecord.h>
#include <keybuf.h>
#include <custom.h>
#include <blitter.h>
#include <xwin.h>
#include <drawing.h>
#include <savestate.h>
#include <debug.h>
// clang-format on
#include <SDL_log.h>
#include <amDebugger/debuggerApp.h>
#include <amDebugger/debuggerOps.h>
#include <amDebugger/vm/absVM.h>
#include <qd/base/endian.h>
#include <qd/qui/uiOperationMgr.h>
#include <qd/thread/thread.h>
#include <qd/typeSystem/typeInfo.h>
#include <quasar_app/quaesar.h>


extern bool get_custom_color_reg(int colreg, uae_u8* r, uae_u8* g, uae_u8* b);
extern uaecptr bplpt[MAX_PLANES], bplptx[MAX_PLANES];


namespace amD {
namespace vm::imp {


UaeVmImp::UaeVmImp() {
    cpu = &instCpu;
    mem = &instMemory;
    custom = &instCustomRegs;
    copper = &instCopper;
    blitter = &instBlitter;
    {
        instEmu.vm = this;
        emu = &instEmu;
    }
}


void UaeVmImp::init() {
    if (mInit)
        return;
    mInit = true;
    amD::AbsVM* s = (amD::AbsVM*)(this);
    uint32_t hiAddr = 0;
    while (hiAddr < MEMORY_BANKS) {
        addrbank* uaeBank = mem_banks[hiAddr];
        if (!uaeBank->allocated_size) {
            ++hiAddr;
            continue;
        }

        bool combined = false;
        for (MemBank& existBank : s->mem->banks) {
            if (existBank.m_realAddr == uaeBank->baseaddr) {
                combined = true;
                hiAddr += uaeBank->allocated_size >> 16;
                break;
            }
        }
        if (combined)
            continue;

        MemBank& memBank = s->mem->banks.emplace_back();
        memBank.m_id = (int)s->mem->banks.size() - 1;
        memBank.m_name = uaeBank->name;
        memBank.m_label = uaeBank->label;
        memBank.m_startAddr = uaeBank->start;
        memBank.m_realAddr = uaeBank->baseaddr;
        memBank.m_mask = uaeBank->mask;
        memBank.m_size = uaeBank->allocated_size;
        hiAddr += memBank.m_size >> 16u;
    }
}


UaeVmImp::~UaeVmImp() {
}


void* UaeVmImp::Blitter::getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) {
    vidbuf_description* vidinfo = &adisplays[mon_id].gfxvidinfo;
    vidbuffer* vb = &vidinfo->drawbuffer;
    if (!vb || !vb->bufmem)
        return nullptr;
    *out_size_w = vb->outwidth;
    *out_size_h = vb->outheight;
    *pitch = vb->rowbytes;
    return vb->bufmem;
}


bool UaeVmImp::Blitter::isBlitterActive() const {
    return blt_info.blit_main || blt_info.blit_finald || blt_info.blit_queued;
}


void UaeVmImp::CustomRegs::fetch() {
    size_t dump_len;
    ::save_custom(&dump_len, (uae_u8*)regsData.data(), 1);
    for (size_t i = 0; i < regsData.size(); ++i)
        qd::swapBytes_<2>(&regsData[i]);
}


void UaeVmImp::CustomRegs::commit() {
    eastl::fixed_vector<uint16_t, CustReg::_COUNT_ + data_offset, false> dst = {regsData.begin(), regsData.end()};
    uint8_t* beg = (uint8_t*)dst.begin();
    dst.erase((uint16_t*)(beg + 0x120), (uint16_t*)(beg + 0x180));
    dst.erase((uint16_t*)(beg + 0x0A0), (uint16_t*)(beg + 0x0E0));

    for (size_t i = 0; i < dst.size(); ++i)
        qd::swapBytes_<2>(&dst[i]);
    ::restore_custom((uae_u8*)dst.data());
}


amD::AddrRef UaeVmImp::Copper::getCopperAddr(CopperAddr_ copno) {
    return ::get_copper_address(copno);
}


void UaeVmImp::Copper::fetch() {
}


void UaeVmImp::Emu::setDebugMode(DebuggerMode debug_mode) {
    if (debug_mode == DebuggerMode_Break) {
        while (!isDebugActivatedFull()) {
            ::debugger_active = 0;
            ::debugging = 0;
            ::activate_debugger_new();
        }
    } else if (debug_mode == DebuggerMode_Live) {
        amD::operation::args::DoDebugTraceContinue m;
        vm->applyOperationProc(&m);
        ::debugger_active = 0;
    }
}

bool UaeVmImp::Emu::isDebugActivated() const {
    return ::debugging > 0 && (::debugger_active > 0);
}

bool UaeVmImp::Emu::isDebugActivatedFull() const {
    return ::debugging > 0 && (::debugger_active > 0 && ::regs.spcflags & SPCFLAG_BRK);
}


};  // namespace vm::imp
//////////////////////////////////////////////////////////////////////////


static bool uae_bp_reg_convert(int uae_reg, EReg& out) {
    if (uae_reg >= amD::breakpoint_reg_end)
        return false;
    out = (EReg::Type)uae_reg;
    return true;
}


void BreakpointsSortedList::init() {
    static_assert(amD::BREAKPOINTS_MAX == BREAKPOINT_TOTAL);

    mBreakpoints.clear();
    for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
        const ::breakpoint_node& uaeCurBrpt = ::bpnodes[i];
        if (uaeCurBrpt.value1 == 0 || uaeCurBrpt.enabled <= 0)
            continue;
        amD::Breakpoint& curBp = mBreakpoints.emplace_back();
        curBp.addr1 = uaeCurBrpt.value1;
        curBp.addr2 = uaeCurBrpt.value2;
        curBp.enabled = uaeCurBrpt.enabled;
        amD::uae_bp_reg_convert(uaeCurBrpt.type, curBp.reg);

        if (uaeCurBrpt.oper == BREAKPOINT_CMP_EQUAL && uaeCurBrpt.enabled > 0) {
            OneAddrBp bp;
            bp.addr = curBp.addr1;
            bp.bpIdx = (int)mBreakpoints.size() - 1;
            mOneAddrBps.insert(bp);
        }
    }
}


void* impFactoryCreateInstance(const std::type_info& type) {
    if (type == typeid(amD::AbsVM)) {
        return new amD::vm::imp::UaeVmImp();
    }
    UNIMPLEMENTED();
    return nullptr;
}


};  //namespace amD
