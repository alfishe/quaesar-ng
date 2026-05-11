#pragma once
#include "qd/file/INodeElement.h"
#include "qd/stl/string.h"
#include "tinyxml2/tinyxml2.h"



class TinyXml2Node : public qd::INodeElement
{
    using TThis = TinyXml2Node;
    const tinyxml2::XMLElement* m_pNode;

public:
    TinyXml2Node(const tinyxml2::XMLElement* pNode)
        : m_pNode(pNode) {}

    virtual ~TinyXml2Node() override = default;

    virtual qtd::string getName() override { return m_pNode->Value(); }

    virtual int getLine() override { return m_pNode->GetLineNum(); }
    virtual int getColumn() override { return -1; }

    virtual bool getStrTry(const qtd::string_view& pAttr, qtd::string& outVal) override {
        const char* pTxt = m_pNode->Attribute(pAttr.data());
        if (!pTxt)
            return false;
        outVal = pTxt;
        return true;
    }

    virtual bool getInt(const qtd::string_view& pAttr, int& outVal) override {
        int64_t v;
        tinyxml2::XMLError err = m_pNode->QueryInt64Attribute(pAttr.data(), &v);
        if (err != tinyxml2::XML_SUCCESS)
            return false;
        outVal = v;
        return true;
    }

    virtual bool getU32Try(const qtd::string_view& pAttr, uint32_t* outVal = nullptr) override {
        uint64_t v;
        tinyxml2::XMLError err = m_pNode->QueryUnsigned64Attribute(pAttr.data(), &v);
        if (err != tinyxml2::XML_SUCCESS)
            return false;
        if (outVal)
            *outVal = v;
        return true;
    }

    virtual bool getFloat(const qtd::string_view& pAttr, float& outVal) override {
        tinyxml2::XMLError err = m_pNode->QueryFloatAttribute(pAttr.data(), &outVal);
        return err == tinyxml2::XML_SUCCESS;
    }

    virtual bool getBool(const qtd::string_view& pAttr, bool& outVal) override {
        tinyxml2::XMLError err = m_pNode->QueryBoolAttribute(pAttr.data(), &outVal);
        return err == tinyxml2::XML_SUCCESS;
    }

    virtual bool getInt2Try(const qtd::string_view& pAttr, int& x, int& y) override { return false; }
    virtual bool getFloat2Try(const qtd::string_view& pAttr, float& x, float& y) override { return false; }


    virtual int getNumChilds() const override {
        const tinyxml2::XMLNode* pChild = m_pNode->FirstChild();
        if (!pChild)
            return 0;
        int Count = 1;
        while ((pChild = pChild->NextSibling()) != nullptr)
            ++Count;
        return Count;
    }

    virtual int getNumKeys() const override {
        int Count = 0;
        const tinyxml2::XMLAttribute* pAttr = m_pNode->FirstAttribute();
        for (; pAttr; pAttr = pAttr->Next())
            Count++;
        return Count;
    }

    virtual qtd::string getKeyByInd(int i) override {
        int Count = i;
        const tinyxml2::XMLAttribute* pAttr = m_pNode->FirstAttribute();
        while (pAttr && Count > 0) {
            pAttr = pAttr->Next();
            --Count;
        }
        return pAttr ? pAttr->Name() : "";
    }

    virtual bool isKeyExists(const qtd::string_view& pAttr) override { return m_pNode->Attribute(pAttr.data()) != nullptr; }

    virtual INodeElement::Ptr getChildByInd(int i) override {
        int Count = i;
        const tinyxml2::XMLElement* pChild = m_pNode->FirstChildElement();
        while (pChild && Count > 0) {
            pChild = pChild->NextSiblingElement();
            --Count;
        }
        return new TThis(pChild);
    }

    virtual INodeElement::Ptr findChild(const qtd::string_view& pName) override {
        const tinyxml2::XMLElement* pChild = m_pNode->FirstChildElement(pName.data());
        if (!pChild)
            return nullptr;
        // 		pChild = pChild->NextSiblingElement(pName.data());
        return new TThis(pChild);
    }


    struct CNodeIter : public qd::INodeIterator {}; // struct CNodeIter


    virtual ref_ptr<qd::INodeIterator> itBegin() override {
        auto* pFirst = m_pNode->FirstChildElement();
        if (!pFirst)
            return nullptr;
        ref_ptr<CNodeIter> pIt;
        pIt.makeSelf();
        pIt->m_pCurrent = new TinyXml2Node(pFirst);
        return pIt;
    }

    virtual void itNext(qd::INodeIterator* pIt0) override {
        if (!pIt0)
            return;
        auto* pIt = static_cast<CNodeIter*>(pIt0);
        auto* pChild = pIt->m_pCurrent.get_<TinyXml2Node>();
        auto* pNext = pChild->m_pNode->NextSiblingElement();
        pIt->m_pCurrent = pNext ? new TinyXml2Node(pNext) : nullptr;
    }

    virtual bool itEnd(qd::INodeIterator* pIt0) override {
        if (!pIt0)
            return true;
        auto* pIt = static_cast<CNodeIter*>(pIt0);
        auto* pChild = pIt->m_pCurrent.get_<TinyXml2Node>();
        if (!pChild || !pChild->m_pNode)
            return true;
        return false;
    }


}; // class TinyXml2Node
//////////////////////////////////////////////////////////////////////////
