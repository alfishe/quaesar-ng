#pragma once
#include "qd/base/ref_ptr.h"
#include "qd/node/node.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd {

struct UiMessage {
    uint32_t id;
    UiMessage(uint32_t _id = 0)
        : id(_id)
    {}

    template<class T>
    T* cast_() const
    {
        if (!c_def(this))
            return nullptr;
        if (id != T::ID)
            return nullptr;
        return static_cast<T*>(const_cast<UiMessage*>(this));
    }

}; // struct UiMessage

template<uint32_t TId>
struct UiMessage_ : public UiMessage {
    static constexpr uint32_t ID = TId;
    UiMessage_()
        : UiMessage(TId)
    {}

#define UI_MSG_BASE(name) qd::UiMessage_<SCID(name)>

}; // struct UiMessage_
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
class UiNode : public qd::Node
{
    TS_BEGIN_REFLECT_CLASS_BASE(50, qd::UiNode, qd::Node);
    TS_END();

    static constexpr uint32_t UNDEF_ID = 0;

private:
    uint32_t m_id = 0;

    struct ChildItem {
        uint32_t id = 0;
        ref_ptr<qd::UiNode> ptr;
        const qd::TypeInfo* typeInfo = nullptr;
        uint32_t getId() const { return id; }
        UiNode* get() const { return this->ptr; }
    };
    qd::vector<ChildItem> m_pChilds;

protected:
    bool m_bVisible = true;

public:
    virtual void onNodeCreated(qd::NodeCreator* mk) override;

    void setup() {}

    inline UiNode* getParent() const { return static_cast<UiNode*>(m_pParent); }
    void setParent(qd::Node* pParent);

    UiNode* findChildById(uint32_t id) const;
    UiNode* findChildByType(const qd::TypeInfo& ti) const;
    int findChildIndex(UiNode* pChild) const;

    UiNode* addChild(ref_ptr<qd::UiNode> pChild);

    template<class T, typename... TArgs>
    inline T* addChild_(const char* p_name = nullptr, TArgs&&... args)
    {
        ref_ptr<T> pChild = new T();
        qd::NodeCreator mk;
        mk.parent = this;
        mk.id = p_name ? qd::fnv1aHash(p_name) : 0;
        pChild->onNodeCreated(&mk);
        if (p_name)
            pChild->setText(p_name);
        pChild->setup(std::forward<TArgs>(args)...);
        UiNode* pNode = addChild(pChild);
        return static_cast<T*>(pNode);
    }

    void removeChild(ref_ptr<UiNode> pChild);

    template<class T>
    inline T* findChildById_(uint32_t ID) const
    {
        return static_cast<T*>(findChildById(ID));
    }

    template<class T>
    inline T* findChildByIdName_(const char* p_name) const
    {
        return static_cast<T*>(findChildById(qd::fnv1aHash(p_name)));
    }

    template<class T>
    inline T* getChildById_(uint32_t ID) const
    {
        return static_cast<T*>(getChildById(ID));
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
    void setIdByName(const char* p_name) { setId(qd::fnv1aHash(p_name)); }


    virtual qd::string getText() const { return qd::string(); }

    virtual EFlow onUiNodeMessageProc(qd::UiMessage* in_msg);

    UiNode* getChild(int idx) const { return m_pChilds[idx].get(); }

public:
    virtual void updateBeforeDraw() {}
    virtual void drawContentImp();
    virtual void drawImp();
    void draw();

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
    virtual void onNodeCreated(qd::NodeCreator* mk) override { TSuper::onNodeCreated(mk); }
    virtual int getNumChild() override;
    virtual UiNode* getChild(int idx) override;
    virtual bool addChild(Node* child) override;
    virtual bool removeChild(Node* child) override;
    virtual bool beginIter(NodeIterator& buf) override;

}; // class UiNodesChildList
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
