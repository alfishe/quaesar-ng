#pragma once
#include "qd/base/eFlow.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/attributesCommon.h"
#include <qd/base/base.h>
//#include <qd/base/classIdCC.h>
#include <qd/stl/ref_ptr.h>
#include <qd/enum/enumBase.h>
#include <qd/node/node.h>


union SDL_Event;


namespace qd {
FORWARD_DECLARATION_2S(appMsg, BaseMsg);
class Application;
class AppPartsManager;


struct EAppPartMtd {
    enum Type {
        NONE = 0,
        UPDATE = 0x01,
        RENDER = 0x02,
    };
    ENUM_DECLARE_BASE(qd::, EAppPartMtd, Type, 0);
    ENUM_DECLARE_FLAGS();
}; // enum EPartMtd



//////////////////////////////////////////////////////////////////////////
// qd Application Parts
class ApplicationPart : public qd::Node
{
    TS_BEGIN_REFLECT_CLASS(qd::ApplicationPart, qd::Node);
    TS_END();
    friend class AppPartsManager;


protected:
    qtd::string m_PartName;
    EAppPartMtd m_Methods;
    float m_ZOrder = 0; // qd.AppParts.EBaseZOrder
    qd::Application* m_pApp = nullptr;
    int m_nUpdates = 0;
    bool m_bPartInit = false;
    bool m_bPartDone = false;
    bool m_bNeedRepaint = true;

public:
    ApplicationPart() { m_Methods += EAppPartMtd::UPDATE; }
    virtual ~ApplicationPart() override;

    struct OnCreate_t {
        qtd::string name;
        const qd::TypeInfo* typeInfo = nullptr;
        qd::Application* app = nullptr;
    };
    virtual void onPartCreate(ApplicationPart::OnCreate_t& prm);

    bool isNeedRepaint() const { return m_bNeedRepaint; }
    void setNeedRepaint(bool NeedRepaint) { m_bNeedRepaint = NeedRepaint; }

    const EAppPartMtd& getPartMtd() const { return m_Methods; }

    inline bool hasMtd(EAppPartMtd Mtd) const { return m_Methods.has(Mtd); }
        ApplicationPart& modifyPartMtd(EAppPartMtd SetMethods, EAppPartMtd ResetMethods = EAppPartMtd::NONE);

    virtual bool isReadyToActivate() const { return true; }

    bool isPartActive() const { return hasMtd(EAppPartMtd::UPDATE); }
    bool setPartActive(bool bActive);
    bool isPartRenderable() const { return hasMtd(EAppPartMtd::RENDER); }
    bool setPartRenderable(bool PartVisisble);

    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg);
    virtual qd::EFlow onSdlEventProc(SDL_Event& /*event*/) { return qd::EFlow::UNDEF; }

    void updateActivateTime() { m_nUpdates++; }

    qd::Application* getApp() const { return m_pApp; }
    void setApp(qd::Application* pApp) { m_pApp = pApp; }


    virtual void updateAppPart(float /*dt*/, float /*time*/) {}
    virtual void renderAppPart() {}
    virtual void postRenderAppPart() {}

    virtual void destroyImp() {}
    virtual void destroy() override;

    AppPartsManager* getAppParts() const;

    float getZOrder() const { return m_ZOrder; }

    void setZOrder(const float& zOrder);

    const qtd::string& getPartName() const { return m_PartName; }
    void setPartName(const qtd::string& PartName)
    {
        assert(!PartName.empty());
        m_PartName = PartName;
    }


    bool isPartInit() const { return m_bPartInit; }
    void setPartInit(bool Init) { m_bPartInit = Init; }
    bool isPartDone() const { return m_bPartDone; }
    void setPartDone(bool Done) { m_bPartDone = Done; }
}; // class ApplicationPart
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
