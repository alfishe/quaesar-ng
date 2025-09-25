#pragma once
// clang-format off
// #include <sysconfig.h>
// #include <uae_lib/include/sysdeps.h>
// #include <uae_lib/include/options.h>
// #include <uae_lib/include/memory.h>
// #include <uae_lib/include/newcpu.h>
// clang-format on
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vmInterface.h>
#include <qd/base/baseTypes.h>
#include <qd/base/color.h>
#include "SDL_stdinc.h"  // strlcpy
#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_1(VAmServerThread);


namespace amD::vm::imp {

class VAmVmImp final : public IVm::VM {
    TS_REFLECT_CLASS(amD::vm::imp::VAmVmImp, IVm::VM);
    VAmServerThread* m_pVAmThread = nullptr;

public:
    VAmVmImp();
    void setServerImp(VAmServerThread* pVAmThread) {
        m_pVAmThread = pVAmThread;
    }
    virtual ~VAmVmImp() override;
    virtual void init() override;

    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;

    virtual amD::EVmDebugMode getVmDebugMode() const override {
        return TSuper::getVmDebugMode();
    }
    virtual void setVmDebugMode(amD::EVmDebugMode debug_mode) override;

    virtual int getCurCycle() override;
    virtual int getVPos() override;
    virtual int getHPos() override;


    //------------------------------------------------------------------------
    struct Cpu : public IVm::Cpu {
        uint32_t getRegA(int i) const override {
            return 0;  // m68k_areg(::regs, i);
        }
        uint32_t getRegD(int i) const override {
            return 0;  //m68k_dreg(regs, i);
        }
        AddrRef getPC() const override {
            return 0;  //m68k_getpc();
        }

        virtual bool getFlg(ECpuFlg_ f) const override;
        virtual int getIntMask() const override {
            return 0;  //regs.intmask;
        }
    };  // struct Cpu
    Cpu instCpu;


    //------------------------------------------------------------------------
    struct Memory final : public IVm::Memory {
    public:
        virtual uint8_t* getRealAddr(AddrRef ptr) override {
            return nullptr;  //(uint8_t*)::memory_get_real_address(ptr);
        }
        virtual bool getU16(AddrRef addr, uint16_t* out) override {
            *out = false;  //(uint16_t)::memory_get_word(addr);
            return true;
        }
        virtual uint16_t getU16(AddrRef addr) override {
            return 0;  //(uint16_t)::memory_get_word(addr);
        }
        virtual void setU16(AddrRef addr, uint16_t v) override {
            nullptr;  //::memory_put_word(addr, v);
        }
        virtual uint32_t getU32(AddrRef addr) override {
            return 0;  //(uint32_t)::memory_get_long(addr);
        }
        virtual void setU32(AddrRef addr, uint32_t v) override {
            nullptr;  //::memory_put_long(addr, v);
        }
    };  // struct Memory
    Memory instMemory;


    //------------------------------------------------------------------------
    struct Blitter final : public IVm::Blitter {
    public:
        virtual bool isBlitterActive() const override;
        virtual void* getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) override;
    } instBlitter;


    //------------------------------------------------------------------------
    class CustomRegs final : public IVm::CustomRegs {
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


    //------------------------------------------------------------------------
    class Copper final : public IVm::Copper {
    public:
        virtual void fetch() override;
        virtual AddrRef getCopperAddr(amD::ECopperAddr_ copno) override;
    };  // class Copper
    Copper instCopper;


    //------------------------------------------------------------------------
    class Emu final : public IVm::Emu {
    public:
        VAmVmImp* vm = nullptr;
        virtual int getDebugDmaMode() override;
        virtual void setDebugDmaMode(int p_mode) override;
        bool isDebugActivatedFull() const;
        bool isDebugActivated() const;
        virtual void getScreenSize(int* out_w, int* out_h) const override {
            *out_w = 754;
            *out_h = 576;
        }
    };  // class Emu
    Emu instEmu;


    //------------------------------------------------------------------------
    class Floppy : public IVm::Floppy {
    public:
        virtual bool getEnabled() override;
        virtual void setEnabled(bool v) override;
        virtual bool getWriteProtect() override {
            return false;
        }
        virtual void setWriteProtect(bool v) override {
        }
        virtual qd::string getAdfPath() override {
            return "";
        }
        virtual void setAdfPath(const qd::string& v) override {
        }
    };
    qd::array<Floppy, IVm::MAX_FLOPPIES> instFloppies = {};

};  // class VAmVmImp
//////////////////////////////////////////////////////////////////////////


};  //namespace amD::vm::imp
