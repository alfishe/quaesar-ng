#pragma once
#include "TypeId.h"
#include <qdIce/qdBase/stringId.h>
#include <qdIce/qdDebug/assert.h>


namespace qd
{
class IReflectedType;

class TypeInfo
    {

    public:

        TypeInfo() = default;
        TypeInfo( TypeInfo const& ) = default;
        virtual ~TypeInfo() = default;

        TypeInfo& operator=( TypeInfo const& rhs ) = default;

        inline IReflectedType const* GetDefaultInstance() const { return m_pDefaultInstance; }

        // Basic Type Info
        //-------------------------------------------------------------------------

        inline char const* GetTypeName() const { return m_ID.ToStringID().c_str(); }

        bool IsAbstractType() const { return m_isAbstract; }

        bool IsDerivedFrom( TypeId const parentTypeID ) const;

        template<typename T>
        inline bool IsDerivedFrom() const { return IsDerivedFrom( T::GetStaticTypeID() ); }

        // Function declaration for generated property registration functions
        template<typename T>
        void RegisterProperties( IReflectedType const* pDefaultTypeInstance )
        {
            QD_HALT(); // Default implementation should never be called
        }


    public:

        TypeId                                  m_ID;
        IReflectedType const*                   m_pDefaultInstance;
        TypeInfo const*                         m_pParentTypeInfo = nullptr;
        int32_t                                 m_size = -1;
        int32_t                                 m_alignment = -1;
        bool                                    m_isAbstract = false;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TypeInfo_ : public TypeInfo
    {};
}
