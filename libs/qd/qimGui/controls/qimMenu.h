#pragma once
#include "EASTL/fixed_function.h"
#include "qd/qimGui/qimGui.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_2(qd, UiOperation);


namespace qim {
class UiMenuBeh;


#define QIM_ELEMENT_CLASS(ElemClass, BaseClass, BehClass)                   \
    TS_REFLECT_CLASS(ElemClass, BaseClass);                                 \
    friend class BehClass;                                                  \
    using TBehClass = BehClass;                                             \
    inline static const qd::TypeInfo& s_behClass = qd::typeof_<BehClass>(); \
    virtual const qd::TypeInfo* getBehaviorClass() const override           \
    {                                                                       \
        return &ElemClass::s_behClass;                                      \
    }                                                                       \
                                                                            \
public:




struct UiMenu : public qim::ElementData {
    QIM_ELEMENT_CLASS(qim::UiMenu, qim::ElementData, qim::UiMenuBeh);

    void setup(const char* text)
    { //
        m_text = text;
    }

    virtual void onBegin(qim::Context* ctx) override;
    virtual void onEnd(qim::Context* ctx) override;

    bool isOpen() const { return m_isOpen; }
    void setText(const char* text) { m_text = text; }
    const char* getText() const { return m_text.c_str(); }

private:
    qd::string m_text;
    bool m_isOpen = false;

}; // struct UiMenu
//////////////////////////////////////////////////////////////////////////



struct UiMenuItem : public qim::ElementData {
    QIM_ELEMENT_CLASS(qim::UiMenuItem, qim::ElementData, qim::UiMenuBeh);
private:
    eastl::fixed_function<16, void()> m_onClickCb;
    qd::string m_text;

public:
    void setup(const char* text)
    { //
        m_text = text;
    }

    virtual void onEnd(qim::Context* ctx) override
    {
        auto pMenu = ctx->getStackTreeTop()->cast_<qim::UiMenu>();
        assert(pMenu);
        if (!pMenu || !pMenu->isOpen())
            return;
        bool bClick = ImGui::MenuItem(m_text.c_str());
        if (bClick && m_onClickCb)
            m_onClickCb();
        m_onClickCb = {};
    }

    void setText(const char* text) { m_text = text; }
    const char* getText() const { return m_text.c_str(); }

    template<typename TFunc>
    void onClick(TFunc cb)
    {
        m_onClickCb = cb;
    }

}; // struct UiMenuItem
//////////////////////////////////////////////////////////////////////////



class UiMenuBeh : public qim::ElementBeh
{
    TS_BEGIN_REFLECT_CLASS(qim::UiMenuBeh, qim::ElementBeh);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();


public:
    ElementData* createElementData(const qd::TypeInfo& type) override;

}; // class UiMenuBeh
//////////////////////////////////////////////////////////////////////////


class UiMenuOperationBeh;



struct UiMenuOperation : public qim::ElementData {
    QIM_ELEMENT_CLASS(qim::UiMenuOperation, qim::ElementData, qim::UiMenuOperationBeh);

    void setup(const char* operation_class_name);
    virtual void onEnd(qim::Context* ctx) override;

private:
    qd::UiOperation* m_pOperation = nullptr;
    const qd::TypeInfo* m_pOperationType = nullptr;

}; // struct UiMenuOperation



class UiMenuOperationBeh : public qim::ElementBeh
{
    TS_BEGIN_REFLECT_CLASS(qim::UiMenuOperationBeh, qim::ElementBeh);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();

public:
    ElementData* createElementData(const qd::TypeInfo& type) override { return new UiMenuOperation(); }

}; // class UiMenuOperationBeh


}; // namespace qim
