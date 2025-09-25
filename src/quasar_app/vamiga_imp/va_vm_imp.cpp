#include "va_vm_imp.h"
#include <SDL_log.h>
#include "amDebugger/debuggerOps.h"
#include "amDebugger/debuggerWndApp.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/endian.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include "qd/typeSystem/typeInfo.h"
#include "quasar_app/quaesar.h"
#include "va_server_thread.h"


namespace amD {
namespace vm::imp {


VAmVmImp::VAmVmImp() {
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
        VAmVmImp::Floppy& curFloppy = instFloppies[i];
        curFloppy.m_nFloppy = (int)i;
        TSuper::floppies[i] = &curFloppy;
    }
}


void VAmVmImp::init() {
    if (mInit)
        return;
    mInit = true;

#if 0
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
#endif  //
}


VAmVmImp::~VAmVmImp() {
}


qd::EFlow VAmVmImp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
    EA_DISABLE_VC_WARNING(4456) /*declaration of 'x' hides previous local declaration*/
    VAmVmImp* vm = this;
    VAmServerThread* pVAm = m_pVAmThread;
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
        pVAm->execConsoleCmd("t");

    } else if (args->cast_<amD::operation::DebugTraceStart>()) {
        r = true;
        if (vm->getVmDebugMode() == EVmDebugMode::Live)
            vm->setVmDebugMode(EVmDebugMode::Break);
        else
            vm->setVmDebugMode(EVmDebugMode::Live);

    } else if (args->cast_<amD::operation::DisasmTraceStepOut>()) {
        r = true;
        pVAm->execConsoleCmd("z");

    } else if (args->cast_<amD::operation::CopperTraceStep>()) {
        r = true;
        pVAm->execConsoleCmd("ot");

    } else if (auto p = args->cast_<amD::operation::DisasmToggleBreakpoint>()) {
        eastl::string cmd;
        cmd.sprintf("f %08x", (uint32_t)p->address);
        if (p->nBreakpoint >= 0)
            cmd.append_sprintf(" %i", p->nBreakpoint);
        pVAm->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (args->cast_<amD::operation::ToggleTurboEmulation>()) {
        r = true;
        //         if (::currprefs.turbo_emulation != 0) {
        //             ::warpmode(0);  // off
        //         } else {
        //             ::warpmode(2);  // on
        //         }

    } else if (args->cast_<amD::operation::UaeResetAmiga>()) {
        r = true;
        //::uae_reset(1, 1);

    } else if (auto p = args->cast_<amD::operation::CopperToggleBreakpoint>()) {
        r = true;
        eastl::string cmd;
        cmd.sprintf("ob %08x", (uint32_t)p->address);
        pVAm->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;

    } else if (auto p = args->cast_<amD::operation::DebugWaitScanLines>()) {
        eastl::string cmd;
        cmd.sprintf("fs %i", p->waitScanLines);
        pVAm->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else if (args->cast_<amD::operation::UaeWndAlwaysOnTop>()) {
        r = true;
        //         if (pVAm->isWndAlwaysOnTop()) {
        //             pVAm->setWndAlwaysOnTop(false);
        //         } else {
        //             pVAm->setWndAlwaysOnTop(true);
        //         }
    }
    return r ? EFlow::STOP : EFlow::NO_RESULT;
}


void* VAmVmImp::Blitter::getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) {
    return nullptr;
#if 0
    vidbuf_description* vidinfo = &adisplays[mon_id].gfxvidinfo;
    vidbuffer* vb = &vidinfo->drawbuffer;
    if (!vb || !vb->bufmem)
        return nullptr;
    *out_size_w = vb->outwidth;
    *out_size_h = vb->outheight;
    *pitch = vb->rowbytes;
    return vb->bufmem;
#endif  //
}


bool VAmVmImp::Blitter::isBlitterActive() const {
    return false;  //blt_info.blit_main || blt_info.blit_finald || blt_info.blit_queued;
}


