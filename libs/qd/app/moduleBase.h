#pragma once
#include "qd/base/base.h"
#include "qd/stl/ref_ptr.h"
#include "qd/enum/enumBase.h"
#include "qd/typeSystem/typeDeclare.h"



namespace qd {
class ModuleManager;
class Application;


struct ModuleCreateParams {
    uint32_t classId = 0;
    ModuleManager* moduleMgr = nullptr;
    Application* app = nullptr;
    ModuleCreateParams(Application* p_app = nullptr)
        : app(p_app)
    {}
    virtual ~ModuleCreateParams() = default;
}; // struct ModuleCreateParams
//////////////////////////////////////////////////////////////////////////


namespace moduleMsg {

struct BaseMsg {
    uint32_t id;
    BaseMsg(uint32_t _id = 0)
        : id(_id)
    {}
};

template<uint32_t TID>
struct BaseMsg_ : public BaseMsg {
    constexpr static uint32_t ID = TID;
    BaseMsg_()
        : BaseMsg(TID)
    {}
};

#define MODULE_MSG_ID_(name) BaseMsg_<SCID(name)>


struct RENDER_IMGUI_DEBUG_INFO_TREE : MODULE_MSG_ID_(RenderImGui) {};


}; // namespace moduleMsg
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// BASE MODULE INTEFACE
class IModuleInterface
{
    TS_REFLECT_CLASS_BASE(10, qd::IModuleInterface, void);
    friend class ModuleManager;

public:
    IModuleInterface(const qd::ModuleCreateParams& /*mc*/) {}

    // Called right after the module DLL has been loaded and the module object has been created
    virtual void onModuleStartup(const qd::ModuleCreateParams& /*mc*/)
    {}

    // Called before the module has been unloaded
    virtual void PreUnloadCallback() // override
    {}

    // Called before the module is unloaded, right before the module object is destroyed.
    virtual void onModuleShutdown() // override
    {}

    virtual void destroyModule() { delete this; }

    virtual void onModuleMessageProc(qd::moduleMsg::BaseMsg& in_msg);

    virtual ~IModuleInterface() = default;


private:
    QD_PUSH_VC_WARNING(4201) // nameless struct/union
    struct t_StateFlags {
        union {
            struct {
                bool m_bModStartuped  :1;
                bool m_bModShutdowned :1;
                bool m_bModDestroyed  :1;
            };
            uint8_t m_Flag;
        };
        t_StateFlags()
            : m_Flag(0)
        {}
    }; // struct CLevelFlags
    QD_POP_VC_WARNING()

    t_StateFlags m_ModuleState;

}; // class IModuleInterface
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
