#include "va_vm_imp.h"
//clang-format off
#include "Aliases.h"
#include "Amiga.h"
#include "CPUTypes.h"
#include "Emulator.h"
#include "Memory/Memory.h"
//clang-format on
#include <SDL_log.h>

#include "amDebugger/debuggerOps.h"
#include "amDebugger/debuggerWndApp.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/endian.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include "qd/typeSystem/typeInfo.h"
#include "va_server_thread.h"

#undef main

class QuaesarVAmigaInjectAccess {
 public:
  vamiga::VAmiga* m_pVAmiga = nullptr;
  vamiga::Amiga* main = &m_pVAmiga->emu->main;

 public:
  QuaesarVAmigaInjectAccess(vamiga::VAmiga* pVAmiga) : m_pVAmiga(pVAmiga) {}

};  // class QuaesarVAmigaInjectAccess
//////////////////////////////////////////////////////////////////////////

namespace IVm::imp {

VAmVmImp::VAmVmImp(VAmServerThread* pVAmThread, vamiga::VAmiga* pVAmiga) {
  m_pVAmThread = pVAmThread;
  m_vAmiga = pVAmiga;
  m_vaAccess = new QuaesarVAmigaInjectAccess(m_vAmiga);
  main = m_vaAccess->main;

  instCpu.m_pVm = this;
  instCpu.m_pVAmiga = m_vAmiga;
  instMemory.m_pVm = this;
  instMemory.m_pVAmiga = m_vAmiga;
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
    VAmVmImp::Floppy& curFloppy = instFloppies[i];
    curFloppy.m_nFloppy = (int)i;
    (&floppy0)[i] = &curFloppy;
  }
}

void VAmVmImp::init() { TSuper::init(); }

VAmVmImp::~VAmVmImp() { SAFE_DELETE(m_vaAccess); }

qd::EFlow VAmVmImp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
  EA_DISABLE_VC_WARNING(
      4456) /*declaration of 'x' hides previous local declaration*/
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
    if (p->nBreakpoint >= 0) cmd.append_sprintf(" %i", p->nBreakpoint);
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

void* VAmVmImp::Blitter::getScreenPixBuf(int mon_id, int* out_size_w,
                                         int* out_size_h, int* pitch) {
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
  return false;  // blt_info.blit_main || blt_info.blit_finald ||
                 // blt_info.blit_queued;
}

void VAmVmImp::CustomRegs::fetch() {
  //     size_t dump_len;
  //     ::save_custom(&dump_len, (uae_u8*)regsData.data(), 1);
  //     for (size_t i = 0; i < regsData.size(); ++i)
  //         qd::swapBytes_<2>(&regsData[i]);
}

void VAmVmImp::CustomRegs::commit() {
  //     eastl::fixed_vector<uint16_t, CustReg::_COUNT_ + data_offset, false>
  //     dst = {regsData.begin(), regsData.end()}; uint8_t* beg =
  //     (uint8_t*)dst.begin(); dst.erase((uint16_t*)(beg + 0x120),
  //     (uint16_t*)(beg + 0x180)); dst.erase((uint16_t*)(beg + 0x0A0),
  //     (uint16_t*)(beg + 0x0E0));
  //
  //     for (size_t i = 0; i < dst.size(); ++i)
  //         qd::swapBytes_<2>(&dst[i]);
  //     ::restore_custom((uae_u8*)dst.data());
}

amD::AddrRef VAmVmImp::Copper::getCopperAddr(IVm::ECopperAddr_ copno) {
  return 0;  // ::get_copper_address(copno);
}

void VAmVmImp::Copper::fetch() {}

int VAmVmImp::Emu::getDebugDmaMode() {
  return 0;  // ::debug_dma;
}

void VAmVmImp::Emu::setDebugDmaMode(int p_mode) {
  //::debug_dma = p_mode;
}

void VAmVmImp::setVmDebugMode(EVmDebugMode debug_mode) {
  TSuper::setVmDebugMode(debug_mode);
//   if (debug_mode == EVmDebugMode::Break) {
//     while (!instEmu.isDebugActivatedFull()) {
//       //             ::debugger_active = 0;
//       //             ::debugging = 0;
//       //             ::activate_debugger_new();
//     }
//   } else if (debug_mode == EVmDebugMode::Live) {
//     if (m_pVAmThread) m_pVAmThread->execConsoleCmd("g");
//     //        ::debugger_active = 0;
//   }
}

int VAmVmImp::getVPos() { return 0; }

int VAmVmImp::getHPos() {
  return 0;  // ::current_hpos_safe();  // ::current_hpos();
}

int VAmVmImp::getCurCycle() {
  // int c = (int)((::get_cycles() - ::vsync_cycles) / CYCLE_UNIT);
  return 0;  // c;
}

bool VAmVmImp::Emu::isDebugActivated() const {
  return 0;  //::debugging > 0 && (::debugger_active > 0);
}

bool VAmVmImp::Emu::isDebugActivatedFull() const {
  return 0;  //::debugging > 0 && (::debugger_active > 0 && ::regs.spcflags &
             //: SPCFLAG_BRK);
}

bool VAmVmImp::Floppy::getEnabled() {
  //::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
  return 0;  // cfgFloppy.dfxtype >= 0;
}

void VAmVmImp::Floppy::setEnabled(bool v) {
  //     ::floppyslot& cfgFloppy = ::changed_prefs.floppyslots[m_nFloppy];
  //     cfgFloppy.dfxtype = v ? 0 : -1;
}


void VAmVmImp::Floppy::setAdfPath(const qd::string &v)
{
    vamiga::FloppyDriveAPI *df = m_pVm->m_vAmiga->df[m_nFloppy];
    df->insert(v.c_str(), m_writeProtect);
}


