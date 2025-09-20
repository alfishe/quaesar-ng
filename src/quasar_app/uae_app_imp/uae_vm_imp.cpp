#include "uae_vm_imp.h"
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
#include <uae.h>
#include <events.h>
#undef byte
#undef D
#undef bug
// clang-format on
#include <SDL_log.h>
#include "amDebugger/debuggerOps.h"
#include "amDebugger/debuggerWndApp.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/endian.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include "qd/typeSystem/typeInfo.h"
#include "quasar_app/quaesar.h"
#include "uae_server_thread.h"


extern int vpos;
extern bool get_custom_color_reg(int colreg, uae_u8* r, uae_u8* g, uae_u8* b);
extern uaecptr bplpt[MAX_PLANES], bplptx[MAX_PLANES];


namespace amD {
namespace vm::imp {


UaeVmImp::UaeVmImp() {
    TSuper::cpu = &instCpu;
    TSuper::mem = &instMemory;
    TSuper::custom = &instCustomRegs;
    TSuper::copper = &instCopper;
    TSuper::blitter = &instBlitter;
    {
        instEmu.vm = this;
        TSuper::emu = &instEmu;
    }
    for (size_t i = 0; i < TSuper::floppies.size(); ++i) {
        UaeVmImp::Floppy& curFloppy = instFloppies[i];
        curFloppy.m_nFloppy = (int)i;
        TSuper::floppies[i] = &curFloppy;
    }
}


void UaeVmImp::init() {
    if (mInit)
        return;
    mInit = true;
    IVm::VM* s = (IVm::VM*)(this);
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


qd::EFlow UaeVmImp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
    UaeVmImp* vm = this;
    UaeServerThread* pUae = m_pUaeThread;
    bool r = false;
    if (c_def(0)) {
    } else if (args->cast_<amD::operation::DebugTraceContinue>()) {
        r = true;
        if (vm->getVmDebugMode() == EVmDebugMode::Live)
            vm->setVmDebugMode(EVmDebugMode::Break);
        else
            vm->setVmDebugMode(EVmDebugMode::Live);

    } else if (args->cast_<amD::operation::DisasmTraceStepInto>()) {
        r = true;
        vm->setVmDebugMode(EVmDebugMode::Break);
        pUae->execConsoleCmd("t");

    } else if (args->cast_<amD::operation::DebugTraceStart>()) {
        r = true;
        if (vm->getVmDebugMode() == EVmDebugMode::Live)
            vm->setVmDebugMode(EVmDebugMode::Break);
        else
            vm->setVmDebugMode(EVmDebugMode::Live);

    } else if (args->cast_<amD::operation::DisasmTraceStepOut>()) {
        r = true;
        pUae->execConsoleCmd("z");

    } else if (args->cast_<amD::operation::CopperTraceStep>()) {
        r = true;
        pUae->execConsoleCmd("ot");

    } else if (auto p = args->cast_<amD::operation::DisasmToggleBreakpoint>()) {
        eastl::string cmd;
        cmd.sprintf("f %08x", (uint32_t)p->address);
        if (p->nBreakpoint >= 0)
            cmd.append_sprintf(" %i", p->nBreakpoint);
        pUae->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (args->cast_<amD::operation::ToggleTurboEmulation>()) {
        r = true;
        if (::currprefs.turbo_emulation != 0) {
            ::warpmode(0);  // off
        } else {
            ::warpmode(2);  // on
        }

    } else if (args->cast_<amD::operation::UaeResetAmiga>()) {
        r = true;
        ::uae_reset(1, 1);

    } else if (auto p = args->cast_<amD::operation::CopperToggleBreakpoint>()) {
        r = true;
        eastl::string cmd;
        cmd.sprintf("ob %08x", (uint32_t)p->address);
        pUae->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (auto p = args->cast_<amD::operation::DebugWaitScanLines>()) {
        eastl::string cmd;
        cmd.sprintf("fs %i", p->waitScanLines);
        pUae->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else if (args->cast_<amD::operation::UaeWndAlwaysOnTop>()) {
        r = true;
        //         if (pUae->isWndAlwaysOnTop()) {
        //             pUae->setWndAlwaysOnTop(false);
        //         } else {
        //             pUae->setWndAlwaysOnTop(true);
        //         }
    }
    return r ? EFlow::STOP : EFlow::NO_RESULT;
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


amD::AddrRef UaeVmImp::Copper::getCopperAddr(amD::ECopperAddr_ copno) {
    return ::get_copper_address(copno);
}


void UaeVmImp::Copper::fetch() {
}

int UaeVmImp::Emu::getDebugDmaMode() {
    return ::debug_dma;
}


void UaeVmImp::Emu::setDebugDmaMode(int p_mode) {
    ::debug_dma = p_mode;
}


void UaeVmImp::setVmDebugMode(EVmDebugMode debug_mode) {
    TSuper::setVmDebugMode(debug_mode);
    if (debug_mode == EVmDebugMode::Break) {
        while (!instEmu.isDebugActivatedFull()) {
            ::debugger_active = 0;
            ::debugging = 0;
            ::activate_debugger_new();
        }
    } else if (debug_mode == EVmDebugMode::Live) {
        if (m_pUaeThread)
            m_pUaeThread->execConsoleCmd("g");
        ::debugger_active = 0;
    }
}


int UaeVmImp::getVPos() {
    return ::vpos;
}


int UaeVmImp::getHPos() {
    return ::current_hpos_safe();  // ::current_hpos();
}


int UaeVmImp::getCurCycle() {
    int c = (int)((::get_cycles() - ::vsync_cycles) / CYCLE_UNIT);
    return c;
}


bool UaeVmImp::Emu::isDebugActivated() const {
    return ::debugging > 0 && (::debugger_active > 0);
}

bool UaeVmImp::Emu::isDebugActivatedFull() const {
    return ::debugging > 0 && (::debugger_active > 0 && ::regs.spcflags & SPCFLAG_BRK);
}


bool UaeVmImp::Floppy::getEnabled() {
    ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    return cfgFloppy.dfxtype >= 0;
}


void UaeVmImp::Floppy::setEnabled(bool v) {
    ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    cfgFloppy.dfxtype = v ? 0 : -1;
}


bool UaeVmImp::Cpu::getFlg(ECpuFlg_ f) const {
    switch (f) {
        case amD::CpuFlg_Z:
            return GET_ZFLG();
        case amD::CpuFlg_C:
            return GET_CFLG();
        case amD::CpuFlg_V:
            return GET_VFLG();
        case amD::CpuFlg_N:
            return GET_NFLG();
        case amD::CpuFlg_X:
            return GET_XFLG();
        default:
            return false;
    }
}


};  // namespace vm::imp
//////////////////////////////////////////////////////////////////////////


static bool uae_bp_reg_convert(int uae_reg, amD::EReg& out) {
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

};  //namespace amD
//////////////////////////////////////////////////////////////////////////


void* IVm::impFactoryCreateInstance(const std::type_info& type) {
    if (type == typeid(IVm::VM)) {
        return new amD::vm::imp::UaeVmImp();
    }
    UNIMPLEMENTED();
    return nullptr;
}
