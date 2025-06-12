#pragma once
#include "qd/Base/base.h"
#include "qd/Base/classInfoReg.h"
#include "qd/Base/types.h"
#include "qd/Core/nodeBase.h"
#include "qd/Debug/assert.h"
#include "qd/STL/fixed_vector.h"
#include "qd/STL/string.h"
#include "qd/TypeSystem/typeDeclare.h"
#include "qd/UI/uiOperationManager.h"


namespace qd {
class GuiManager;
class UiOperation;


// Operation's component classId
enum class EOperationCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiOperationCreator : public qd::NodeCreator {};


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
    TS_ATTRIBUTE(qd::CreateClassCbAttr(&createUiOperationCb_<TRefClass>)); \
    TS_END();


//////////////////////////////////////////////////////////////////////////
class UiOperation : public qd::Node
{
    TS_REFLECT_CLASS_BASE(200, qd::UiOperation, qd::Node);

public:
    uint32_t mClassId = -1;
    qd::string m_name;
    qd::string m_description;
    bool m_bActive = true;

public:
    UiOperation() = default;
    virtual ~UiOperation() = default;


    virtual void onOperationCreate(qd::UiOperationCreator* cp) { onNodeCreated(cp); }

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


protected:
    EFlow _applyMsgProcDefImp(operation::msg::Base* pBaseMtd);

}; // class UiOperation
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
