#pragma once
#include <qdIce/qdTypeSystem/TypeId.h>
#include <qdIce/qdTypeSystem/typeInfoBase.h>
#include "qdIce/qdSTL/vector.h"
#include <qdIce/qdBase/stringId.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdTypeSystem/stdTypeId.h>


namespace qd {


class TypeInfo : public TypeInfoBase
{
    TypeId m_id;
    StdTypeId m_stdTypeId;

    qd::string m_fullName;
    qd::string_view m_shortName;
    qd::string_view m_namespace;
    THash32 m_cid = 0; // =hash(m_fullName)

    int32_t m_size = -1;
    int32_t m_alignment = -1;
    bool m_isAbstract = false;
    bool m_bDefined = false;
    bool m_bFinal = false;

    // parents (super) classes of this type
    qd::vector<const TypeInfo*> m_pBaseSuperTypes;
    typedef qd::vector<const TypeInfo*> TBaseSuperTypes;

    friend class TypeRegistry;
    friend struct TypeInfoBuilder;

public:

    const qd::string& getFullName() const {
        return m_fullName;
    }

    // Basic Type Info
    //-------------------------------------------------------------------------

    const TypeId& getId() const { return m_id; }
    const StdTypeId& getStdTypeId() const { return m_stdTypeId; }

    inline const qd::string& getTypeName() const { return m_fullName; }

    bool isAbstractType() const { return m_isAbstract; }

    bool isDerivedFrom(const TypeInfo& parentTypeID) const;

    template<typename T>
    inline bool isDerivedFrom_() const
    {
        checkDefined();
        return isDerivedFrom(qd::typeof_<T>());
    }

    bool isDefined() const {
        return m_bDefined;
    }

    THash32 getCid() const {
        return m_cid;
    }

    //------------------------------------------------------------------------
public:
    TypeInfo() = default;
    TypeInfo(TypeInfo const&) = default;

    TypeInfo(const StdTypeId& typeInfo)
        : m_stdTypeId(typeInfo)
    {}
    virtual ~TypeInfo() = default;
    TypeInfo& operator= (TypeInfo const& rhs) = default;

    bool checkDefined() const;

protected:
    void onTypeCreated();

    virtual void getInheritedProviders(_Out_ qd::vector<const TypeInfoBase* >& out_list) const override;

}; // class TypeInfo
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