void VAmVmImp::CustomRegs::fetch() {
    //     size_t dump_len;
    //     ::save_custom(&dump_len, (uae_u8*)regsData.data(), 1);
    //     for (size_t i = 0; i < regsData.size(); ++i)
    //         qd::swapBytes_<2>(&regsData[i]);
}


void VAmVmImp::CustomRegs::commit() {
    //     eastl::fixed_vector<uint16_t, CustReg::_COUNT_ + data_offset, false> dst = {regsData.begin(),
    //     regsData.end()}; uint8_t* beg = (uint8_t*)dst.begin(); dst.erase((uint16_t*)(beg + 0x120), (uint16_t*)(beg +
    //     0x180)); dst.erase((uint16_t*)(beg + 0x0A0), (uint16_t*)(beg + 0x0E0));
    //
    //     for (size_t i = 0; i < dst.size(); ++i)
    //         qd::swapBytes_<2>(&dst[i]);
    //     ::restore_custom((uae_u8*)dst.data());
}


amD::AddrRef VAmVmImp::Copper::getCopperAddr(amD::ECopperAddr_ copno) {
    return 0;  // ::get_copper_address(copno);
}


void VAmVmImp::Copper::fetch() {
}

int VAmVmImp::Emu::getDebugDmaMode() {
    return 0;  // ::debug_dma;
}


void VAmVmImp::Emu::setDebugDmaMode(int p_mode) {
    //::debug_dma = p_mode;
}


void VAmVmImp::setVmDebugMode(EVmDebugMode debug_mode) {
    TSuper::setVmDebugMode(debug_mode);
    if (debug_mode == EVmDebugMode::Break) {
        while (!instEmu.isDebugActivatedFull()) {
            //             ::debugger_active = 0;
            //             ::debugging = 0;
            //             ::activate_debugger_new();
        }
    } else if (debug_mode == EVmDebugMode::Live) {
        if (m_pVAmThread)
            m_pVAmThread->execConsoleCmd("g");
        //        ::debugger_active = 0;
    }
}


int VAmVmImp::getVPos() {
    return 0;  //::vpos;
}


int VAmVmImp::getHPos() {
    return 0;  // ::current_hpos_safe();  // ::current_hpos();
}


int VAmVmImp::getCurCycle() {
    //int c = (int)((::get_cycles() - ::vsync_cycles) / CYCLE_UNIT);
    return 0;  //c;
}


bool VAmVmImp::Emu::isDebugActivated() const {
    return 0;  //::debugging > 0 && (::debugger_active > 0);
}

bool VAmVmImp::Emu::isDebugActivatedFull() const {
    return 0;  //::debugging > 0 && (::debugger_active > 0 && ::regs.spcflags & SPCFLAG_BRK);
}


bool VAmVmImp::Floppy::getEnabled() {
    //::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    return 0;  //cfgFloppy.dfxtype >= 0;
}


void VAmVmImp::Floppy::setEnabled(bool v) {
    //     ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
    //     cfgFloppy.dfxtype = v ? 0 : -1;
}


bool VAmVmImp::Cpu::getFlg(ECpuFlg_ f) const {
    switch (f) {
        case amD::CpuFlg_Z:
            return 0;  //GET_ZFLG();
        case amD::CpuFlg_C:
            return 0;  //GET_CFLG();
        case amD::CpuFlg_V:
            return 0;  //GET_VFLG();
        case amD::CpuFlg_N:
            return 0;  //GET_NFLG();
        case amD::CpuFlg_X:
            return 0;  //GET_XFLG();
        default:
            return false;
    }
}


};  // namespace vm::imp
//////////////////////////////////////////////////////////////////////////


};  //namespace amD
//////////////////////////////////////////////////////////////////////////


// void* IVm::impFactoryCreateInstance(const std::type_info& type) {
//     if (type == typeid(IVm::VM)) {
//         return new amD::vm::imp::VAmVmImp();
//     }
//     //UNIMPLEMENTED();
//     return nullptr;
// }
