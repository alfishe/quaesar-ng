#include "va_vm_imp.h"
//clang-format off
#include "Aliases.h"
#include "Amiga.h"
#include "CPUTypes.h"
#include "Emulator.h"
#include "Memory/Memory.h"
//clang-format on
#include <SDL_log.h>
#include "qd/base/compiler.h"

#include "amDebugger/debuggerOps.h"
#include "amDebugger/debuggerWndApp.h"
#include "amDebugger/debugger.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/endian.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include "qd/typeSystem/typeInfo.h"
#include "va_server_thread.h"


class QuaesarVAmigaInjectAccess {
public:
#undef main
    vamiga::VAmiga *m_pVAmiga;
    vamiga::Amiga *main;

public:
    QuaesarVAmigaInjectAccess(vamiga::VAmiga *pVAmiga) : m_pVAmiga(pVAmiga), main(&m_pVAmiga->emu->main) {}

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
  instCustomRegs.m_pVm = this;
  TSuper::copper = &instCopper;
  instCopper.m_pVm = this;
  TSuper::blitter = &instBlitter;
  instBlitter.m_pVm = this;
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
QD_PUSH_VC_WARNING(
    4456) /*declaration of 'x' hides previous local declaration*/
VAmVmImp* vm = this;
  VAmServerThread* pVAm = m_pVAmThread;
  bool r = false;
  if constexpr (0) {
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
    qtd::string cmd = qd::string_format("f %08x", (uint32_t)p->address);
    if (p->nBreakpoint >= 0) cmd += qd::string_format(" %i", p->nBreakpoint);
    pVAm->execConsoleCmd(std::move(cmd));
    return qd::EFlow::SUCCESS;

  } else if (args->cast_<amD::operation::ToggleTurboEmulation>()) {
    r = true;
    vamiga::VAmiga* pVAmiga = vm->m_vAmiga;
    if (pVAmiga->isWarping()) {
      pVAmiga->warpOff(1);  // source=1 (non-zero, 0 is reserved for config)
    } else {
      pVAmiga->warpOn(1);
    }

  } else if (args->cast_<amD::operation::PauseEmulation>()) {
    r = true;
    vm->setVmDebugMode(EVmDebugMode::Break);

  } else if (args->cast_<amD::operation::VmEmuReset>()) {
    r = true;
    vm->m_vAmiga->hardReset();

  } else if (auto p = args->cast_<amD::operation::CopperToggleBreakpoint>()) {
    r = true;
    qtd::string cmd = qd::string_format("ob %08x", (uint32_t)p->address);
    pVAm->execConsoleCmd(std::move(cmd));
    return qd::EFlow::SUCCESS;

  } else if (auto p = args->cast_<amD::operation::DebugWaitScanLines>()) {
    qtd::string cmd = qd::string_format("fs %i", p->waitScanLines);
    pVAm->execConsoleCmd(std::move(cmd));
    return qd::EFlow::SUCCESS;
  } else if (auto p = args->cast_<amD::operation::ExecConsoleCmd>()) {
    r = true;
    pVAm->execConsoleCmd(qtd::string(p->cmd));

  } else if (args->cast_<amD::operation::VmPlayerWndAlwaysOnTop>()) {
    r = true;
    //         if (pVAm->isWndAlwaysOnTop()) {
    //             pVAm->setWndAlwaysOnTop(false);
    //         } else {
    //             pVAm->setWndAlwaysOnTop(true);
    //         }
  }
  QD_POP_VC_WARNING()
  return r ? EFlow::STOP : EFlow::NO_RESULT;
}

void* VAmVmImp::Blitter::getScreenPixBuf(int mon_id, int* out_size_w,
                                         int* out_size_h, int* pitch) {
  // Return the visible-area buffer (already channel-swapped to ARGB).
  // copyVisibleArea() in VAmServerThread extracts the displayable portion
  // from vAmiga's raw 912x313 texture and swaps R/B channels.
  VAmServerThread* pThread = m_pVm->m_pVAmThread;
  if (!pThread || !pThread->m_pAmigaBuffer) return nullptr;

  *out_size_w = pThread->m_scrWidth;
  *out_size_h = pThread->m_scrHeight;
  *pitch = pThread->m_scrWidth * sizeof(uint32_t);
  return pThread->m_pAmigaBuffer;
}

