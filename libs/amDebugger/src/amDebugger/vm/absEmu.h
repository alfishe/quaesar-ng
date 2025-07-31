#pragma once
#include <qd/base/types.h>
#include <EASTL/span.h>
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>

namespace amD {

enum DebuggerMode;


//////////////////////////////////////////////////////////////////////////

class AbsEmu {
protected:
    int amiga_width = (754 + 7) & ~7;
    int amiga_height = 576;
    bool mInit = false;
    static AbsEmu* staticVmInst;
    AbsEmu();

public:
    static AbsEmu* get() {
        return AbsEmu::staticVmInst;
    }
    static AbsEmu* setVmInst(AbsEmu* vm_inst) {
        AbsEmu::staticVmInst = vm_inst;
        return AbsEmu::staticVmInst;
    }
    static void destrotVmInst() {
        AbsEmu* oldVm = AbsEmu::staticVmInst;
        AbsEmu::staticVmInst = nullptr;
        delete oldVm;
    }
    virtual ~AbsEmu();

    virtual void init() = 0;

    int getScreenSizeX() const {
        return amiga_width;
    }
    int getScreenSizeY() const {
        return amiga_height;
    }


    class Memory { /*: public vm::imp::UaeEmuVmImp::Memory*/
    public:
        eastl::fixed_vector<amD::MemBank, 8, false> banks;

    public:
        const amD::MemBank* getBankByInd(int ind) const {
            if ((size_t)ind < banks.size())
                return &banks[ind];
            return nullptr;
        }
        eastl::span<const amD::MemBank> getBanks() const {
            return banks;
        }
        virtual uint8_t* getRealAddr(AddrRef ptr) = 0;
        virtual bool getU16(AddrRef addr, uint16_t* out) = 0;
        virtual uint16_t getU16(AddrRef addr) = 0;
        virtual void setU16(AddrRef addr, uint16_t v) = 0;
        virtual uint32_t getU32(AddrRef addr) = 0;
        virtual void setU32(AddrRef addr, uint32_t v) = 0;

        const amD::MemBank *findBankByAddr(AddrRef addr) const {
            for (const amD::MemBank& bank : banks) {
                if (addr >= bank.m_startAddr && addr < (bank.m_startAddr + bank.m_size))
                    return &bank;
            }
            return nullptr;
        }

    };  // struct Memory
    amD::AbsEmu::Memory* mem = nullptr;


    class Cpu {
    public:
        virtual uint32_t getRegA(int i) const = 0;
        virtual uint32_t getRegD(int i) const = 0;
        virtual AddrRef getPC() const = 0;
        virtual bool getFlg(CpuFlg_ f) const = 0;
        virtual int getIntMask() const = 0;
    };  // struct Cpu
    amD::AbsEmu::Cpu* cpu = nullptr;


    class CustomRegs {
    public:
        virtual void fetch() = 0;
        virtual void commit() = 0;

        virtual uint16_t getRegVal(CustReg reg) = 0;
        virtual void setRegVal(CustReg reg, uint16_t new_val) = 0;

    };  // class CustomRegs
    amD::AbsEmu::CustomRegs* custom = nullptr;


    class Copper {
    public:
        virtual void fetch() = 0;
        virtual AddrRef getCopperAddr(CopperAddr_ copno) = 0;
    };  // class Copper
    amD::AbsEmu::Copper* copper = nullptr;


    struct Blitter {
    public:
        virtual bool isBlitterActive() const = 0;
        virtual void* getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) = 0;
    };  // Blitter
    amD::AbsEmu::Blitter* blitter = nullptr;


    struct Emu {
    public:
        virtual void setDebugMode(DebuggerMode debug_mode) /*base*/ {}
    };
    amD::AbsEmu::Emu* emu = nullptr;


};  // class AbsEmu


void* impFactoryCreateInstance(const std::type_info& type);


template <typename T>
inline T *createByFactory() {
  return static_cast<T*>(impFactoryCreateInstance(typeid(T)));
}

};  // namespace amD
