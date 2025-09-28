#pragma once
#include <cstdint>
#include "qd/stl/string.h"
#include "qd/stl/span.h"
#include "amDebugger/base.h"
#include "qd/enum/enumBase.h"
#include "amDebugger/vm/memory.h"


namespace IVm {

//------------------------------------------------------------------------
/*
*     rom: Read-only memory
*          Holds a Kickstart Rom or a Boot Rom (A1000).
*
*     wom: Write-once Memory
*          If rom holds a Boot Rom, a wom is automatically created. It
*          is the place where the A1000 stores the Kickstart loaded
*          from disk.
*
*     ext: Extended Rom
*          Such a Rom was added to newer Amiga models when the 512 KB
*          Kickstart Rom became too small. It is emulated to support
*          the Aros Kickstart replacement.
*
*    chip: Chip Ram
*          Holds the memory which is shared by the CPU and the Amiga Chip
*          set. The original Agnus chip is able to address 512 KB Chip
*          memory. Newer models are able to address up to 2 MB.
*
*    slow: Slow Ram (aka Bogo Ram)
*          This Ram is addressed by the same bus as Chip Ram, but it can
*          used by the CPU only.
*
*    fast: Fast Ram
*          Only the CPU can access this Ram. It is connected via a
*          separate bus and doesn't slow down the Chip set when the CPU
*          addresses it.
*/

struct EMemSrc {
    enum Type {
        NONE,
        CHIP,
        CHIP_MIRROR,
        SLOW,
        SLOW_MIRROR,
        FAST,
        CIA,
        CIA_MIRROR,
        RTC, // Real-time clock
        CUSTOM,
        CUSTOM_MIRROR,
        AUTOCONF,
        ZOR,
        ROM,
        ROM_MIRROR,
        WOM,
        EXT,
        MAX_COUNT,
    };
    ENUM_DECLARE_BASE(IVm::, EMemSrc, Type, NONE);

    static const char* to_string(EMemSrc value);
    static const char* to_desc(EMemSrc value);

}; // struct MemSrc
//////////////////////////////////////////////////////////////////////////



class MemBank {

public:
    EMemSrc m_id = EMemSrc::NONE;
    uint32_t m_size = 0;
    uint32_t m_mask = 0;
    eastl::string m_name;
    eastl::string m_label;
    AddrRef m_startAddr = 0;
    uint8_t* m_realAddr = nullptr;
    bool m_bEnabled = false;

public:

    bool isValid() const {
        return c_def(this) && m_size != 0;
    }

    qd::span<uint8_t> getSpan() const {
        return {m_realAddr + m_startAddr, m_size};
    }

    bool isAddrIn(AddrRef addr) const {
        return (addr >= m_startAddr && addr < (m_startAddr + m_size)); }

    uint8_t getU8(AddrRef addr) const {
        return m_realAddr[addr - m_startAddr];
    }
    void setU8(AddrRef addr, uint8_t v) const {
        m_realAddr[addr - m_startAddr] = v;
    }

};  // class MemBank
//////////////////////////////////////////////////////////////////////////


struct EReg {
    enum Type : uint8_t {
        Dx = 0,
        Ax = 8,
        PC = 16,
        USP = 17,
        MSP = 18,
        ISP = 19,
        VBR = 20,
        SR = 21,
        CCR = 22,
        CACR = 23,
        CAAR = 24,
        SFC = 25,
        DFC = 26,
        TC = 27,
        ITT0 = 28,
        ITT1 = 29,
        DTT0 = 30,
        DTT1 = 31,
        BUSC = 32,
        PCR = 33,
        FPIAR = 34,
        FPCR = 35,
        FPSR = 36,
        MAX_COUNT = 37,
    };  // enum Type

    Type mV = MAX_COUNT;

    EReg() = default;
    EReg(Type v) : mV(v) {
    }
};  // struct EReg


// CPU Status Registers with Condition Code Register (CCR)
struct ECpuFlg {
    enum Type : uint8_t {
        C = 0,  // Carry
        V = 1,  // oVerflow
        Z = 2,  // Zero
        N = 3,  // Negative
        X = 4,  // eXtended

        I0 = 8,   // Interrupt priority mask bit 1
        I1 = 9,   // Interrupt priority mask bit 2
        I2 = 10,  // Interrupt priority mask bit 3
        M = 12,   // Master/Interrupt switch. Determines which stack mode to use if S is set
        S = 13,   // Supervisor Mode flag. If clear, SP refers to UserStack(USP) or SystemStack (SSP)
        T0 = 14,  // Trace bit 1. If set, trace on change of program flow
        T1 = 15,  // Trace bit 2. If set, trace is allowed on any instruction
        STOPPED,

        MAX_COUNT,
    };

    Type mV = MAX_COUNT;

    ECpuFlg() = default;
    ECpuFlg(Type v) : mV(v) {
    }
};  // ECpuFlg
//////////////////////////////////////////////////////////////////////////



};  // namespace IVm
