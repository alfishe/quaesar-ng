#pragma once
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeDeclare.h"
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>
#include <EASTL/span.h>
#include <qd/base/baseTypes.h>


enum EVmDebugMode;

//////////////////////////////////////////////////////////////////////////
// It gonna be a snapshot of the machine at any given moment, just data.

namespace IVm {
static constexpr int MAX_FLOPPIES = 4;

class Memory;
class Cpu;
class CustomRegs;
class Copper;
class Blitter;
class Emu;
class Floppy;


//////////////////////////////////////////////////////////////////////////
class VM
    : public qd::RefCounted
    , qd::IOperationEnvironment
{
    TS_REFLECT_CLASS(IVm::VM, qd::IOperationEnvironment);

public:
    enum EModuleState {
        MS_MEMORY,
        MS_CPU,
        MS_CUSTOM_REGS,
        MS_COPPER,
        MS_BLITTER,
        MS_FLOPPY,
        MS_MAX_COUNT,
    };


protected:
    int amiga_width = (754 + 7) & ~7;
    int amiga_height = 576;
    bool mInit = false;
    amD::EVmDebugMode m_debugMode = amD::EVmDebugMode::Live;

    static IVm::VM* staticVmInst;
    IVm::VM();

public:
    static IVm::VM* get() { return IVm::VM::staticVmInst; }
    static IVm::VM* setVmInst(IVm::VM* vm_inst);
    static void destrotVmInst();
    virtual ~VM();

    virtual void init() = 0;
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* args) { assert(0); return qd::EFlow::NO_RESULT; }

    int getScreenSizeX() const { return amiga_width; }
    int getScreenSizeY() const { return amiga_height; }

    virtual amD::EVmDebugMode getVmDebugMode() const { return m_debugMode; }
    virtual void setVmDebugMode(amD::EVmDebugMode debug_mode) { m_debugMode = debug_mode; }; // base

    IVm::Memory* mem = nullptr;
    IVm::Cpu* cpu = nullptr;
    IVm::CustomRegs* custom = nullptr;
    IVm::Copper* copper = nullptr;
    IVm::Blitter* blitter = nullptr;
    qd::array<IVm::Floppy*, IVm::MAX_FLOPPIES> floppies = {};
    IVm::Emu* emu = nullptr;

}; // class IVm::VM
//////////////////////////////////////////////////////////////////////////


class Floppy
{
public:
    int m_nFloppy = 0;
    virtual bool getEnabled() = 0;
    virtual void setEnabled(bool v) = 0;
    virtual bool getWriteProtect() = 0;
    virtual void setWriteProtect(bool v) = 0;
    virtual qd::string getAdfPath() = 0;
    virtual void setAdfPath(const qd::string& v) = 0;
    virtual ~Floppy() = default;

}; // class Floppy
//////////////////////////////////////////////////////////////////////////


class Emu
{

public:
    virtual int getDebugDmaMode() { return 0; }
    virtual void setDebugDmaMode(int p_mode) {}

    virtual void getScreenSize(int* out_w, int* out_h) const
    {
        *out_w = 754;
        *out_h = 576;
    }

}; // class Emu
//////////////////////////////////////////////////////////////////////////


class Memory
{ /*: public vm::imp::UaeEmuVmImp::Memory*/
public:
    eastl::fixed_vector<amD::MemBank, 8, false> banks;

public:
    const amD::MemBank* getBankByInd(int ind) const
    {
        if ((size_t)ind < banks.size())
            return &banks[ind];
        return nullptr;
    }
    eastl::span<const amD::MemBank> getBanks() const { return banks; }
    virtual uint8_t* getRealAddr(AddrRef ptr) = 0;
    virtual bool getU16(AddrRef addr, uint16_t* out) = 0;
    virtual uint16_t getU16(AddrRef addr) = 0;
    virtual void setU16(AddrRef addr, uint16_t v) = 0;
    virtual uint32_t getU32(AddrRef addr) = 0;
    virtual void setU32(AddrRef addr, uint32_t v) = 0;

    const amD::MemBank* findBankByAddr(AddrRef addr) const
    {
        for (const amD::MemBank& bank : banks)
        {
            if (addr >= bank.m_startAddr && addr < (bank.m_startAddr + bank.m_size))
                return &bank;
        }
        return nullptr;
    }
}; // class Memory
//////////////////////////////////////////////////////////////////////////


class Cpu
{
public:
    virtual uint32_t getRegA(int i) const = 0;
    virtual uint32_t getRegD(int i) const = 0;
    virtual AddrRef getPC() const = 0;
    virtual bool getFlg(amD::ECpuFlg_ f) const = 0;
    virtual int getIntMask() const = 0;
}; // class Cpu
//////////////////////////////////////////////////////////////////////////


class CustomRegs
{
public:
    virtual void fetch() = 0;
    virtual void commit() = 0;

    virtual uint16_t getRegVal(amD::CustReg reg) = 0;
    virtual void setRegVal(amD::CustReg reg, uint16_t new_val) = 0;

}; // class CustomRegs
//////////////////////////////////////////////////////////////////////////


class Copper
{
public:
    virtual void fetch() = 0;
    virtual AddrRef getCopperAddr(amD::ECopperAddr_ copno) = 0;
}; // class Copper
//////////////////////////////////////////////////////////////////////////


class Blitter
{
public:
    virtual bool isBlitterActive() const = 0;
    virtual void* getScreenPixBuf(int mon_id, int* out_size_w, int* out_size_h, int* pitch) = 0;
}; // class Blitter
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

void* impFactoryCreateInstance(const std::type_info& type);

template<typename T>
inline T* createByFactory_()
{
    return static_cast<T*>(impFactoryCreateInstance(typeid(T)));
}


}; // namespace IVm
