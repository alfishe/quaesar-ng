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
#include "VAmiga.h"
//#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_1(VAmServerThread);
FORWARD_DECLARATION_2(vamiga, VAmiga);
class QuaesarVAmigaInjectAccess;
#undef main


namespace IVm::imp {

class VAmVmImp final : public IVm::VM {
    //TS_REFLECT_CLASS(amD::vm::imp::VAmVmImp, IVm::VM);
    typedef IVm::VM TSuper;
public:
    VAmServerThread* m_pVAmThread = nullptr;
    vamiga::VAmiga* m_vAmiga = nullptr;
    vamiga::Amiga* main = nullptr;
    QuaesarVAmigaInjectAccess* m_vaAccess = nullptr;

public:
    VAmVmImp(VAmServerThread* pVAmThread, vamiga::VAmiga* pVAmiga);
    virtual ~VAmVmImp() override;
    virtual void init() override;

    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;

    virtual IVm::EVmDebugMode getVmDebugMode() const override {
        return TSuper::getVmDebugMode();
    }
    virtual void setVmDebugMode(IVm::EVmDebugMode debug_mode) override;

    virtual int getCurCycle() override;
    virtual int getVPos() override;
    virtual int getHPos() override;


    //------------------------------------------------------------------------
    struct Cpu : public IVm::Cpu {
        VAmVmImp* m_pVm = nullptr;
        vamiga::VAmiga* m_pVAmiga = nullptr;
        const vamiga::CPUInfo* m_pCpuInfo = nullptr;

    public:
        uint32_t getRegA(int i) const override {
            return m_pCpuInfo->a[i];
        }
        uint32_t getRegD(int i) const override {
            return m_pCpuInfo->d[i];
        }
        AddrRef getPC() const override {
            return m_pCpuInfo->pc0;
        }

        virtual bool getFlg(ECpuFlg_ f) const override;
        virtual int getIntMask() const override {
            return (m_pCpuInfo->sr >> 8) & 7;  // IPL bits from SR
        }

        virtual void fetch() override {
            m_pCpuInfo = &m_pVAmiga->cpu.getInfo();
        }

        virtual bool isMmuEnabled() const override;
        virtual int getCpuModel() const override;
        virtual void getMmuPages(qtd::vector<MmuPage>& outPages, ::IVm::Cpu::MmuStats* outStats = nullptr) const override;
    };  // struct Cpu
    Cpu instCpu;


    //------------------------------------------------------------------------
    struct Memory final : public IVm::Memory {
        VAmVmImp* m_pVm = nullptr;
        vamiga::VAmiga* m_pVAmiga = nullptr;

    public:
        virtual void init(IVm::VM* p_vm) override;
        virtual uint8_t* getRealAddr(AddrRef ptr) override;
        virtual bool getU16(AddrRef addr, uint16_t* out) override;
        virtual uint16_t getU16(AddrRef addr) override;
        virtual uint8_t getU8(AddrRef addr) override;
        virtual void setU16(AddrRef addr, uint16_t v) override;
        virtual uint32_t getU32(AddrRef addr) override;
        virtual void setU32(AddrRef addr, uint32_t v) override;
    };  // struct Memory
    Memory instMemory;


    //------------------------------------------------------------------------
    struct Blitter final : public IVm::Blitter {
        VAmVmImp* m_pVm = nullptr;
    public:
        virtual bool isBlitterActive() const override;
        virtual void* getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) override;
    } instBlitter;


    //------------------------------------------------------------------------
    class CustomRegs final : public IVm::CustomRegs {
        static constexpr size_t data_offset = 2;
        eastl::array<uint16_t, CustReg::_COUNT_ + data_offset> regsData;
    public:
        VAmVmImp* m_pVm = nullptr;

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
        VAmVmImp* m_pVm = nullptr;
        vamiga::CopperInfo m_copInfo = {};
        virtual void fetch() override;
        virtual AddrRef getCopperAddr(IVm::ECopperAddr_ copno) override;
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
            *out_w = (int)vamiga::HPIXELS;   // 912
            *out_h = (int)vamiga::VPIXELS;   // 313
        }
        virtual void initBreakPoints(amD::BreakpointsSortedList& bpList) override;
    };  // class Emu
    Emu instEmu;


    //------------------------------------------------------------------------
    class Floppy : public IVm::Floppy {
        VAmVmImp *m_pVm = nullptr;
        bool m_writeProtect = false;
        qtd::string m_adfPath;
    public:
        virtual bool getEnabled() override;
        virtual void setEnabled(bool v) override;
        virtual bool getWriteProtect() override;
        virtual void setWriteProtect(bool v) override;
        virtual qtd::string getAdfPath() override;
        virtual void setAdfPath(const qtd::string& v) override;

        virtual void init(IVm::VM *) override;
    };
    qtd::array<Floppy, IVm::MAX_FLOPPIES> instFloppies = {};

};  // class VAmVmImp
//////////////////////////////////////////////////////////////////////////


};  //namespace IVm::imp
