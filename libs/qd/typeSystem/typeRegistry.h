#pragma once
#include <EASTL/span.h>
#include <EASTL/vector_map.h>
#include <qd/typeSystem/stdTypeId.h>
#include <qd/typeSystem/TypeID.h>
#include <qd/typeSystem/typeInfo.h>


//-------------------------------------------------------------------------

namespace qd {
class IReflectedType;
class TypeInfo;
class EnumInfo;
class PropertyInfo;
class PropertyPath;
class TypeInfoSpan;
struct ResourceInfo;
struct DataFileInfo;



struct CTypeInfoCmp {
    inline bool operator() (const std::type_info* t1, const std::type_info* t2) const { return t1->before(*t2) != 0; }
}; // struct CTypeInfoCmp

typedef eastl::vector_map<const std::type_info*, TypeInfo*, CTypeInfoCmp> TypeInfoMap;
// typedef eastl::vector_map<CGuid32, eastl::fixed_vector<TypeInfo*, 1, true>> TypeMapByGuid;
//------------------------------------------------------------------------



//-------------------------------------------------------------------------

class TypeRegistry
{
    struct SharedData;
    TypeRegistry::SharedData* m_pSharedData = nullptr;
    const TypeInfo* m_pVoidType = nullptr;

public:
    TypeRegistry();
    ~TypeRegistry();

    static TypeRegistry* get();
    SharedData* getSharedData() const;
    const TypeInfo& getTypeInfo(const StdTypeId& ti, bool bReplaceIfDefined = false) const;
    void bindNamedTypeInfo(const TypeInfo& type_info);

    inline const TypeInfoMap& getTypesMap();

    // Finds all inherited classes from the current
    // BRUTEFORCE may be very slowly
    eastl::vector<const TypeInfo*> findAllDerivedFromTypes(const TypeInfo& rBaseType, bool bIncludeBaseInList = false);

    template<class TBaseClass>
    static TypeInfoSpan findAllDerivedFromTypesCached_(bool bIncludeBaseInList = false);

    const qd::TypeInfo* findTypeByName(const char* pName) const;
    const qd::TypeInfo& getTypeByName(const char* pName) const;


protected:
    const TypeInfo& _createUnNamedTypeInfoByStdType(const StdTypeId& ti);
    void _createSharedData() const;
}; // class TypeRegistry
//////////////////////////////////////////////////////////////////////////



// ARRAY OF REFLECTION TYPES
class TypeInfoSpan : public eastl::span<const TypeInfo* >
{
    using TType = const TypeInfo*;
    using TSuper = eastl::span<TType>;

public:
    using TSuper::TSuper; // base constructor

    template<class TAttr>
    const TypeInfo* findTypeByAttrValue(const TAttr& attr, bool inherit = false) const
    {
        const TypeInfo& attrType = typeof_<TAttr>();
        for (const TypeInfo* curType : *this)
        {
            const TypeInfoAttribute* foundBaseAttr = curType->findCustomAttribute(attrType, inherit);
            if (!foundBaseAttr)
                continue;
            TAttr* foundAttr = static_cast<TAttr*>(foundBaseAttr);
            if (attr == *foundAttr) // operator ==
                return curType;
        }
        return nullptr;
    }


}; // class ReflectionTypesSpan
//////////////////////////////////////////////////////////////////////////




struct TypeRegistry::SharedData {
    TypeInfoMap m_TypeMap;
    const TypeInfo* m_pTypeVoid = nullptr;
    eastl::vector_map<THash32, const TypeInfo* > m_TypeByFullName; // Hash from full name
    typedef eastl::vector_map<THash32, const TypeInfo* > TTypeByFullNameMap;

    SharedData() = default;
    ~SharedData();

}; // struct
//////////////////////////////////////////////////////////////////////////



template<class TBaseClass>
TypeInfoSpan TypeRegistry::findAllDerivedFromTypesCached_(bool bIncludeBaseInList /* = false*/)
{
    static eastl::vector<const TypeInfo*> derivedClasses; // CACHED CLASSES
    if (derivedClasses.empty())
    {
        derivedClasses.clear(); // for debug
        const TypeInfo& refBaseType = qd::typeof_<TBaseClass>();
        TypeRegistry* pReflection = get();
        derivedClasses = pReflection->findAllDerivedFromTypes(refBaseType, bIncludeBaseInList);
        assert(!derivedClasses.empty() && "No reflect declared class found");
    }
    return TypeInfoSpan(derivedClasses);
}



}; // namespace qd
