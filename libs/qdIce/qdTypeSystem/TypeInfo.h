#pragma once
#include <qdIce/qdTypeSystem/TypeId.h>
#include <qdIce/qdTypeSystem/typeInfoBase.h>
#include <EASTL/fixed_vector.h>
#include <qdIce/qdBase/stringId.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdTypeSystem/stdTypeId.h>


namespace qd {


class TypeInfo : public TypeInfoBase
{
    TypeId m_ID;
    StdTypeId m_StdTypeId;

    string m_FullName;
    string_view m_ShortName;
    string_view m_Namespace;

    int32_t m_size = -1;
    int32_t m_alignment = -1;
    bool m_isAbstract = false;
    bool m_bDefined = false;
    bool m_bFinal = false;

    // parents (super) classes of this type
    eastl::fixed_vector<const TypeInfo*, 2, true> m_pBaseSuperTypes;
    typedef eastl::fixed_vector<const TypeInfo*, 2, true> TBaseSuperTypes;

    friend class TypeRegistry;
    friend struct TypeInfoBuilder;

public:

    const string& getFullName() const {
        return m_FullName;
    }

    // Basic Type Info
    //-------------------------------------------------------------------------


    const TypeId& getId() const { return m_ID; }
    const StdTypeId& getStdTypeId() const { return m_StdTypeId; }

    inline const string& getTypeName() const { return m_FullName; }

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


public:
    TypeInfo() = default;
    TypeInfo(TypeInfo const&) = default;

    TypeInfo(const StdTypeId& typeInfo)
        : m_StdTypeId(typeInfo)
    {}
    virtual ~TypeInfo() = default;
    TypeInfo& operator= (TypeInfo const& rhs) = default;

    bool checkDefined() const;

protected:
    void onTypeCreated();

    virtual void getInheritedProviders(_Out_ eastl::vector<const TypeInfoBase* >& out_list) const override;

}; // class TypeInfo
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
