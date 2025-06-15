#pragma once
#include "qd/base/flowEnum.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/attributesCommon.h"
#include <qd/base/base.h>
//#include <qd/base/classIdCC.h>
#include <qd/base/ref_ptr.h>
#include <qd/enum/enumBase.h>
#include <qd/node/node.h>
#include <qd/typeSystem/reflectedType.h>


FORWARD_DECLARATION_3S(qd, appMsg, BaseMsg);
union SDL_Event;

namespace qd {
class Application;


struct EAppPartMtd {
    enum Type {
        NONE = 0,
        RENDER = (1 << 0),
        UPDATE = (1 << 1),
        UPDATE_WHILE_LOADING = (1 << 2),
    };
    ENUM_DECLARE_BASE(qd::, EAppPartMtd, Type, 0);
    ENUM_DECLARE_FLAGS;
}; // enum EPartMtd



//////////////////////////////////////////////////////////////////////////
// CrioGen Application Parts
class AppPartBase : public qd::Node
{
    TS_BEGIN_REFLECT_CLASS(qd::AppPartBase, qd::Node);
    TS_END();
    friend class AppPartsManager;


protected:
    qd::string m_PartName;
    EAppPartMtd m_Methods;
    float m_ZOrder = 0; // CrioGen.AppParts.EBaseZOrder
    qd::Application* m_pApp = nullptr;
    int m_nUpdates = 0;
    bool m_bPartInit = false;
    bool m_bPartDone = false;
    bool m_bNeedRepaint = true;

public:
    AppPartBase() { m_Methods += EAppPartMtd::UPDATE; }

    struct OnCreate_t {
        qd::string name;
        const qd::TypeInfo* typeInfo = nullptr;
        qd::Application* app = nullptr;
    };
    virtual void onPartCreate(AppPartBase::OnCreate_t& prm);

    bool isNeedRepaint() const { return m_bNeedRepaint; }
    void setNeedRepaint(bool NeedRepaint) { m_bNeedRepaint = NeedRepaint; }

    const EAppPartMtd& getPartMtd() const { return m_Methods; }

    inline bool hasMtd(EAppPartMtd Mtd) const { return m_Methods.has(Mtd); }

    inline AppPartBase& modifyPartMtd(EAppPartMtd SetMethods, EAppPartMtd ResetMethods = EAppPartMtd::NONE)
    {
        m_Methods -= ResetMethods;
        m_Methods += SetMethods;
        return *this;
    }

    virtual bool isReadyToActivate() const { return true; }

    bool isPartActive() const { return hasMtd(EAppPartMtd::UPDATE); }
    bool setPartActive(bool bActive);

    bool isPartVisible() const { return hasMtd(EAppPartMtd::RENDER); }
    bool setPartVisisble(bool PartVisisble);


    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg);

    void updateActivateTime() { m_nUpdates++; }

    Application* getApp() const { return m_pApp; }
    void setApp(Application* pApp) { m_pApp = pApp; }


    virtual void update(float dt, float time) {}
    virtual void render() {}

    virtual void onSdlEventProc(SDL_Event& event) {}

    virtual void postRender() {}

    virtual void destroyImp() {}
    void destroy();

    AppPartsManager* getAppParts() const;

    float getZOrder() const { return m_ZOrder; }

    void setZOrder(const float& zOrder);

    const qd::string& getPartName() const { return m_PartName; }
    void setPartName(const qd::string& PartName)
    {
        assert(!PartName.empty());
        m_PartName = PartName;
    }

    virtual ~AppPartBase(void) {}

    bool isPartInit() const { return m_bPartInit; }
    void setPartInit(bool Init) { m_bPartInit = Init; }
    bool isPartDone() const { return m_bPartDone; }
    void setPartDone(bool Done) { m_bPartDone = Done; }
}; // class AppPartBase
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
