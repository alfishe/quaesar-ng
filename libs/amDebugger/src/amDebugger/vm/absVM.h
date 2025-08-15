    #pragma once
#include <qd/base/baseTypes.h>
#include <EASTL/span.h>
#include <amDebugger/vm/customRegs.h>
#include <amDebugger/vm/emuDefs.h>
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/net/replicatedVar.h"


enum EDebuggerMode;

//////////////////////////////////////////////////////////////////////////
// It gonna be a snapshot of the machine at any given moment, just data.

namespace AbsVM
{
static constexpr int MAX_FLOPPIES = 4;

class Memory;
class Cpu;
class CustomRegs;
class Copper;
class Blitter;
class Emu;
class Floppy;


	class VM : public qd::RefCounted, public qd::net::ReflectableObject {
	    TS_REFLECT_CLASS(AbsVM::VM, qd::IOperationEnvironment);
	public:
	    enum EModuleState
	    {
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
	    static AbsVM::VM* staticVmInst;
	    AbsVM::VM();

	public:
	    static AbsVM::VM* get() {
	        return AbsVM::VM::staticVmInst;
	    }
	    static AbsVM::VM* setVmInst(AbsVM::VM* vm_inst);
	    static void destrotVmInst();
	    virtual ~VM();

	    virtual void init() = 0;
	    virtual qd::EFlow applyOperationProc(qd::operation::args::Base* args)
	    {
	        return qd::EFlow::NO_RESULT;
	    }

	    int getScreenSizeX() const {
	        return amiga_width;
	    }
	    int getScreenSizeY() const {
	        return amiga_height;
	    }

	    //virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const = 0;

	    AbsVM::Memory* mem = nullptr;
	    AbsVM::Cpu* cpu = nullptr;
	    AbsVM::CustomRegs* custom = nullptr;
	    AbsVM::Copper* copper = nullptr;
        AbsVM::Blitter* blitter = nullptr;
	    qd::array<AbsVM::Floppy*, MAX_FLOPPIES> floppySlots = {};
	    AbsVM::Emu* emu = nullptr;

	};  // class AbsVM::VM
	//////////////////////////////////////////////////////////////////////////


    class Floppy : public qd::net::ReflectableObject
    {
    public:
        DECL_REFLECTION(AbsVM::Floppy, qd::net::ReflectableObject);
        REFL_VAR(1, bool, active);
        REFL_VAR(2, bool, writeProtect);
        // REFL_VAR(3, qd::string, adfPath);
    }; // class Floppy
    //////////////////////////////////////////////////////////////////////////


    class Emu {
    public:
        amD::EDebuggerMode m_debugMode = amD::DebuggerMode_Live;
        virtual void setDebugMode(amD::EDebuggerMode debug_mode) /*base*/ {}
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
    }; // struct Memory
    //////////////////////////////////////////////////////////////////////////


    class Cpu
    {
    public:
        virtual uint32_t getRegA(int i) const = 0;
        virtual uint32_t getRegD(int i) const = 0;
        virtual AddrRef getPC() const = 0;
        virtual bool getFlg(amD::ECpuFlg_ f) const = 0;
        virtual int getIntMask() const = 0;
    }; // struct Cpu
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
    }; // Blitter
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////

	void* impFactoryCreateInstance(const std::type_info& type);

	template <typename T>
	inline T *createByFactory_() {
	  return static_cast<T*>(impFactoryCreateInstance(typeid(T)));
	}


}; // namespace AbsVM
