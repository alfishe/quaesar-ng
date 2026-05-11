#pragma once
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>
#include <qd/base/baseTypes.h>
#include <qd/typeSystem/typeDeclare.h>
#include <qd/stl/span.h>
#include <qd/stl/array.h>
#include <qd/stl/string.h>
#include <qd/qui/uiOperation.h>


struct CfgVmPrefs;
FORWARD_DECLARATION_2(amD, BreakpointsSortedList);


//////////////////////////////////////////////////////////////////////////
// It gonna be a snapshot of the machine at any given moment, just data.

namespace IVm {
static constexpr int MAX_FLOPPIES = 4;

class VM;
class Memory;
class Cpu;
class CustomRegs;
class Copper;
class Blitter;
class Emu;
class Floppy;


class IVmHandler
{
public:
    virtual IVm::VM* getVm() const = 0;
};

class IModule
{
public:
    IModule() = default;
    virtual void init(IVm::VM *) {}
    virtual void fetch() {}
};

//////////////////////////////////////////////////////////////////////////
class VM
    : public qd::RefCounted
    , public qd::IOperationEnvironment
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
    int m_scrSizeX = (754 + 7) & ~7;
    int m_scrSizeY = 576;
    bool mInit = false;
    IVm::EVmDebugMode m_debugMode = IVm::EVmDebugMode::Live;
    VM();

public:
    virtual ~VM() override;

    virtual void init();
    virtual void fetchStateFromEmu();
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* /*args*/) override;
    virtual void applyVmConfig(CfgVmPrefs* prefs);

    int getScreenSizeX() const { return m_scrSizeX; }
    int getScreenSizeY() const { return m_scrSizeY; }
    virtual int getCurCycle() { return -1; }
    virtual int getVPos() { return -1; }
    virtual int getHPos() { return -1; }

    virtual IVm::EVmDebugMode getVmDebugMode() const { return m_debugMode; }
    virtual void setVmDebugMode(IVm::EVmDebugMode debug_mode) { m_debugMode = debug_mode; }; // base

    IVm::IModule* m_modSectBeg = nullptr;
    IVm::Memory* mem = nullptr;
    IVm::Cpu* cpu = nullptr;
    IVm::CustomRegs* custom = nullptr;
    IVm::Copper* copper = nullptr;
    IVm::Blitter* blitter = nullptr;
    IVm::Floppy* floppy0 = nullptr;
    IVm::Floppy* floppy1 = nullptr;
    IVm::Floppy* floppy2 = nullptr;
    IVm::Floppy* floppy3 = nullptr;
    IVm::Emu* emu = nullptr;
    IVm::IModule* m_modSectEnd = nullptr;

}; // class IVm::VM
//////////////////////////////////////////////////////////////////////////


class Floppy : public IVm::IModule
{
public:
    int m_nFloppy = 0;
    virtual bool getEnabled() = 0;
    virtual void setEnabled(bool v) = 0;
    virtual bool getWriteProtect() = 0;
    virtual void setWriteProtect(bool v) = 0;
    virtual qtd::string getAdfPath() = 0;
    virtual void setAdfPath(const qtd::string& v) = 0;
    virtual ~Floppy() = default;

}; // class Floppy
//////////////////////////////////////////////////////////////////////////


class Emu : public IVm::IModule
{
public:
    virtual int getDebugDmaMode() { return 0; }
    virtual void setDebugDmaMode(int /*p_mode*/) {}

    virtual void getScreenSize(int* out_w, int* out_h) const
    {
        *out_w = 754;
        *out_h = 576;
    }
    virtual void initBreakPoints(amD::BreakpointsSortedList& /*bpList*/) {}

}; // class Emu
//////////////////////////////////////////////////////////////////////////


class Memory : public IVm::IModule
{
public:
    qtd::array<IVm::MemBank, EMemSrc::MAX_COUNT> m_banks;

public:
    const IVm::MemBank* getBankByInd(int ind) const
    {
        if ((size_t)ind < m_banks.size())
            return &m_banks[ind];
        return nullptr;
    }
    qtd::span<const IVm::MemBank> getBanks() const { return m_banks; }
    virtual uint8_t* getRealAddr(AddrRef ptr) = 0;
    virtual bool getU16(AddrRef addr, uint16_t* out) = 0;
    virtual uint16_t getU16(AddrRef addr) = 0;
    virtual void setU16(AddrRef addr, uint16_t v) = 0;
    virtual uint32_t getU32(AddrRef addr) = 0;
    virtual void setU32(AddrRef addr, uint32_t v) = 0;

    const IVm::MemBank* findBankByAddr(AddrRef addr) const;
}; // class Memory
//////////////////////////////////////////////////////////////////////////


class Cpu : public IVm::IModule
{
public:
    virtual uint32_t getRegA(int i) const = 0;
    virtual uint32_t getRegD(int i) const = 0;
    virtual AddrRef getPC() const = 0;
    virtual bool getFlg(IVm::ECpuFlg_ f) const = 0;
    virtual int getIntMask() const = 0;
}; // class Cpu
//////////////////////////////////////////////////////////////////////////


class CustomRegs : public IVm::IModule
{
public:
    virtual void fetch() override = 0;
    virtual void commit() = 0;

    virtual uint16_t getRegVal(IVm::CustReg reg) = 0;
    virtual void setRegVal(IVm::CustReg reg, uint16_t new_val) = 0;

}; // class CustomRegs
//////////////////////////////////////////////////////////////////////////


class Copper : public IModule
{
public:
    virtual void fetch() override = 0;
    virtual AddrRef getCopperAddr(IVm::ECopperAddr_ copno) = 0;
}; // class Copper
//////////////////////////////////////////////////////////////////////////


class Blitter : public IModule
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
