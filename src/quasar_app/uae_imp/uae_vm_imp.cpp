#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0  // fix 'byte': ambiguous symbol

#include "uae_vm_imp.h"
// clang-format off
#include <sysconfig.h>
#include <uae_lib/include/sysdeps.h>
#include <uae_lib/include/options.h>
#include <uae_lib/include/memory.h>
#include <uae_lib/include/newcpu.h>
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
#include "SDL_stdinc.h"  // strlcpy
#include "amDebugger/debuggerOps.h"
#include "amDebugger/debuggerWndApp.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/endian.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include "qd/typeSystem/typeInfo.h"
#include "qsr_operations.h"
#include "quasar_app/quaesar.h"
#include "uae_server_thread.h"


extern int vpos;
extern bool get_custom_color_reg(int colreg, uae_u8* r, uae_u8* g, uae_u8* b);
extern uaecptr bplpt[MAX_PLANES], bplptx[MAX_PLANES];


namespace IVm::imp {


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
    for (size_t i = 0; i < IVm::MAX_FLOPPIES; ++i) {
        UaeVmImp::Floppy& curFloppy = instFloppies[i];
        curFloppy.m_nFloppy = (int)i;
        (&floppy0)[i] = &curFloppy;
    }
}


void UaeVmImp::init() {
    TSuper::init();
}


UaeVmImp::~UaeVmImp() {
}


