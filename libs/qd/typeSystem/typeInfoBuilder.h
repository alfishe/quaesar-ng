#pragma once
#include <qd/base/base.h>
#include <qd/typeSystem/stdTypeId.h>
#include <qd/typeSystem/typeInfoBase.h>
#include "qd/mem/fnvHash.h"



//////////////////////////////////////////////////////////////////////////
namespace qd {
class TypeInfoBase;
class TypeInfo;
class TypeRegistry;
class TypeInfoAttribute;

void validateStaticTypeInfoPtr(qd::TypeInfo const* pPtr);
const TypeInfo& getTypeInfo(const StdTypeId& ti);


template<typename T>
inline const TypeInfo& typeof_()
{
    static const qd::TypeInfo* staticType = nullptr;
    if (!staticType)
    {
        constexpr StdTypeId ti = qd::makeStdTypeId_<T>();
        staticType = &qd::getTypeInfo(ti);
    }
    return *staticType;
}

template<typename T>
inline const TypeInfo& typeof(T pInst)
{
    return pInst.getTypeInfo();
}


constexpr THash32 hash_type_info_name(const char* class_name)
{
    return qd::fnv1aHash(class_name);
}

constexpr THash32 hash_type_info_name(const char* class_name, size_t len)
{
    return qd::fnv1aHash(class_name, (uint32_t)len);
}

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
    void addAttribute_(TypeInfoBase* pParentType, TAttr* pAttr)
    {
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

    TypeInfoBuilder_(const char* fullName, bool bAbstract = false)
        : TSuper(qd::makeStdTypeId_<T>())
    {
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
        : TypeInfoBuilder_<T>(fullName, false)
    {}
}; // class TypeInfoBuilderObject_
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