bool VAmVmImp::Blitter::isBlitterActive() const {
  return m_pVm->m_vAmiga->agnus.blitter.getInfo().bbusy;
}

void VAmVmImp::CustomRegs::fetch() {
  // Read each custom register via vAmiga's spypeek16 API
  // cust_reg_data maps each CustReg enum to its hardware address (0xDFFxxx)
  for (size_t i = 0; i < CustReg::_COUNT_; ++i) {
    uint32_t addr = CustReg::cust_reg_data[i].addr;
    regsData[i + data_offset] =
        m_pVm->m_vAmiga->mem.debugger.spypeek16(vamiga::Accessor::CPU, addr);
  }
}

void VAmVmImp::CustomRegs::commit() {
  // Write modified register values back through vAmiga's memory subsystem.
  // Note: Many custom registers are read-only (e.g., VPOSR, DMACONR).
  // Writing to read-only addresses is harmless — the hardware ignores writes.
  // Only write registers that differ from their fetched values.
  for (size_t i = 0; i < CustReg::_COUNT_; ++i) {
    uint32_t addr = CustReg::cust_reg_data[i].addr;
    // Use the memory write path — writes to custom registers go through Agnus
    m_pVm->mem->setU16(addr, regsData[i + data_offset]);
  }
}

amD::AddrRef VAmVmImp::Copper::getCopperAddr(IVm::ECopperAddr_ copno) {
  switch (copno) {
    case IVm::CopperAddr_cop1lc: return m_copInfo.cop1lc;
    case IVm::CopperAddr_cop2lc: return m_copInfo.cop2lc;
    case IVm::CopperAddr_ip:     return m_copInfo.coppc0;
    case IVm::CopperAddr_vblankip: return m_copInfo.cop1lc;  // vblank starts copper list 1
    default: return 0;
  }
}

void VAmVmImp::Copper::fetch() {
  m_copInfo = m_pVm->m_vAmiga->agnus.copper.getInfo();
}

int VAmVmImp::Emu::getDebugDmaMode() {
  return 0;  // ::debug_dma;
}

void VAmVmImp::Emu::initBreakPoints(amD::BreakpointsSortedList& bpList) {
  bpList.mBreakpoints.clear();

  vamiga::VAmiga* pVAmiga = vm->m_vAmiga;
  long count = (long)pVAmiga->cpu.breakpoints.elements();

  for (long i = 0; i < count; ++i) {
    auto guardInfo = pVAmiga->cpu.breakpoints.guardNr(i);
    if (!guardInfo.has_value()) continue;
    if (!guardInfo->enabled) continue;

    amD::Breakpoint& curBp = bpList.mBreakpoints.emplace_back();
    curBp.addr1 = guardInfo->addr;
    curBp.addr2 = 0;
    curBp.enabled = guardInfo->enabled;
    curBp.reg = IVm::EReg::PC;  // vAmiga breakpoints are address-based

    amD::BreakpointsSortedList::OneAddrBp bp;
    bp.addr = curBp.addr1;
    bp.bpIdx = (int)bpList.mBreakpoints.size() - 1;
    bpList.mOneAddrBps.insert(bp);
  }
}

void VAmVmImp::Emu::setDebugDmaMode(int p_mode) {
  //::debug_dma = p_mode;
}

void VAmVmImp::setVmDebugMode(EVmDebugMode debug_mode) {
  TSuper::setVmDebugMode(debug_mode);
  vamiga::VAmiga* pVAmiga = m_vAmiga;
  if (debug_mode == EVmDebugMode::Break) {
    pVAmiga->pause();
  } else if (debug_mode == EVmDebugMode::Live) {
    pVAmiga->run();
  }
}

int VAmVmImp::getVPos() {
  return (int)m_vAmiga->amiga.getInfo().vpos;
}

int VAmVmImp::getHPos() {
  return (int)m_vAmiga->amiga.getInfo().hpos;
}