qd::EFlow UaeVmImp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
    EA_DISABLE_VC_WARNING(4456) /*declaration of 'x' hides previous local declaration*/
    UaeVmImp* vm = this;
    UaeServerThread* pUae = m_pUaeThread;
    // No live UAE thread bound (e.g. dummy debugger connection before emulator is started).
    // All operations below talk to live UAE state, so bail out cleanly instead of crashing.
    if (!pUae)
        return EFlow::NO_RESULT;
    bool r = false;
    if (qd::c_def(0)) {
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
        qtd::string cmd = qd::string_format("f %08x", (uint32_t)p->address);
        if (p->nBreakpoint >= 0)
            cmd += qd::string_format(" %i", p->nBreakpoint);
        pUae->execConsoleCmd(std::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (args->cast_<amD::operation::ToggleTurboEmulation>()) {
        r = true;
        if (::currprefs.turbo_emulation != 0) {
            ::warpmode(0);  // off
        } else {
            ::warpmode(2);  // on
        }

    } else if (args->cast_<amD::operation::VmEmuReset>()) {
        r = true;
        ::uae_reset(1, 1);

    } else if (auto p = args->cast_<amD::operation::CopperToggleBreakpoint>()) {
        r = true;
        qtd::string cmd = qd::string_format("ob %08x", (uint32_t)p->address);
        pUae->execConsoleCmd(std::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (auto p = args->cast_<amD::operation::DebugWaitScanLines>()) {
        qtd::string cmd = qd::string_format("fs %i", p->waitScanLines);
        pUae->execConsoleCmd(std::move(cmd));
        return qd::EFlow::SUCCESS;
    } else if (args->cast_<qsr::operations::QuitQuasarApp>()) {
        ::quit_program = UAE_QUIT;
        r = true;
    } else if (args->cast_<amD::operation::VmPlayerWndAlwaysOnTop>()) {
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


amD::AddrRef UaeVmImp::Copper::getCopperAddr(IVm::ECopperAddr_ copno) {
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


bool UaeVmImp::Floppy::getWriteProtect() {
    ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    return cfgFloppy.forcedwriteprotect;
}


void UaeVmImp::Floppy::setWriteProtect(bool v) {
    ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    cfgFloppy.forcedwriteprotect = v;
}


qtd::string UaeVmImp::Floppy::getAdfPath() {
    ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    return cfgFloppy.df;
}


void UaeVmImp::Floppy::setAdfPath(const qtd::string& v) {
    ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    SDL_strlcpy(cfgFloppy.df, v.c_str(), sizeof(cfgFloppy.df));
}


bool UaeVmImp::Cpu::getFlg(ECpuFlg_ f) const {
    switch (f) {
        case IVm::CpuFlg_Z:
            return GET_ZFLG();
        case IVm::CpuFlg_C:
            return GET_CFLG();
        case IVm::CpuFlg_V:
            return GET_VFLG();
        case IVm::CpuFlg_N:
            return GET_NFLG();
        case IVm::CpuFlg_X:
            return GET_XFLG();
        default:
            return false;
    }
}


uint32_t UaeVmImp::Cpu::getRegA(int i) const {
    return m68k_areg(::regs, i);
}


uint32_t UaeVmImp::Cpu::getRegD(int i) const {
    return m68k_dreg(regs, i);
}


AddrRef UaeVmImp::Cpu::getPC() const {
    return m68k_getpc();
}


int UaeVmImp::Cpu::getIntMask() const {
    return regs.intmask;
}


uint8_t* UaeVmImp::Memory::getRealAddr(AddrRef ptr) {
    // Safety: validate the address is in an allocated memory bank before
    // calling memory_get_real_address(), which dereferences mem_banks[].
    // If UAE memory hasn't been initialized yet (mem_banks[] are NULL),
    // or the address maps to a bank without allocated memory, return nullptr.
    const uint32_t addr = (uint32_t)ptr;
    const uint32_t bankIdx = addr >> 16;
    if (bankIdx >= MEMORY_BANKS)
        return nullptr;
    addrbank* ab = mem_banks[bankIdx];
    if (!ab)
        return nullptr;
    // Check if the bank has allocated memory
    if (!ab->allocated_size)
        return nullptr;
    return (uint8_t*)::memory_get_real_address(ptr);
}


bool UaeVmImp::Memory::getU16(AddrRef addr, uint16_t* out) {
    *out = (uint16_t)::memory_get_word(addr);
    return true;
}


uint16_t UaeVmImp::Memory::getU16(AddrRef addr) {
    return (uint16_t)::memory_get_word(addr);
}


void UaeVmImp::Memory::setU16(AddrRef addr, uint16_t v) {
    ::memory_put_word(addr, v);
}


uint32_t UaeVmImp::Memory::getU32(AddrRef addr) {
    return (uint32_t)::memory_get_long(addr);
}


void UaeVmImp::Memory::setU32(AddrRef addr, uint32_t v) {
    ::memory_put_long(addr, v);
}


void fill_bank(IVm::Memory* ivmem, EMemSrc id, const ::addrbank& pUaeBank) {
    IVm::MemBank& memBank = ivmem->m_banks[id];
    memBank.m_id = id;
    if (!pUaeBank.allocated_size) {
        return;
    }
    memBank.m_name = pUaeBank.name;
    memBank.m_label = pUaeBank.label;
    memBank.m_startAddr = pUaeBank.start;
    memBank.m_realAddr = pUaeBank.baseaddr;
    memBank.m_mask = pUaeBank.mask;
    memBank.m_size = pUaeBank.allocated_size;
}


void UaeVmImp::Memory::init(IVm::VM*) {
    fill_bank(this, EMemSrc::ROM, ::kickmem_bank);
    fill_bank(this, EMemSrc::CHIP, ::chipmem_bank);
    fill_bank(this, EMemSrc::CUSTOM, ::custom_bank);
    fill_bank(this, EMemSrc::SLOW, ::bogomem_bank);
}


static bool uae_bp_reg_convert(int uae_reg, IVm::EReg& out) {
    if (uae_reg >= IVm::breakpoint_reg_end)
        return false;
    out = (EReg::Type)uae_reg;
    return true;
}


void UaeVmImp::Emu::initBreakPoints(amD::BreakpointsSortedList& bpList) {
    static_assert(amD::BREAKPOINTS_MAX == BREAKPOINT_TOTAL);

    bpList.mBreakpoints.clear();
    for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
        const ::breakpoint_node& uaeCurBrpt = ::bpnodes[i];
        if (uaeCurBrpt.value1 == 0 || uaeCurBrpt.enabled <= 0)
            continue;
        amD::Breakpoint& curBp = bpList.mBreakpoints.emplace_back();
        curBp.addr1 = uaeCurBrpt.value1;
        curBp.addr2 = uaeCurBrpt.value2;
        curBp.enabled = uaeCurBrpt.enabled;
        uae_bp_reg_convert(uaeCurBrpt.type, curBp.reg);

        if (uaeCurBrpt.oper == BREAKPOINT_CMP_EQUAL && uaeCurBrpt.enabled > 0) {
            amD::BreakpointsSortedList::OneAddrBp bp;
            bp.addr = curBp.addr1;
            bp.bpIdx = (int)bpList.mBreakpoints.size() - 1;
            bpList.mOneAddrBps.insert(bp);
        }
    }
}


};  // namespace IVm::imp
//////////////////////////////////////////////////////////////////////////

void* IVm::impFactoryCreateInstance(const std::type_info& type) {
    if (type == typeid(IVm::VM)) {
        return new IVm::imp::UaeVmImp();
    }
    UNIMPLEMENTED();
    //return nullptr;
}
