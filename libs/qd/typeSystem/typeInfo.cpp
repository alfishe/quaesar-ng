#include "qd/typeSystem/typeInfo.h"
#include "qd/debug/assert.h"

//-------------------------------------------------------------------------

namespace qd {
bool TypeInfo::isDerivedFrom(const TypeInfo& type) const
{
    if (getStdTypeId() == type.getStdTypeId())
        return true;

    for (auto it = m_pBaseSuperTypes.begin(); it != m_pBaseSuperTypes.end(); ++it)
    {
        const TypeInfo* pCurBaseType = *it;
        if (pCurBaseType->getStdTypeId() == type.getStdTypeId())
            return true;
        if (!pCurBaseType->m_pBaseSuperTypes.empty())
        {
            if (pCurBaseType->isDerivedFrom(type)) // RECURSIVE
                return true;
        }
    }
    return false;
}


bool TypeInfo::checkDefined() const
{
    if (c_def(this) && isDefined())
        return true;
    QD_HALT("TypeInfo not defined");
    return false;
}


void TypeInfo::onTypeCreated()
{
    m_bDefined = true;
}


void TypeInfo::getInheritedProviders(/*_Out_*/ qtd::vector<const TypeInfoBase* >& out_list) const
{
    out_list.reserve(m_pBaseSuperTypes.size());
    for (TBaseSuperTypes::const_iterator i = m_pBaseSuperTypes.begin(); i != m_pBaseSuperTypes.end(); ++i)
    {
        const TypeInfo* pType = *i;
        const TypeInfoBase* pProvider = static_cast<const TypeInfoBase*>(pType);
        out_list.push_back(pProvider);
    }
}


}; // namespace qd