int VAmVmImp::getCurCycle() {
  // Return horizontal position as the cycle count within the current scanline
  return (int)m_vAmiga->amiga.getInfo().hpos;
}

bool VAmVmImp::Emu::isDebugActivated() const {
  // Debugger is active when the emulator is paused (breakpoint hit, manual break)
  return vm->m_vAmiga->isPaused();
}

bool VAmVmImp::Emu::isDebugActivatedFull() const {
  // Fully activated = paused AND debugger is in Break mode
  return vm->m_vAmiga->isPaused() && vm->getVmDebugMode() == EVmDebugMode::Break;
}

bool VAmVmImp::Floppy::getEnabled() {
  return m_pVm->m_vAmiga->df[m_nFloppy]->getConfig().connected;
}

void VAmVmImp::Floppy::setEnabled(bool v) {
  try {
    m_pVm->m_vAmiga->emu->set(vamiga::Opt::DRIVE_CONNECT, v ? 1 : 0, {m_nFloppy});
  } catch (...) {
    // Ignore config errors (e.g., cannot disconnect drive 0)
  }
}


void VAmVmImp::Floppy::setAdfPath(const qtd::string &v)
{
    m_adfPath = v;
    if (v.empty()) {
        m_pVm->m_vAmiga->df[m_nFloppy]->ejectDisk();
    } else {
        vamiga::FloppyDriveAPI *df = m_pVm->m_vAmiga->df[m_nFloppy];
        df->insert(v.c_str(), m_writeProtect);
    }
}


qtd::string VAmVmImp::Floppy::getAdfPath()
{
    return m_adfPath;
}


void VAmVmImp::Floppy::init(IVm::VM *vm)
{
    m_pVm = static_cast<VAmVmImp *>(vm);
}

bool VAmVmImp::Floppy::getWriteProtect()
{
    return m_pVm->m_vAmiga->df[m_nFloppy]->getInfo().hasProtectedDisk;
}

void VAmVmImp::Floppy::setWriteProtect(bool v)
{
    m_writeProtect = v;
    m_pVm->m_vAmiga->df[m_nFloppy]->setFlag(vamiga::DiskFlags::PROTECTED, v);
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

bool VAmVmImp::Cpu::isMmuEnabled() const {
  return false;
}

int VAmVmImp::Cpu::getCpuModel() const {
  return 68000;
}

void VAmVmImp::Cpu::getMmuPages(qtd::vector<MmuPage>& outPages, ::IVm::Cpu::MmuStats* outStats) const {
  // vAmiga (68000/68010) has no MMU
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

uint16_t VAmVmImp::Memory::getU16(AddrRef addr) {
  return m_pVm->m_vAmiga->mem.debugger.spypeek16(vamiga::Accessor::CPU, (uint32_t)addr);
}

uint8_t VAmVmImp::Memory::getU8(AddrRef addr) {
  return m_pVm->m_vAmiga->mem.debugger.spypeek8(vamiga::Accessor::CPU, (uint32_t)addr);
}

bool VAmVmImp::Memory::getU16(AddrRef addr, uint16_t* out) {
  *out = getU16(addr);
  return true;
}

void VAmVmImp::Memory::setU16(AddrRef addr, uint16_t v) {
  // Find the bank for this address and write directly
  const IVm::MemBank* pBank = findBankByAddr(addr);
  if (!pBank || !pBank->m_realAddr) return;
  // Write in big-endian (Amiga is 68000 big-endian)
  uint8_t* ptr = pBank->m_realAddr + (addr - pBank->m_startAddr);
  ptr[0] = (uint8_t)(v >> 8);
  ptr[1] = (uint8_t)(v & 0xFF);
}

uint32_t VAmVmImp::Memory::getU32(AddrRef addr) {
  return ((uint32_t)getU16(addr) << 16) | (uint32_t)getU16(addr + 2);
}

void VAmVmImp::Memory::setU32(AddrRef addr, uint32_t v) {
  setU16(addr, (uint16_t)(v >> 16));
  setU16(addr + 2, (uint16_t)(v & 0xFFFF));
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
