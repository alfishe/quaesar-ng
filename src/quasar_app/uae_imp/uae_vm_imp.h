#pragma once
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vmInterface.h>
#include <qd/base/baseTypes.h>
#include <qd/base/Color.h>


FORWARD_DECLARATION_1(UaeServerThread);


namespace IVm::imp {

class UaeVmImp final : public IVm::VM {
    //TS_REFLECT_CLASS(IVm::imp::UaeVmImp, IVm::VM);
    typedef IVm::VM TSuper;
    UaeServerThread* m_pUaeThread = nullptr;

public:
    UaeVmImp();
    void setServerImp(UaeServerThread* pUaeThread) {
        m_pUaeThread = pUaeThread;
    }
    virtual ~UaeVmImp() override;
    virtual void init() override;

    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;

    virtual IVm::EVmDebugMode getVmDebugMode() const override {
        return TSuper::getVmDebugMode();
    }
    virtual void setVmDebugMode(IVm::EVmDebugMode debug_mode) override;

    virtual int getCurCycle() override;
    virtual int getVPos() override;
    virtual int getHPos() override;
    virtual int getChipsetLevel() const override;


    //------------------------------------------------------------------------
    // Cpu module — snapshots register state in fetch() so that widgets
    // see stable values between fetchVmState() calls (~15fps throttle).
    // Without this snapshot, getters read the live UAE ::regs global which
    // changes continuously as the emulator runs, causing register and
    // disassembly widgets to flicker at 60fps.
    struct Cpu : public IVm::Cpu {
        // Snapshot populated by fetch(). Reads from getters return these
        // cached values, not live emulator state.
        uint32_t snap_regs_d[8] = {};
        uint32_t snap_regs_a[8] = {};
        uint32_t snap_pc = 0;
        uint32_t snap_intmask = 0;
        bool snap_flg_z = false;
        bool snap_flg_c = false;
        bool snap_flg_v = false;
        bool snap_flg_n = false;
        bool snap_flg_x = false;

        void fetch() override;
        uint32_t getRegA(int i) const override;
        uint32_t getRegD(int i) const override;
        AddrRef getPC() const override;

        virtual bool getFlg(IVm::ECpuFlg_ f) const override;
        virtual int getIntMask() const override;
    };  // struct Cpu
    Cpu instCpu;


    //------------------------------------------------------------------------
    struct Memory final : public IVm::Memory {
    public:
        virtual void init(IVm::VM* p_vm) override;
        virtual uint8_t* getRealAddr(AddrRef ptr) override;
        virtual bool getU16(AddrRef addr, uint16_t* out) override;
        virtual uint16_t getU16(AddrRef addr) override;
        virtual void setU16(AddrRef addr, uint16_t v) override;
        virtual uint32_t getU32(AddrRef addr) override;
        virtual void setU32(AddrRef addr, uint32_t v) override;
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
        eastl::array<uint16_t, IVm::CustReg::_COUNT_ + data_offset> regsData;

    public:
        void fetch() override;
        void commit() override;

        uint16_t getRegVal(IVm::CustReg reg) override {
            return regsData[(size_t)reg + data_offset];
        }
        void setRegVal(IVm::CustReg reg, uint16_t new_val) override {
            regsData[(size_t)reg + data_offset] = new_val;
        }
    };  // class CustomRegs
    CustomRegs instCustomRegs;


    //------------------------------------------------------------------------
    class Copper final : public IVm::Copper {
    public:
        virtual void fetch() override;
        virtual AddrRef getCopperAddr(IVm::ECopperAddr_ copno) override;
    };  // class Copper
    Copper instCopper;


    //------------------------------------------------------------------------
    class Emu final : public IVm::Emu {
    public:
        UaeVmImp* vm = nullptr;
        virtual int getDebugDmaMode() override;
        virtual void setDebugDmaMode(int p_mode) override;
        bool isDebugActivatedFull() const;
        bool isDebugActivated() const;
        virtual void getScreenSize(int* out_w, int* out_h) const override {
            *out_w = 754;
            *out_h = 576;
        }
        virtual void initBreakPoints(amD::BreakpointsSortedList& bpList) override;
    };  // class Emu
    Emu instEmu;


    //------------------------------------------------------------------------
    class Floppy : public IVm::Floppy {
    public:
        virtual bool getEnabled() override;
        virtual void setEnabled(bool v) override;
        virtual bool getWriteProtect() override;
        virtual void setWriteProtect(bool v) override;
        virtual qtd::string getAdfPath() override;
        virtual void setAdfPath(const qtd::string& v) override;
    };
    qtd::array<Floppy, IVm::MAX_FLOPPIES> instFloppies = {};

};  // class UaeVmImp
//////////////////////////////////////////////////////////////////////////


};  //namespace IVm::imp
