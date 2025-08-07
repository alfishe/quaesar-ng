#pragma once
// clang-format off
#include <sysconfig.h>
#include <uae_lib/include/sysdeps.h>
#include <uae_lib/include/options.h>
#include <uae_lib/include/memory.h>
#include <uae_lib/include/newcpu.h>
// clang-format on
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>
#include <amDebugger/vm/absVM.h>
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>
#include <amDebugger/vm/memory.h>
#include <qd/Base/color.h>
#include <qd/Base/types.h>
#include "qd/typeSystem/typeDeclare.h"


namespace amD {


namespace vm {
namespace imp {

class UaeVmImp final : public amD::AbsVM {
    TS_REFLECT_CLASS(amD::vm::imp::UaeVmImp, amD::AbsVM);

public:
    UaeVmImp();
    virtual ~UaeVmImp();
    virtual void init() override;

    //     virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationProc(qd::operation::args::Base* args) override {
        return qd::EFlow::NO_RESULT;
    }

    struct Cpu : public AbsVM::Cpu {
        uint32_t getRegA(int i) const override {
            return m68k_areg(::regs, i);
        }
        uint32_t getRegD(int i) const override {
            return m68k_dreg(regs, i);
        }
        AddrRef getPC() const override {
            return m68k_getpc();
        }

        virtual bool getFlg(CpuFlg_ f) const override {
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
        virtual int getIntMask() const override {
            return regs.intmask;
        }
    };  // struct Cpu
    Cpu instCpu;

    ///
    struct Memory final : public AbsVM::Memory {
    public:
        virtual uint8_t* getRealAddr(AddrRef ptr) override {
            return (uint8_t*)::memory_get_real_address(ptr);
        }
        virtual bool getU16(AddrRef addr, uint16_t* out) override {
            *out = (uint16_t)::memory_get_word(addr);
            return true;
        }
        virtual uint16_t getU16(AddrRef addr) override {
            return (uint16_t)::memory_get_word(addr);
        }
        virtual void setU16(AddrRef addr, uint16_t v) override {
            ::memory_put_word(addr, v);
        }
        virtual uint32_t getU32(AddrRef addr) override {
            return (uint32_t)::memory_get_long(addr);
        }
        virtual void setU32(AddrRef addr, uint32_t v) override {
            ::memory_put_long(addr, v);
        }
    };  // struct Memory
    Memory instMemory;

    //
    struct Blitter final : public AbsVM::Blitter {
    public:
        virtual bool isBlitterActive() const override;
        virtual void* getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) override;
    } instBlitter;


    //
    class CustomRegs final : public AbsVM::CustomRegs {
        static constexpr size_t data_offset = 2;
        eastl::array<uint16_t, CustReg::_COUNT_ + data_offset> regsData;

    public:
        void fetch() override;
        void commit() override;

        uint16_t getRegVal(CustReg reg) override {
            return regsData[(size_t)reg + data_offset];
        }
        void setRegVal(CustReg reg, uint16_t new_val) override {
            regsData[(size_t)reg + data_offset] = new_val;
        }
    };  // class CustomRegs
    CustomRegs instCustomRegs;


    class Copper final : public AbsVM::Copper {
    public:
        virtual void fetch() override;
        virtual AddrRef getCopperAddr(CopperAddr_ copno) override;
    };  // class Copper
    Copper instCopper;


    class Emu final : public AbsVM::Emu {
    public:
        UaeVmImp* vm = nullptr;
        virtual void setDebugMode(DebuggerMode debug_mode) override;
        bool isDebugActivatedFull() const;
        bool isDebugActivated() const;
    };  // class Emu
    Emu instEmu;


};  // class UaeVmImp
//////////////////////////////////////////////////////////////////////////


};  // namespace imp
};  // namespace vm
};  //namespace amD
