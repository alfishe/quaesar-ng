#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0  // fix 'byte': ambiguous symbol

#include "uae_vm_imp.h"
// clang-format off
#include <sysconfig.h>
#include <uae_lib/include/sysdeps.h>
#include <uae_lib/include/options.h>
#include <uae_lib/include/memory.h>
#include <uae_lib/include/newcpu.h>
#include <uae_lib/include/mmu_common.h>
#include <uae_lib/include/cpummu.h>
#include <uae_lib/include/cpummu030.h>
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
#include <set>

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
        if (pUae) pUae->execConsoleCmd("t");

    } else if (args->cast_<amD::operation::DebugTraceStart>()) {
        r = true;
        if (vm->getVmDebugMode() == EVmDebugMode::Live)
            vm->setVmDebugMode(EVmDebugMode::Break);
        else
            vm->setVmDebugMode(EVmDebugMode::Live);

    } else if (args->cast_<amD::operation::DisasmTraceStepOut>()) {
        r = true;
        if (pUae) pUae->execConsoleCmd("z");

    } else if (args->cast_<amD::operation::CopperTraceStep>()) {
        r = true;
        if (pUae) pUae->execConsoleCmd("ot");

    } else if (auto p = args->cast_<amD::operation::DisasmToggleBreakpoint>()) {
        if (!pUae) return EFlow::NO_RESULT;
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

    } else if (args->cast_<amD::operation::PauseEmulation>()) {
        r = true;
        vm->setVmDebugMode(EVmDebugMode::Break);

    } else if (args->cast_<amD::operation::VmEmuReset>()) {
        r = true;
        ::uae_reset(1, 1);

    } else if (auto p = args->cast_<amD::operation::CopperToggleBreakpoint>()) {
        if (!pUae) return EFlow::NO_RESULT;
        r = true;
        qtd::string cmd = qd::string_format("ob %08x", (uint32_t)p->address);
        pUae->execConsoleCmd(std::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (auto p = args->cast_<amD::operation::DebugWaitScanLines>()) {
        if (!pUae) return EFlow::NO_RESULT;
        qtd::string cmd = qd::string_format("fs %i", p->waitScanLines);
        pUae->execConsoleCmd(std::move(cmd));
        return qd::EFlow::SUCCESS;
    } else if (args->cast_<qsr::operations::QuitQuasarApp>()) {
        ::quit_program = UAE_QUIT;
        r = true;
    } else if (args->cast_<amD::operation::VmPlayerWndAlwaysOnTop>()) {
        r = true;
    }
    return r ? EFlow::STOP : EFlow::NO_RESULT;
}


// getScreenPixBuf — lock-free emulator framebuffer snapshot for the debugger.
//
// Returns m_pAmigaBuffer: a stable snapshot that the emulator writes at each
// vsync boundary (see UaeServerThread::_lockUaeScreenTexBuf / _unlockUaeScreenTexBuf).
//
// The caller (debugger ScreenWnd) reads this WITHOUT acquiring m_UaeScrTextureMutex.
// This is intentional and correct for a read-only consumer: reading the snapshot
// mid-update may cause a harmless single-scanline tear, which is acceptable for
// a debugger preview. Avoiding the mutex eliminates all contention with the main
// window render path and the emulator thread.
//
// History: previously this returned raw vb->bufmem (UAE core's live render
// buffer), which had worse tearing because the emulator writes to it
// continuously throughout frame rendering, not just at vsync boundaries.
void* UaeVmImp::Blitter::getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) {
    UaeServerThread* pThread = UaeServerThread::get();
    if (!pThread || !pThread->m_pAmigaBuffer)
        return nullptr;
    *out_size_w = pThread->m_scrWidth;
    *out_size_h = pThread->m_scrHeight;
    *pitch = pThread->m_scrWidth * (int)sizeof(uint32_t);
    return pThread->m_pAmigaBuffer;
}


bool UaeVmImp::Blitter::isBlitterActive() const {
    return blt_info.blit_main || blt_info.blit_finald || blt_info.blit_queued;
}


