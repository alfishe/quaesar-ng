#pragma once
#include "qd/base/base.h"
#include "qd/base/classInfoReg.h"
#include "qd/base/types.h"
//#include "qd/node/node.h"
#include "qd/debug/assert.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/uiOperationMessages.h"


namespace qd {
class DbgGuiDesktop;
class UiOperation;
class UiOperationMgr;


// Operation's component classId
enum class EOperationCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiOperationCreator {
    UiOperationMgr* uiOpsMgr = nullptr;
    uint32_t id = 0;

    template<class TClass, typename... TArgs>
    TClass* make_(TArgs&&... args)
    {
        TClass* pNode = new TClass(args...);
        pNode->onNodeCreated(this);
        return pNode;
    } // make_
};


class OperationHistory
{
public:
};


template<class TClass>
static qd::UiOperation* createUiOperationCb_(const qd::TypeInfo& /*meta*/, qd::UiOperationCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onOperationCreate(cp);
    pNewInst->setup();
    return pNewInst;
}

#define QD_REG_OPERATION(ClassName)                                        \
    TS_BEGIN_REFLECT_CLASS(ClassName, qd::operation::Operation);           \
    TS_ATTRIBUTE(qd::ts::attr::CreateClassCb(&createUiOperationCb_<TRefClass>)); \
    TS_END();


//////////////////////////////////////////////////////////////////////////
class UiOperation : public qd::RefCounted
{
    TS_REFLECT_CLASS_BASE(200, qd::UiOperation, qd::RefCounted);

public:
    uint32_t mClassId = -1;
    qd::string m_name;
    qd::string m_description;
    bool m_bActive = true;
    ref_ptr<ShortcutHnd> m_pShortcuts;
public:
    UiOperation() = default;
    virtual ~UiOperation() = default;

    virtual void onOperationCreate(qd::UiOperationCreator* cp) { /*onNodeCreated(cp);*/ }

    virtual void destroy() {}

    void doOperationBase();

    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) { return _applyMsgProcDefImp(msg); }

    void addShortcut(int sid);

    virtual bool hasMtd(int id) const { return false; /*supportMtd[id];*/ }

    virtual void doOperation(qd::OperationHistory* history = nullptr)
    {
        operation::msg::DoOperation msg;
        applyOperationMsgProc(&msg);
    }
    virtual void undoOperation(OperationHistory& history) {}

    bool isActive() const { return m_bActive; }
    void setActive(bool Active) { m_bActive = Active; }

    const qd::string& getName() const { return m_name; }

    qd::ShortcutHnd* getShortcuts() const { return m_pShortcuts; }


protected:
    EFlow _applyMsgProcDefImp(operation::msg::Base* pBaseMtd);

}; // class UiOperation
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
