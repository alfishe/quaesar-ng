#pragma once
#include <qd/typeSystem/typeDeclare.h>


namespace qd
{


//------------------------------------------------------------------------
// BASE ATTRIBUTE
class TypeInfoAttribute
{
    TS_REFLECT_CLASS_BASE(100, qd::TypeInfoAttribute, void);

protected:
    StdTypeId m_TypeId;
    const TypeInfoBase* m_pParent = nullptr;
    friend class TypeInfoBase;

public:
    virtual ~TypeInfoAttribute() = default;
    const StdTypeId& getTypeId() const { return m_TypeId; }
    void setTypeId(const StdTypeId& TypeId) { m_TypeId = TypeId; }

    virtual void onReflectionEventMsgProc(qd::TypeInfoMsgBase* in_msg) {}

}; // class TypeInfoAttribute
//////////////////////////////////////////////////////////////////////////

	
	
}; // namespace qd
