#pragma once
#include "qdIce/qdTypeSystem/typeInfoBuilder.h"
#include "qdIce/qdTypeSystem/stdTypeId.h"


//////////////////////////////////////////////////////////////////////////
#define TS_FOR_EACH(WHAT, ...) TS_FOR_EACH_(TS_GET_ARGS_COUNT(__VA_ARGS__), WHAT, __VA_ARGS__)

//------------------------------------------------------------------------
// clang-format off
#define TS_EXPAND(x) x
#define TS_FIRST_ARG(X, ...) X
#define TS_FOR_EACH_1(WHAT, X)       WHAT(X)
#define TS_FOR_EACH_2(WHAT, X, ...)  WHAT(X) TS_EXPAND(TS_FOR_EACH_1(WHAT, __VA_ARGS__))
#define TS_FOR_EACH_3(WHAT, X, ...)  WHAT(X) TS_EXPAND(TS_FOR_EACH_2(WHAT, __VA_ARGS__))
#define TS_FOR_EACH_4(WHAT, X, ...)  WHAT(X) TS_EXPAND(TS_FOR_EACH_3(WHAT, __VA_ARGS__))

#define TS_GET_ARGS_COUNT(...) TS_FOR_EACH_NARG_(__VA_ARGS__, TS_FOR_EACH_RSEQ_N())
#define TS_FOR_EACH_ARG_N(_1, _2, _3, _4, _LAST_N, ...) _LAST_N
#define TS_FOR_EACH_RSEQ_N()       4, 3, 2, 1, 0
#define TS_FOR_EACH_NARG_(...)     TS_EXPAND(TS_FOR_EACH_ARG_N(__VA_ARGS__))
#define TS_CONCATENATE(x, y)       x##y
#define TS_FOR_EACH_(N, WHAT, ...) TS_EXPAND(TS_CONCATENATE(TS_FOR_EACH_, N)(WHAT, __VA_ARGS__))
// clang-format on




//-------------------------------------------------------------------------
// Reflection Macros
//-------------------------------------------------------------------------
#define TS_BEGIN_REFLECT_TYPE(ObjectClass, BaseClass)                          \
private:                                                                            \
    struct ClassMeta;                                                               \
    using TSuper = BaseClass;                                                       \
                                                                                    \
public:                                                                             \
    inline static const qd::TypeInfo* s_pTypeInfo = qd::_regTypeInfo_<ClassMeta>(); \
    constexpr static THash32 CID = qd::hash_type_info_name(#ObjectClass);           \
                                                                                    \
    virtual THash32 getCid() const /*override*/                                     \
    {                                                                               \
        return ObjectClass::CID; /* constexpr ID from fnv1hhash of type name */     \
    }                                                                               \
    static const qd::TypeInfo& getStaticTypeInfo()                                  \
    {                                                                               \
        validateStaticTypeInfoPtr(ObjectClass::s_pTypeInfo);                        \
        return *ObjectClass::s_pTypeInfo;                                           \
    }                                                                               \
    virtual const qd::TypeInfo& getTypeInfo() const /*override*/                    \
    {                                                                               \
        validateStaticTypeInfoPtr(ObjectClass::s_pTypeInfo);                        \
        return *ObjectClass::s_pTypeInfo;                                           \
    }                                                                               \
                                                                                    \
private:                                                                            \
    struct ClassMeta : public qd::TypeInfoBuilderObject_<ObjectClass> {             \
                                                                                    \
        ClassMeta()                                                                 \
            : qd::TypeInfoBuilderObject_<ObjectClass>(#ObjectClass)                 \
        {                                                                           \
            qd::TypeInfo* pCurTypeInfo = m_pType;                                   \
            (void)(pCurTypeInfo);



#define TS_END()                    \
    }                               \
    }                               \
    ; /*struct TObject::ClassMeta*/ \
public:



//------------------------------------------------------------------------
// Declare parent (derived from) type of object_reflector
#define TS_DECLARE_BASE_TYPE(x) addBaseType(qd::makeStdTypeId_<x>());

// Mark class base (parent) for number
#define TS_BASE_FOR_N_TYPES(x) \
    markAsBase(x);             \
    //////////////////////////////////////////////////////////////////////////


// Mark type that it is an base (parent) for about N number of inheritors
#define TS_MARK_AS_FINAL(x)    \
    markAsFinal(pCurTypeInfo); \
    //////////////////////////////////////////////////////////////////////////


// WARNING: 'TS_ATTRIBUTE(...)' MUST BE SITUATED BELOW OF BEGIN_OBJECT_REFLECTOR(...), NO AFTER CONSTRUCTOR
// DECLARATION 'AttrClass' MUST BE INHERITED FROM 'TypeInfoAttribute'
#define TS_ATTRIBUTE(AttrClass) addAttribute_((qd::TypeInfoBase*)pCurTypeInfo, new AttrClass);
//////////////////////////////////////////////////////////////////////////



#define TS_BEGIN_REFLECT_CLASS_BASE(nApproxInherited, ObjectType, ...) \
    TS_BEGIN_REFLECT_TYPE(ObjectType, TS_FIRST_ARG(__VA_ARGS__))                     \
    TS_BASE_FOR_N_TYPES(nApproxInherited);                             \
    TS_FOR_EACH(TS_DECLARE_BASE_TYPE, __VA_ARGS__)


// Declare reflected TypeInfo with class and derives. Use `void` as null base class
#define TS_BEGIN_REFLECT_CLASS(ObjectType, ...)    \
    TS_BEGIN_REFLECT_TYPE(ObjectType, TS_FIRST_ARG(__VA_ARGS__)) \
    TS_FOR_EACH(TS_DECLARE_BASE_TYPE, __VA_ARGS__)


#define TS_REFLECT_CLASS_BASE(nApproxInherited, ObjectType, ...) \
    TS_BEGIN_REFLECT_TYPE(ObjectType, TS_FIRST_ARG(__VA_ARGS__))               \
    TS_BASE_FOR_N_TYPES(nApproxInherited);                       \
    TS_FOR_EACH(TS_DECLARE_BASE_TYPE, __VA_ARGS__)               \
    TS_END()


#define TS_REFLECT_CLASS_FINAL(ObjectType, ...)    \
    TS_BEGIN_REFLECT_TYPE(ObjectType, TS_FIRST_ARG(__VA_ARGS__)) \
    TS_MARK_AS_FINAL();                            \
    TS_FOR_EACH(TS_DECLARE_BASE_TYPE, __VA_ARGS__) \
    TS_END()


#define TS_REFLECT_CLASS(ObjectType, ...)          \
    TS_BEGIN_REFLECT_TYPE(ObjectType, TS_FIRST_ARG(__VA_ARGS__)) \
    TS_FOR_EACH(TS_DECLARE_BASE_TYPE, __VA_ARGS__) \
    TS_END()


namespace qd {
class TypeInfo;

template<typename TMetaClassReg>
const qd::TypeInfo* _regTypeInfo_()
{
    TMetaClassReg registrator;
    return registrator.m_pType;
}

}; // namespace qd