qd::string VAmVmImp::Floppy::getAdfPath()
{
    //vamiga::FloppyDriveAPI *df = m_pVm->m_vAmiga->df[m_nFloppy];
    return "";
}


void VAmVmImp::Floppy::init(IVm::VM *vm)
{
    m_pVm = static_cast<VAmVmImp *>(vm);
}


bool VAmVmImp::Cpu::getFlg(ECpuFlg_ f) const {
  switch (f) {
    case IVm::CpuFlg_Z:
      return (m_pCpuInfo->sr >> 2) & 1;
    case IVm::CpuFlg_C:
      return (m_pCpuInfo->sr >> 0) & 1;
    case IVm::CpuFlg_V:
      return (m_pCpuInfo->sr >> 1) & 1;
    case IVm::CpuFlg_N:
      return (m_pCpuInfo->sr >> 3) & 1;
    case IVm::CpuFlg_X:
      return (m_pCpuInfo->sr >> 4) & 1;
    default:
      return false;
  }
}

uint8_t* VAmVmImp::Memory::getRealAddr(AddrRef ptr) {
  vamiga::Memory& mem = m_pVm->main->mem;
  uint32_t addr = (ptr & 0xFFFFFF);
  vamiga::MemSrc memBank = mem.cpuMemSrc[addr >> 16];
  switch (memBank) {
    case vamiga::MemSrc::NONE:
      return nullptr;
    case vamiga::MemSrc::CHIP:
    case vamiga::MemSrc::CHIP_MIRROR:
      return mem.chip;
    case vamiga::MemSrc::SLOW:
    case vamiga::MemSrc::SLOW_MIRROR:
      return mem.slow;
    case vamiga::MemSrc::FAST:
      return mem.fast;
    case vamiga::MemSrc::CUSTOM:
    case vamiga::MemSrc::CUSTOM_MIRROR:
      return mem.chip;
    case vamiga::MemSrc::ROM:
    case vamiga::MemSrc::ROM_MIRROR:
      return mem.rom;
    case vamiga::MemSrc::WOM:
      return mem.wom;
    case vamiga::MemSrc::EXT:
      return mem.ext;
    case vamiga::MemSrc::CIA:
    case vamiga::MemSrc::CIA_MIRROR:
    case vamiga::MemSrc::RTC:
    case vamiga::MemSrc::AUTOCONF:
    case vamiga::MemSrc::ZOR:
    default:
      return nullptr;
  }
}

void VAmVmImp::Memory::init(IVm::VM* p_vm) {
  m_pVm = (VAmVmImp*)p_vm;
  vamiga::VAmiga* pVAmiga = m_pVm->m_vAmiga;
  const vamiga::MemInfo& memInfo = pVAmiga->mem.getInfo();
  const vamiga::MemConfig& memCfg = pVAmiga->mem.getConfig();

  m_banks = {};
  m_banks[EMemSrc::CHIP] = {
      .m_id = EMemSrc::CHIP,
      .m_size = (uint32_t)memCfg.chipSize,
      .m_mask = memInfo.chipMask,
      .m_name = "Chip RAM",
      .m_label = "Chip RAM",
  };

  m_banks[EMemSrc::ROM] = {
      .m_id = EMemSrc::ROM,
      .m_size = (uint32_t)memCfg.romSize,
      .m_mask = memInfo.romMask,
      .m_name = "ROM",
      .m_label = "ROM",
  };

  m_banks[EMemSrc::SLOW] = {.m_id = EMemSrc::SLOW,
                            .m_size = (uint32_t)memCfg.slowSize,
                            .m_mask = (uint32_t)memCfg.slowSize - 1,
                            .m_name = "Slow RAM",
                            .m_label = "Slow RAM",
                            .m_startAddr = SLOW_RAM_STRT};

  m_banks[EMemSrc::FAST] = {
      .m_id = EMemSrc::FAST,
      .m_size = (uint32_t)memCfg.fastSize,
      .m_mask = (uint32_t)memCfg.fastSize - 1,
      .m_name = "Fast RAM",
      .m_label = "Fast RAM",
  };

  m_banks[EMemSrc::EXT] = {
      .m_id = EMemSrc::EXT,
      .m_size = (uint32_t)memCfg.extSize,
      .m_mask = (uint32_t)memInfo.extMask,
      .m_name = "Extended RAM",
      .m_label = "Extended RAM",
  };

  m_banks[EMemSrc::WOM] = {
      .m_id = EMemSrc::WOM,
      .m_size = (uint32_t)memCfg.womSize,
      .m_mask = (uint32_t)memInfo.womMask,
      .m_name = "WOM Memory",
      .m_label = "Write-Only Memory",
  };

  // fill RealAddr and StartAddr for each bank
  for (uint32_t i = 0; i < 0xFF; ++i) {

    vamiga::MemSrc src = memInfo.cpuMemSrc[i];
    if (src == vamiga::MemSrc::NONE) continue;
    AddrRef addr = (i << 16u);
    EMemSrc bankId = (EMemSrc)(int)src;
    IVm::MemBank& curBank = m_banks[bankId];
    curBank.m_id = bankId;
    if (curBank.m_bEnabled) continue;
    curBank.m_startAddr = addr;
    curBank.m_realAddr = getRealAddr(addr);
    curBank.m_bEnabled = true;

  }
}

};  // namespace IVm::imp
//////////////////////////////////////////////////////////////////////////

// void* IVm::impFactoryCreateInstance(const std::type_info& type) {
//     if (type == typeid(IVm::VM)) {
//         return new amD::vm::imp::VAmVmImp();
//     }
//     //UNIMPLEMENTED();
//     return nullptr;
// }
