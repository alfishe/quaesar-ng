#pragma once
#include <qd/typeSystem/stdTypeId.h>
#include "qd/stl/vector.h"
#include "qd/typeSystem/typeInfoBuilder.h"


namespace qd {
class TypeInfo;
class TypeInfoBase;
class TypeInfoAttribute;


struct TypeInfoMsgBase {
    int id = -1;
    TypeInfoMsgBase(int _id)
        : id(_id)
    {}
};

//------------------------------------------------------------------------
// BASE CLASS FOR ATTRIBUTES HANDLING
class TypeInfoBase
{
    eastl::vector<TypeInfoAttribute*> m_pAttributes;
    typedef eastl::vector<TypeInfoAttribute*> TAttrList;
    friend struct TypeInfoBuilder;

public:
    void broadcastReflectionEventMsg(qd::TypeInfoMsgBase* in_msg);

    const TypeInfoAttribute* getAttribute(const StdTypeId& Type, bool inherit) const;
    const TypeInfoAttribute* getAttribute(const TypeInfo& Type, bool inherit) const;

    template<typename T>
    const T* getAttribute_(bool find_in_inherit = false) const
    {
        const TypeInfoAttribute* pAttr = getAttribute(qd::typeof_<T>(), find_in_inherit);
        if (!pAttr)
            return nullptr;
        assert(dynamic_cast<const T*>(pAttr) && "Attribute Type mismatch");
        return static_cast<const T* >(pAttr);
    }

    const TypeInfoBase::TAttrList& getAttributes() const { return m_pAttributes; }
    void deleteCustomAttribute(const TypeInfoAttribute* pAttr);

protected:
    virtual ~TypeInfoBase();

    virtual void getInheritedProviders(eastl::vector<const TypeInfoBase* >& Providers) const = 0;
    virtual void addCustomAttribute(TypeInfoAttribute* pAttr);

}; // class TypeInfoBase
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
