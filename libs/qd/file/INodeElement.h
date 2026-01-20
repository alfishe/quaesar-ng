#pragma once
#include "qd/base/base.h"
#include "qd/stl/ref_ptr.h"
#include "qd/stl/string.h"
#include "qd/debug/exception.h"


namespace qd {
class INodeElement;

struct INodeIterator : public qd::RefCounted {
    ref_ptr<INodeElement> m_pCurrent;
    using Ptr = qd::ref_ptr<INodeIterator>;

public:
    ref_ptr<INodeElement>& get() { return m_pCurrent; }
    void setCurrent(const ref_ptr<INodeElement>& Current) { m_pCurrent = Current; }

    virtual ~INodeIterator() override = default;

}; // INodeIterator
//////////////////////////////////////////////////////////////////////////



// It's base interface for getting data from hierarchical data sources like XML, JSON, etc
// read-only Interface
class INodeElement : public qd::RefCounted
{
    using TThis = INodeElement;
public:
    using Ptr = qd::ref_ptr<INodeElement>;

    virtual qtd::string getName() { return ""; }

    virtual int getLine() { return -1; }
    virtual int getColumn() { return -1; }

    virtual bool getStrTry(const qtd::string_view& /*pKey*/, qtd::string& /*outVal*/) { return false; }
    virtual bool getIntValue(const qtd::string_view& /*pKey*/, int& /*outVal*/) { return false; }
    virtual bool getU32Try(const qtd::string_view& /*pKey*/, uint32_t* /*outVal*/ = nullptr) { return false; }
    virtual bool getFloatValue(const qtd::string_view& /*pKey*/, float& /*outVal*/) { return false; }
    virtual bool getBoolValue(const qtd::string_view& /*pKey*/, bool& /*outVal*/) { return false; }
    virtual bool getPointValue(const qtd::string_view& /*pKey*/, int& /*outX*/, int& /*outY*/) { return false; }
    virtual bool getVectorValue(const qtd::string_view& /*pKey*/, float& x, float& y) { return false; }

    template<class TString>
    qtd::string getStrDef(const qtd::string_view& key, const TString& defVal) {
        qtd::string tmpVal;
        if (this->getStrTry(key, tmpVal))
            return tmpVal;
        return defVal;
    }

    template<class TString>
    bool getStrTry(const qtd::string_view& pKey, TString* outVal) {
        qtd::string tmpVal;
        if (this->getStrTry(pKey, tmpVal)) {
            if (outVal)
                outVal->assign(tmpVal.c_str(), tmpVal.size());
            return true;
        }
        return false;
    }


    inline int getIntValue(const qtd::string_view& pKey) {
        int Val;
        if (!getIntValue(pKey, Val))
            G_THROW_OR_DO(Exception("Node can't parse value for Attr:\"%s\"", pKey), return 0);
        return Val;
    }


    inline uint32_t getU32Def(const qtd::string_view& key, uint32_t defVal) {
        uint32_t Val;
        if (getU32Try(key, &Val))
            return Val;
        return defVal;
    }


    inline float getFloatValue(const qtd::string_view& pKey) {
        float Val;
        if (!getFloatValue(pKey, Val))
            G_THROW_OR_DO(Exception("Node can't parse value for Attr:\"%s\"", pKey), return 0.f);
        return Val;
    }

    inline bool getBoolValue(const qtd::string_view& pKey) {
        bool Val;
        if (!getBoolValue(pKey, Val))
            G_THROW_OR_DO(Exception("Node can't parse value for Attr:\"%s\"", pKey), return false);
        return Val;
    }

    inline qtd::string getStrTry(const qtd::string_view& pKey) {
        qtd::string Res;
        if (!getStrTry(pKey, Res))
            return qtd::string();
        return Res;
    }

    virtual qtd::string getNodeText() { return qtd::string(); }

    // Keys access by Index
    // Get Num Keys
    virtual int getNumKeys() const { return 0; }
    // GET Key By Index
    virtual qtd::string getKeyByInd(int /*nKey*/) { return qtd::string(); }

    // = HAS KEY
    virtual bool isKeyExists(const qtd::string_view& /*pKey*/) { return false; }

    // Childs By indexe
    virtual int getNumChilds() const { return 0; }

    virtual INodeElement::Ptr getChildByInd(int /*i*/) { return nullptr; }
    INodeElement::Ptr getChildByInd(int i) const { return const_cast<TThis*>(this)->getChildByInd(i); }

    // Finds child node by name
    virtual INodeElement::Ptr findChild(const qtd::string_view& /*key*/) { return nullptr; }
    INodeElement::Ptr findChild(const qtd::string_view& key) const { return const_cast<TThis*>(this)->findChild(key); }

    // Finds child by his attributes NAME and VALUE in this Attribute
    virtual INodeElement::Ptr findChildByKeyVal(const qtd::string_view& /*pKeyName*/, const qtd::string_view& /*pKeyValue*/) { return nullptr; }

    // Iteratable API
    virtual qd::INodeIterator::Ptr itBegin() { return nullptr; }
    virtual void itNext(qd::INodeIterator* /*pIt*/) {}
    virtual bool itEnd(qd::INodeIterator* /*pIt*/) { return true; }

}; // class INodeElement
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////



/// //
///  Read value from Simple CString
///
class CNodeString : public qd::INodeElement
{
    typedef CNodeString TThis;
    qtd::string m_Name;
    qtd::string m_Value;

public:
    CNodeString(const qtd::string& Value = qtd::string(), const qtd::string& _Name = qtd::string())
        : m_Value(Value)
        , m_Name(_Name) {}

    TThis* setName(const qtd::string& v) {
        m_Name = v;
        return this;
    }
    TThis* setValue(const qtd::string& v) {
        m_Value = v;
        return this;
    }
    virtual qtd::string getName() override { return m_Name; }
    virtual int getLine() override { return 0; }
    virtual int getColumn() override { return 0; }

    virtual qtd::string getNodeText() override { return m_Value; }

    virtual bool getStrTry(const qtd::string_view&, qtd::string& Value) override {
        Value = m_Value;
        return true;
    }
    // TODO:
    //virtual bool getIntValue(const qtd::string_view&, int& Value) override { return m_Value.parseInt(Value); }
    //virtual bool getUIntValue(const qtd::string_view&, uint32_t& Value) override { return m_Value.parseUInt(Value); }
    //virtual bool getFloatValue(const qtd::string_view&, float& Value) override { return m_Value.parseFloat(Value); }
    //virtual bool getBoolValue(const qtd::string_view&, bool& Value) override { return m_Value.parseBool(Value); }

    virtual int getNumChilds() const override { return 0; }
    virtual int getNumKeys() const override { return 0; }
    virtual qtd::string getKeyByInd(int /*i*/) override { return ""; }
    virtual bool isKeyExists(const qtd::string_view& /*pAttr*/) override { return false; }
    virtual ref_ptr<INodeElement> getChildByInd(int /*i*/) override { return nullptr; }
    virtual ref_ptr<INodeElement> findChild(const qtd::string_view& /*pName*/) override { return nullptr; }

}; // class CNodeCmdArgs
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
