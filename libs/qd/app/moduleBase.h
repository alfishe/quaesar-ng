#include <qd/base/base.h>
#include <qd/enum/enumBase.h>
#include <qd/base/ref_ptr.h>



namespace qd
{
class ModuleManager;
class Application;


struct ECgModuleID {
    typedef uint32_t EType;
    //G_ENUM_TO_STRING(qd::ECgModuleID);

    // DON'T FORGET REGENRATE FILE 'qdEnumGenerated.cpp' with 'qdEnumGenerate.bat'
    enum eType {
        UNKNOWN = -1,
        NONE = 0,
        APP_PARTS,

        _FAST_ACCESS_,

        USER_MODULE /*=100*/,
    };
    ENUM_DECLARE_BASE(qd::, ECgModuleID, eType, 0);
    //ENUM_DECLARE_TO_STRING_BASE();

};  // enum ECgModuleID
//////////////////////////////////////////////////////////////////////////



	struct ModuleCreateParams {
	    uint32_t classId = 0;
	    ModuleManager* moduleMgr = nullptr;
	    Application* app = nullptr;
	    ModuleCreateParams(Application* p_app = nullptr) : app(p_app) {}
        virtual ~ModuleCreateParams() = default;
	};  // struct CModuleCreateParams
	//////////////////////////////////////////////////////////////////////////




// MODULE INNER MESSAGES
    namespace Enm {
    namespace EModuleMsg {
    enum eType {
        UNKNOWN = 0,
        RENDER_IMGUI_DEBUG_INFO_TREE,
        _COUNT_,
    };  // enum eType

    struct Msg_t {
        ENUM_DECLARE_BASE(qd::Enm::EModuleMsg::, Msg_t, int, 0);
    };

    struct RENDER_IMGUI_DEBUG_INFO_TREE_t {};

    };  //namespace EModuleMsg
    }; // namespace EN





//////////////////////////////////////////////////////////////////////////
    // BASE MODULE INTEFACE
    class IModuleInterface : public RefCounted {
        friend class ModuleManager;

    public:
        static qd::ECgModuleID getModuleTypeId() {
            assert(0);
            return qd::ECgModuleID::UNKNOWN;
        };

    public:
        IModuleInterface(qd::ModuleCreateParams* pCP = nullptr) {
        }

        // Called right after the module DLL has been loaded and the module object has been created
        virtual void onModuleStartup(qd::ModuleCreateParams* mc)  // override
        {
        }

        // Called before the module has been unloaded
        virtual void PreUnloadCallback()  // override
        {
        }

        // Called before the module is unloaded, right before the module object is destroyed.
        virtual void onModuleShutdown()  // override
        {
        }

        virtual void destroyModule()  // override
        {
            //delete this;
        }

        virtual void onModuleMessageProc(qd::Enm::EModuleMsg::Msg_t MsgId, void* pMsgData = nullptr);


        virtual ~IModuleInterface() = default;


    private:
        struct t_StateFlags {
            union {
                struct {
                    bool m_bModStartuped : 1;
                    bool m_bModShutdowned : 1;
                    bool m_bModDestroyed : 1;
                };
                uint8_t m_Flag;
            };
            t_StateFlags() : m_Flag(0) {
            }
        };  // struct CLevelFlags

        t_StateFlags m_ModuleState;


    public:
        qd::ECgModuleID m_CGModuleTypeID;

    };  // class IModuleInterface
    //////////////////////////////////////////////////////////////////////////



}; // namespace qd
