#pragma once
#include "qd/qdBase/ref_ptr.h"
#include "qd/qdCore/nodeBase.h"
#include "qd/qdTypeSystem/typeInfo.h"


namespace qd {

struct UiNodeMessage : public qd::NodeMessage {
    TS_REFLECT_CLASS_BASE(1000, qd::UiNodeMessage, qd::NodeMessage);

    UiNodeMessage() = default;

}; // struct UiNodeMessage


//////////////////////////////////////////////////////////////////////////
class UiNode : public qd::Node
{
    TS_BEGIN_REFLECT_CLASS_BASE(1000, qd::UiNode, qd::Node);
    TS_END();

    static constexpr uint32_t UNDEF_ID = 0;

private:
    uint32_t m_id = 0;

    struct ChildItem {
        uint32_t id = 0;
        ref_ptr<UiNode> ptr;
        uint32_t getId() const { return id; }
        UiNode* get() const { return this->ptr; }
    };
    qd::vector<ChildItem> m_pChilds;

protected:
    bool m_bVisible = true;

public:
    virtual void onNodeCreated(NodeCreator* mk) override;

    void setup() {}

    inline UiNode* getParent() const { return static_cast<UiNode*>(m_pParent); }
    void setParent(qd::Node* pParent);

    UiNode* findChildById(uint32_t id) const;
    int findChildIndex(UiNode* pChild) const;

    UiNode* addChild(ref_ptr<UiNode> pChild);

    template<class T, typename... TArgs>
    inline T* addChild_(TArgs&&... args)
    {
        ref_ptr<T> pChild = new T();
        NodeCreator mk;
        mk.parent = this;
        pChild->onNodeCreated(&mk);
        pChild->setup(std::forward<TArgs>(args)...);
        UiNode* pNode = addChild(pChild);
        return static_cast<T*>(pNode);
    }

    void removeChild(ref_ptr<UiNode> pChild);

    template<class T>
    inline T* findChildById_(uint32_t ID) const
    {
        return ptr<T>(findChildById(ID));
    }

    template<class T>
    inline T* getChildById_(uint32_t ID) const
    {
        return ptr<T>(getChildById(ID));
    }

    int getNumChild() const;

    UiNode* getChildById(uint32_t ID) const;
    int getChildIndex(UiNode* pChild) const;
    UiNode* getChildById(std::initializer_list<uint32_t> idList) const;

    template<class T>
    inline T* getParent_() const
    {
        return static_cast<T>(m_pParent);
    }

    uint32_t getId() const { return m_id; }
    void setId(uint32_t newId);


    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg) override;

    UiNode* getChild(int idx) const { return m_pChilds[idx].get(); }

public:
    virtual void updateBeforeDraw() {}

    virtual void drawContentImp();

    virtual void draw();

    bool isVisible(bool bCheckParents = false) const;
    bool setVisible(bool bVisible);

    void destroyRecursive();

    virtual void destroy() override;


}; // class UiNode
//////////////////////////////////////////////////////////////////////////


void uinode_draw_child(UiNode* pNode);


class UiNodesChildList : public qd::INodesChildList
{
    TS_REFLECT_CLASS(qd::UiNodesChildList, qd::INodesChildList);

public:
    UiNode* getOwner() const { return static_cast<UiNode*>(m_pParent); }
    virtual void onNodeCreated(qd::NodeCreator* mk) { TSuper::onNodeCreated(mk); }
    virtual int getNumChild() override;
    virtual UiNode* getChild(int idx) override;
    virtual bool addChild(Node* child) override;
    virtual bool removeChild(Node* child) override;
    virtual bool beginIter(NodeIterator& buf) override;

}; // class UiNodesChildList
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
