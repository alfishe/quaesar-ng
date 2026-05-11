#pragma once
#include "qd/mem/fnvHash.h"
#include <qd/base/base.h>
#include <qd/typeSystem/stdTypeId.h>
#include <qd/typeSystem/typeInfoBase.h>



//////////////////////////////////////////////////////////////////////////
namespace qd {


//////////////////////////////////////////////////////////////////////////
struct TypeInfoBuilder {
    TypeInfo* m_pType;
    TypeRegistry* m_pRegistry;

public:
    TypeInfoBuilder(const StdTypeId& type_info, TypeRegistry* p_pregistry = nullptr);
    void declareType(const char* name) const;
    void addBaseType(const StdTypeId& baseType);
    void markAsBase(int inherited_count = 16) const;
    void markAsFinal(TypeInfo* pType) const;
    void setup(const TypeInfo* pType, const char* name);
    const qd::TypeInfo* getTypeInfo() const { return m_pType; }
    void addBaseAttribute(TypeInfoBase* pParentType, TypeInfoAttribute* pAttr);

    template<class TAttr>
    void addAttribute_(TypeInfoBase* pParentType, TAttr* pAttr) {
        pAttr->setTypeId(makeStdTypeId_<TAttr>());
        return addBaseAttribute(pParentType, pAttr);
    }

}; // struct
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
template<typename T>
struct TypeInfoBuilder_ : public TypeInfoBuilder {
    typedef TypeInfoBuilder TSuper;
    typedef TypeInfoBuilder_<T> TThis;

public:
    using TRefType = T;

    TypeInfoBuilder_(const char* fullName, bool /*bAbstract*/ = false)
        : TSuper(qd::makeStdTypeId_<T>()) {
        declareType(fullName);
    }

}; // class TypeInfoBuilder_
//////////////////////////////////////////////////////////////////////////



template<typename T>
struct TypeInfoBuilderObject_ : public TypeInfoBuilder_<T> {
public:
    typedef T TRefClass;
    typedef TypeInfoBuilderObject_<T> Inherited;

    TypeInfoBuilderObject_(const char* fullName)
        : TypeInfoBuilder_<T>(fullName, false) {}
}; // class TypeInfoBuilderObject_
//////////////////////////////////////////////////////////////////////////



template<typename T>
const T* TypeInfoBase::findAttribute_(bool find_in_inherit /*= false*/) const {
    const TypeInfoAttribute* pAttr = findAttribute(qd::typeof_<T>(), find_in_inherit);
    if (!pAttr)
        return nullptr;
    assert(dynamic_cast<const T*>(pAttr) && "Attribute Type mismatch");
    return static_cast<const T*>(pAttr);
}

}; // namespace qd
