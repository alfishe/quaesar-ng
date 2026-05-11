#pragma once
#include "qd/base/base.h"
#include "qd/mem/fnvHash.h"
#include "qd/stl/vector.h"
#include "qd/typeSystem/stdTypeId.h"


namespace qd {
class TypeInfo;
class TypeInfoAttribute;
class TypeInfoBase;
class TypeRegistry;


struct TypeInfoMsgBase {
    int id = -1;
    TypeInfoMsgBase(int _id)
        : id(_id) {}
};

//------------------------------------------------------------------------
// BASE CLASS FOR ATTRIBUTES HANDLING
class TypeInfoBase
{
    qtd::vector<TypeInfoAttribute*> m_pAttributes;
    typedef qtd::vector<TypeInfoAttribute*> TAttrList;
    friend struct TypeInfoBuilder;

public:
    void broadcastReflectionEventMsg(qd::TypeInfoMsgBase* in_msg);

    const TypeInfoAttribute* findAttribute(const StdTypeId& Type, bool inherit) const;
    const TypeInfoAttribute* findAttribute(const TypeInfo& Type, bool inherit) const;

    template<typename T>
    const T* findAttribute_(bool find_in_inherit = false) const;

    const TypeInfoBase::TAttrList& getAttributes() const { return m_pAttributes; }
    void deleteCustomAttribute(const TypeInfoAttribute* pAttr);

protected:
    virtual ~TypeInfoBase();

    virtual void getInheritedProviders(qtd::vector<const TypeInfoBase* >& Providers) const = 0;
    virtual void addCustomAttribute(TypeInfoAttribute* pAttr);

}; // class TypeInfoBase
//////////////////////////////////////////////////////////////////////////



void validateStaticTypeInfoPtr(qd::TypeInfo const* pPtr);
const qd::TypeInfo& get_type_info(const qd::StdTypeId& ti);


template<typename T>
const qd::TypeInfo& typeof_() {
    static const qd::TypeInfo* staticType = nullptr;
    if (!staticType) {
        constexpr StdTypeId ti = qd::makeStdTypeId_<T>();
        staticType = &qd::get_type_info(ti);
    }
    return *staticType;
}


template<typename T>
const qd::TypeInfo& typeof_(T) {
    return qd::typeof_<T>();
}


template<typename T>
inline const qd::TypeInfo& type_of(const T* pInst) { // typeof is reserved for gcc extension
    if (pInst)
        return pInst->getTypeInfo();
    return qd::typeof_<void>();
}


extern const qd::TypeInfo& typeof_by_name(const char* pClass);


constexpr THash32 hash_type_info_name(const char* class_name) {
    return qd::fnv1aHash(class_name);
}


constexpr THash32 hash_type_info_name(const char* class_name, size_t len) {
    return qd::fnv1aHash2(class_name, (uint32_t)len);
}

}; // namespace qd