void UaeVmImp::CustomRegs::fetch() {
    // Read custom register values directly from UAE's internal
    // custom_storage[] array. This avoids memory_get_word() which
    // dispatches through the custom bank and triggers hardware side
    // effects (event scheduling, interrupt clears, etc.) causing
    // "out of event2's!" crashes when called from the UI thread.
    regsData[0] = 0;
    regsData[1] = 0;
    for (int i = 0; i < IVm::CustReg::_COUNT_; ++i) {
        uint32_t addr = IVm::CustReg::cust_reg_data[i].addr;
        regsData[i + data_offset] = ::custom_storage[(addr & 0x1FE) >> 1].value;
    }
}


void UaeVmImp::CustomRegs::commit() {
    // Write back via memory_put_word which properly dispatches through
    // UAE's custom register write path (only safe when emulator is paused).
    for (int i = 0; i < IVm::CustReg::_COUNT_; ++i) {
        uint32_t addr = IVm::CustReg::cust_reg_data[i].addr;
        ::memory_put_word(addr, regsData[i + data_offset]);
    }
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
        if (m_pUaeThread) {
            ::activate_debugger_new_pc(0, 0xFFFFFFFF);
        }
    } else if (debug_mode == EVmDebugMode::Live) {
        if (m_pUaeThread)
            m_pUaeThread->execConsoleCmd("g");
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


int UaeVmImp::getChipsetLevel() const {
    if (::currprefs.chipset_mask & CSMASK_AGA)
        return 2;  // AGA (A1200/A4000): Alice + Lisa
    if (::currprefs.chipset_mask & (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE))
        return 1;  // ECS (A500+/A600/A3000): Super Agnus + Super Denise
    return 0;       // OCS (A1000/A500/A2000): Agnus + Denise
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


// Snapshot all CPU registers from the live UAE ::regs global.
// Called by fetchVmState() at the throttled rate (~15fps).
// Getters below read from this snapshot, so widgets see stable values
// between fetches instead of flickering at the emulator's execution rate.
void UaeVmImp::Cpu::fetch() {
    for (int i = 0; i < 8; i++) {
        snap_regs_d[i] = m68k_dreg(::regs, i);
        snap_regs_a[i] = m68k_areg(::regs, i);
    }
    snap_pc = ::regs.instruction_pc;
    snap_intmask = ::regs.intmask;
    snap_flg_z = GET_ZFLG();
    snap_flg_c = GET_CFLG();
    snap_flg_v = GET_VFLG();
    snap_flg_n = GET_NFLG();
    snap_flg_x = GET_XFLG();
}


bool UaeVmImp::Cpu::getFlg(ECpuFlg_ f) const {
    switch (f) {
        case IVm::CpuFlg_Z:
            return snap_flg_z;
        case IVm::CpuFlg_C:
            return snap_flg_c;
        case IVm::CpuFlg_V:
            return snap_flg_v;
        case IVm::CpuFlg_N:
            return snap_flg_n;
        case IVm::CpuFlg_X:
            return snap_flg_x;
        default:
            return false;
    }
}


uint32_t UaeVmImp::Cpu::getRegA(int i) const {
    return snap_regs_a[i];
}


uint32_t UaeVmImp::Cpu::getRegD(int i) const {
    return snap_regs_d[i];
}


AddrRef UaeVmImp::Cpu::getPC() const {
    return snap_pc;
}


int UaeVmImp::Cpu::getIntMask() const {
    return snap_intmask;
}


bool UaeVmImp::Cpu::isMmuEnabled() const {
    if (::currprefs.mmu_model == 0) return false;
    if (::currprefs.mmu_model == 68030) {
        return (tc_030 & 0x80000000) != 0;
    }
    return ::regs.mmu_enabled != 0;
}

int UaeVmImp::Cpu::getCpuModel() const {
    return ::currprefs.cpu_model;
}

void UaeVmImp::Cpu::getMmuPages(qtd::vector<MmuPage>& outPages, MmuStats* outStats) const {
    if (!isMmuEnabled()) return;
    
    // Safety check - avoid running if CPU isn't 68030/040/060
    if (::currprefs.cpu_model < 68030) return;

    std::set<uaecptr> seenPtrTables;
    std::set<uaecptr> seenPageTables;

    auto walk_table = [&](uaecptr root_ptr, bool super) {
        if (outStats) outStats->numRootTables++;

        const int ROOT_TABLE_SIZE = 128, PTR_TABLE_SIZE = 128, PAGE_TABLE_SIZE = 64;
        const int ROOT_INDEX_SHIFT = 25, PTR_INDEX_SHIFT = 18;

        for (int root_idx = 0; root_idx < ROOT_TABLE_SIZE; root_idx++) {
            uae_u32 root_des = ::x_phys_get_long(root_ptr + root_idx * 4);
            if ((root_des & 2) == 0) continue;

            uaecptr root_log = root_idx << ROOT_INDEX_SHIFT;
            uaecptr ptr_des_addr = root_des & MMU_ROOT_PTR_ADDR_MASK;

            if (outStats && seenPtrTables.insert(ptr_des_addr).second) {
                outStats->numPtrTables++;
            }

            for (int ptr_idx = 0; ptr_idx < PTR_TABLE_SIZE; ptr_idx++) {
                uae_u32 ptr_des = ::x_phys_get_long(ptr_des_addr + ptr_idx * 4);
                if ((ptr_des & 2) == 0) continue;

                uaecptr ptr_log = root_log | (ptr_idx << PTR_INDEX_SHIFT);
                uaecptr page_addr = ptr_des & (::mmu_pagesize_8k ? MMU_PTR_PAGE_ADDR_MASK_8 : MMU_PTR_PAGE_ADDR_MASK_4);

                if (outStats && seenPageTables.insert(page_addr).second) {
                    outStats->numPageTables++;
                }

                for (int page_idx = 0; page_idx < PAGE_TABLE_SIZE; page_idx++) {
                    uae_u32 page_des = ::x_phys_get_long(page_addr + page_idx * 4);
                    if ((page_des & 3) == 0) continue;
                    if ((page_des & 3) == 2) {
                        uae_u32 indirect_addr = page_des & MMU_PAGE_INDIRECT_MASK;
                        page_des = ::x_phys_get_long(indirect_addr);
                        if ((page_des & 3) == 0) continue;
                    }

                    uaecptr page_log = ptr_log | (page_idx << (::mmu_pagesize_8k ? 13 : 12));
                    
                    MmuPage mp;
                    mp.logical = page_log;
                    mp.physical = page_des & (::mmu_pagesize_8k ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
                    mp.size = ::mmu_pagesize_8k ? 8192 : 4096;
                    mp.flags = page_des;
                    mp.cacheable = ((page_des & MMU_TTR_CACHE_MASK) >> MMU_TTR_CACHE_SHIFT) != 1;
                    mp.writeProtected = (page_des & MMU_DES_WP) != 0;
                    mp.superOnly = (page_des & MMU_DES_SUPER) != 0 || super;
                    mp.modified = (page_des & MMU_DES_MODIFIED) != 0;
                    
                    outPages.push_back(mp);
                }
            }
        }
    };

    walk_table(::regs.urp, false);
    if (::regs.srp != ::regs.urp) {
        walk_table(::regs.srp, true);
    }

    if (outStats) {
        outStats->totalMemoryBytes = (outStats->numRootTables * 128 * 4) +
                                     (outStats->numPtrTables * 128 * 4) +
                                     (outStats->numPageTables * 64 * 4);
    }
}


uint8_t* UaeVmImp::Memory::getRealAddr(AddrRef ptr) {
    return (uint8_t*)::memory_get_real_address(ptr);
}


bool UaeVmImp::Memory::getU16(AddrRef addr, uint16_t* out) {
    *out = (uint16_t)::memory_get_word(addr);
    return true;
}


uint16_t UaeVmImp::Memory::getU16(AddrRef addr) {
    return (uint16_t)::memory_get_word(addr);
}


uint8_t UaeVmImp::Memory::getU8(AddrRef addr) {
    return (uint8_t)::memory_get_byte(addr);
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
