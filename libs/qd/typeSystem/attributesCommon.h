#pragma once
#include <qd/typeSystem/typeInfoAttrBase.h>
#include "qd/typeSystem/typeInfo.h"


namespace qd::tsAttr {
//
// ATTRIBUTE TO DECLARE A COMMON NAME OF AN OBJECT
//
class Name : public qd::TypeInfoAttribute
{
    qtd::string m_Name;

public:
    explicit Name(const string& name)
        : m_Name(name)
    {}

    const string& getName() const { return m_Name; }
}; // class Name
//////////////////////////////////////////////////////////////////////////

//
// ATTRIBUTE TO DECLARE A DESCRIPTION OF AN OBJECT
//
class Description : public qd::TypeInfoAttribute
{
    string m_Description;

public:
    explicit Description(const string& description)
        : m_Description(description)
    {}

    const string& getDescription() const { return m_Description; }
}; // class Description
//////////////////////////////////////////////////////////////////////////

//
// ATTRIBUTE TO DECLARE A CATEGORY OF AN OBJECT
//
class Category : public qd::TypeInfoAttribute
{
    string m_Category;

public:
    explicit Category(const string& category)
        : m_Category(category)
    {}

    const string& getCategory() const { return m_Category; }
}; // class Category
//////////////////////////////////////////////////////////////////////////

//
// ATTRIBUTE TO DECLARE A GUID FOR AN OBJECT
//
class GUID32 : public qd::TypeInfoAttribute
{
    uint32_t m_Guid;

public:
    explicit GUID32(uint32_t guid)
        : m_Guid(guid)
    {}

    uint32_t getGuid() const { return m_Guid; }

    bool operator== (const GUID32& other) const { return m_Guid == other.m_Guid; }
}; // class GUID32
//////////////////////////////////////////////////////////////////////////

using ClassID32 = GUID32;


//------------------------------------------------------------------------
// Declares callback function to create class instance
//
class CreateClassCb : public qd::TypeInfoAttribute
{
    TS_REFLECT_CLASS(qd::tsAttr::CreateClassCb, qd::TypeInfoAttribute);
    void *m_pCreateCallback;

public:
    template<typename TCreateFunc>
    explicit CreateClassCb(TCreateFunc createCallback)
        : m_pCreateCallback(reinterpret_cast<void *>(createCallback))
    {}

    template<class TBaseClass, typename... TArgs>
    TBaseClass* makeInstance_(TArgs... args) const
    {
        const qd::TypeInfo& class_info = *(static_cast<const qd::TypeInfo*>(m_pParent));
        using TCreateInstanceFunc = TBaseClass* (*)(const qd::TypeInfo&, TArgs...);
        auto makeInstFn = reinterpret_cast<TCreateInstanceFunc>(m_pCreateCallback);
        return makeInstFn(class_info, args...); // Fixed: Simplified return statement
    }
}; // class CreateClassCb
//////////////////////////////////////////////////////////////////////////


struct IntValueAttr : public qd::TypeInfoAttribute {
    int m_Value;

    IntValueAttr(int value)
        : m_Value(value)
    {}

    int getValue() const { return m_Value; }
}; // class IntValueAttr



struct CustomClassId32 : public qd::TypeInfoAttribute {
    uint32_t m_Id32;
    template<typename TVal>
    CustomClassId32(TVal value)
        : m_Id32((uint32_t)value)
    {}

    uint32_t getId32() const { return m_Id32; }
}; // class


}; // namespace qd::tsAttr
