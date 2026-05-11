#include "typeInfoBase.h"
#include <qd/typeSystem/typeInfo.h>
#include <qd/typeSystem/typeInfoAttrBase.h>
#include "qd/stl/algorithm.h"


namespace qd {


void TypeInfoBase::addCustomAttribute(TypeInfoAttribute* pAttr)
{
    assert(pAttr && pAttr->getTypeId().isValid());
    if (pAttr)
    {
        pAttr->m_pParent = this;
        m_pAttributes.push_back(pAttr);
    }
}


void TypeInfoBase::broadcastReflectionEventMsg(TypeInfoMsgBase* in_msg)
{
    for (TypeInfoAttribute* pCurAttr : m_pAttributes)
        pCurAttr->onReflectionEventMsgProc(in_msg);
}


void TypeInfoBase::deleteCustomAttribute(const TypeInfoAttribute* pAttr)
{
    TAttrList::iterator Iter = qtd::find(m_pAttributes.begin(), m_pAttributes.end(), pAttr);
    if (Iter != m_pAttributes.end())
    {
        m_pAttributes.erase(Iter);
        delete pAttr;
    }
}


const TypeInfoAttribute* TypeInfoBase::findAttribute(const StdTypeId& rfAttrType, bool find_in_inherit) const
{
    for (TAttrList::const_iterator It = m_pAttributes.begin(); It != m_pAttributes.end(); ++It)
    {
        const TypeInfoAttribute* pCurAttr = *It;
        if (pCurAttr->getTypeId() == rfAttrType)
            return *It;
    }

    if (find_in_inherit)
    {
        qtd::vector<const TypeInfoBase* > providers;
        getInheritedProviders(providers);
        for (auto i = providers.begin(); i != providers.end(); ++i)
        {
            const TypeInfoBase* pProvider = *i;
            const TypeInfoAttribute* ca = pProvider->findAttribute(rfAttrType, true);
            if (ca)
                return ca;
        }
    }
    return nullptr;
}



const qd::TypeInfoAttribute* TypeInfoBase::findAttribute(const TypeInfo& type, bool inherit) const
{
    return findAttribute(type.getStdTypeId(), inherit);
}


TypeInfoBase::~TypeInfoBase()
{
    for (TAttrList::iterator i = m_pAttributes.begin(); i != m_pAttributes.end(); ++i)
    {
        const TypeInfoAttribute* pAttr = *i;
        delete pAttr;
    }
}


}; // namespace qd
