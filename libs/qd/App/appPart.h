#pragma once
#include <qd/Base/classIdCC.h>
#include <qd/Base/base.h>
#include <qd/Base/ref_ptr.h>
#include "qd/STL/string.h"
#include <qd/Enum/enumBase.h>
#include <qd/Math/fixedPoint.h>
#include <qd/TypeSystem/ReflectedType.h>
#include <qd/Core/nodeBase.h>


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
};  // enum EPartMtd



//////////////////////////////////////////////////////////////////////////
namespace EAppPartEvent {

enum eType {
    CUSTOM1 = 1000,
};

struct Msg {
    uint32_t m_MsgId;
    Msg(uint32_t msgId = ~0u) : m_MsgId{msgId} {
    }
};

template <uint32_t TId>
struct Msg_ : public Msg {
    static constexpr uint32_t ID = TId;
    Msg_() : Msg{TId} {}
};

static constexpr int COUNTER_BASE = (__COUNTER__);
#define PAUTO_ID (__COUNTER__ - COUNTER_BASE)


struct ON_ACTIVE_CHANGE : EAppPartEvent::Msg_<PAUTO_ID> {
    bool m_bActive = false;
};
struct ON_VISIBLE_CHANGE : EAppPartEvent::Msg_<PAUTO_ID> {
    bool m_bVisible = false;
};
struct ON_Z_ORDER_CHANGE : EAppPartEvent::Msg_<PAUTO_ID> {
    float m_ZOrder = 0;
};
struct IS_NEED_REPAINT : EAppPartEvent::Msg_<PAUTO_ID> {
    bool m_bNeedRepaint = false;
};
struct ON_PRE_DESTROY : EAppPartEvent::Msg_<PAUTO_ID> {};

struct RENDER_IMGUI_DEBUG_INFO_TREE : EAppPartEvent::Msg_<PAUTO_ID> {
    //ImAPI::CImGuiBase* pIm = nullptr;
};

#undef PAUTO_ID
};  // namespace EAppPartEvent
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CrioGen Application Parts
class BaseAppPart : public qd::Node {
    TS_REFLECT_CLASS(qd::BaseAppPart, qd::Node);
    friend class AppPartsManager;

protected:
    qd::string m_PartName;
    EAppPartMtd m_Methods;
    float m_ZOrder = 0;  // CrioGen.AppParts.EBaseZOrder
    Application* m_pApp;
    int m_nUpdates = 0;
    bool m_bPartInit = false;
    bool m_bPartDone = false;
    bool m_bNeedRepaint = true;

public:
    BaseAppPart(Application* pApp = nullptr) : m_pApp(pApp) {
        m_Methods += EAppPartMtd::UPDATE;
    }

    bool isNeedRepaint() const {
        return m_bNeedRepaint;
    }
    void setNeedRepaint(bool NeedRepaint) {
        m_bNeedRepaint = NeedRepaint;
    }

    const EAppPartMtd& getPartMtd() const {
        return m_Methods;
    }

    inline bool hasMtd(EAppPartMtd Mtd) const {
        return m_Methods.has(Mtd);
    }

    inline void modifyPartMtd(EAppPartMtd SetMethods,
                              EAppPartMtd ResetMethods = EAppPartMtd::NONE) {
        m_Methods -= ResetMethods;
        m_Methods += SetMethods;
    }

    virtual bool isReadyToActivate() const {
        return true;
    }

    bool isPartActive() const {
        return hasMtd(EAppPartMtd::UPDATE);
    }

    bool setPartActive(bool bActive);

    bool isPartVisible() const {
        return hasMtd(EAppPartMtd::RENDER);
    }

    bool setPartVisisble(bool PartVisisble);


    virtual void onAppPartMsgProc(EAppPartEvent::Msg* pMtd) {
        switch (pMtd->m_MsgId) {
            case EAppPartEvent::ON_VISIBLE_CHANGE::ID:
                // ptr<EPartEvent::ON_VISIBLE_CHANGE_t> p(pData);
                break;
            default:
                break;
        }
    }


    void updateActivateTime() {
        m_nUpdates++;
    }

    Application* getApp() const {
        return m_pApp;
    }
    void setApp(Application* pApp) {
        m_pApp = pApp;
    }


    virtual void update(CFixed32 Delta, CFixed32 Time)
    {
    }

    virtual void render()
    {
    }

    virtual void postRender() {
    }

    virtual void destroy() {
    }


    AppPartsManager* getAppParts() const;

    float getZOrder() const {
        return m_ZOrder;
    }

    void setZOrder(const float& zOrder) {
        if (m_ZOrder == zOrder)
            return;
        EAppPartEvent::ON_Z_ORDER_CHANGE p;
        p.m_ZOrder = zOrder;
        onAppPartMsgProc(&p);
        m_ZOrder = zOrder;
    }

    const string& getPartName() const {
        return m_PartName;
    }
    void setPartName(const string& PartName) {
        assert(!PartName.empty());
        m_PartName = PartName;
    }

    virtual ~BaseAppPart(void) {
    }

    bool isPartInit() const {
        return m_bPartInit;
    }
    void setPartInit(bool Init) {
        m_bPartInit = Init;
    }
    bool isPartDone() const {
        return m_bPartDone;
    }
    void setPartDone(bool Done) {
        m_bPartDone = Done;
    }
};  // class BasePart
//////////////////////////////////////////////////////////////////////////


};  // namespace qd 
