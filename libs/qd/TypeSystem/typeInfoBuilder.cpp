#include "typeInfoBuilder.h"
#include <qd/TypeSystem/TypeRegistry.h>
#include <qd/TypeSystem/TypeInfo.h>
#include "qd/Mem/fnvHash.h"


namespace qd {


 TypeInfoBuilder::TypeInfoBuilder(const StdTypeId& type_info, TypeRegistry* p_registry)
    : m_pRegistry(p_registry)
{
     if (!m_pRegistry)
         m_pRegistry = TypeRegistry::get();
     const TypeInfo& tp = m_pRegistry->get()->getTypeInfo(type_info);
     m_pType = const_cast<TypeInfo*>(&tp);
}


 void split_qualified_name(_In_ const string_view& full_name, _Out_ string_view& out_type_name,
    _Out_ string_view& out_namespace)
{
    int templ = 0;
    size_t lastSplitPoint = string::npos;
    size_t j = 0;

    size_t nLen = full_name.size();
    for (size_t i = 0; i < nLen; ++i, ++j)
    {
        if (full_name[i] == '<')
            ++templ;
        if (full_name[i] == '>')
            --templ;
        if (templ == 0)
        {
            if (full_name[i] == ':' && ((i + 1) != nLen) && (full_name[i + 1] == ':'))
                lastSplitPoint = j;
        }
    }

    if (lastSplitPoint == string::npos)
    {
        out_type_name = full_name;
        out_namespace = "";
    }
    else
    {
        out_type_name = full_name.substr(lastSplitPoint + 2);
        out_namespace = full_name.substr(0, lastSplitPoint);
    }
}


void TypeInfoBuilder::declareType(const char* full_name) const
{
    assert(!m_pType->isDefined() && "Builded type_info already registered!");
    m_pType->m_cid = qd::fnv1aHash(full_name);
    m_pType->m_fullName = full_name;
    split_qualified_name(m_pType->m_fullName, m_pType->m_shortName, m_pType->m_namespace);
    m_pType->m_bDefined = true;
    m_pRegistry->bindNamedTypeInfo(*m_pType);
}


void TypeInfoBuilder::addBaseType(const StdTypeId& baseType)
{
    const TypeInfo& pBaseType = m_pRegistry->getTypeInfo(baseType);
    assert(!(m_pType->getStdTypeId() == baseType) && "ERROR: Object can't be inherited from itself");
    m_pType->m_pBaseSuperTypes.push_back(&pBaseType);
}


void TypeInfoBuilder::markAsBase(int inherited_count) const
{

}


void TypeInfoBuilder::markAsFinal(TypeInfo* pType) const
{
    m_pType->m_bFinal = true;
}


void TypeInfoBuilder::setup(const TypeInfo* pType, const char* name)
{
    assert(0);
}


void TypeInfoBuilder::addBaseAttribute(TypeInfoBase* pParentType, TypeInfoAttribute* pAttr)
{
    pParentType->addCustomAttribute(pAttr);
}


void validateStaticTypeInfoPtr(TypeInfo const* pPtr)
{
    assert(pPtr && "Invalid TypeInfo Ptr");
}


}; // namespace qd
