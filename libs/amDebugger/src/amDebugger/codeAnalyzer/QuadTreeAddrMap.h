#pragma once
#include "amDebugger/base.h"
#include <qd/stl/vector.h>
#include <deque>
#include <vector>


namespace amD {
template<class TItem, int MAX_DEPTH = 16>
class QuadTreeAddrMap
{
public:
    struct Node {
        Node* m_children[4] = {nullptr, nullptr, nullptr, nullptr};
        TItem m_item = {};
        uint16_t m_idx = -1;
    public:
        Node()
        {}

        void reset()
        {
            for (int i = 0; i < 4; ++i)
                m_children[i] = nullptr;
        }

        bool isLeaf() const { return !m_children[0] && !m_children[1] && !m_children[2] && !m_children[3]; }
    };

public:
    QuadTreeAddrMap(AddrRef startAddr, AddrRef endAddr) { m_root = allocateNode(); }

    ~QuadTreeAddrMap() = default;

    void insert(AddrRef addr, const TItem& item) { insertImp(m_root, addr, item, 0); }

    //void query(AddrRef start, AddrRef end, qd::vector<TItem>& outItems) const { queryImp(m_root, start, end, outItems); }
    bool querySingle(AddrRef start, TItem *outItem) const
    {
        return querySingleImp(m_root, start, outItem);
    }

    bool remove(AddrRef addr, const TItem& item) { return removeImp(m_root, addr, item); }

    void clear()
    {
        for (int i = 0; i < 4; ++i)
            freeChildNode(m_root, i);
    }

private:
    std::deque<Node> m_nodes; // All nodes are stored here
    std::vector<uint16_t> m_freeIndices; // Indices of reusable nodes
    Node* m_root = nullptr;

public:
    Node* allocateNode()
    {
        // Reuse node from m_freeIndices if available
        if (!m_freeIndices.empty())
        {
            size_t idx = m_freeIndices.back();
            m_freeIndices.pop_back();
            m_nodes[idx].reset();
            return &m_nodes[idx];
        }
        // Otherwise, emplace new node
        Node& node = m_nodes.emplace_back();
        node.m_idx = (uint16_t)(m_nodes.size() - 1);
        return &node;
    }


    void freeChildNode(Node* parent, int idx)
    {
        if (!parent)
            return;
        if (Node* child = parent->m_children[idx])
        {
            for (int i = 0; i < 4; ++ i)
                freeChildNode(child, i);
            m_freeIndices.push_back(child->m_idx);
            parent->m_children[idx] = nullptr;
        }
    }


    void insertImp(Node* node, AddrRef addr, const TItem& item, int depth = 0)
    {
        if (depth >= MAX_DEPTH)
        {
            node->m_item = item;
            return;
        }
        int idx = getChildIndex(addr);
        Node* child = node->m_children[idx];
        if (!child)
        {
            child = allocateNode();
            node->m_children[idx] = child;
        }
        insertImp(child, addr << 2, item, depth + 1); // recursive
    }



    bool querySingleImp(const Node* node, AddrRef addr, TItem *outItem, const int depth = 0) const
    {
        if (depth < MAX_DEPTH)
        {
            int idx = getChildIndex(addr);
            Node* child = node->m_children[idx];
            if (!child)
                return false;
            return querySingleImp(child, addr << 2, outItem, depth + 1);
        }
        *outItem = node->m_item;
        return true;
    }


    bool removeImp(Node* parent, AddrRef addr, const TItem& item, const int depth = 0)
    {
        int idx = getChildIndex(addr);
        Node* child = parent->m_children[idx];
        if (!child)
            return true;
        if (depth < MAX_DEPTH - 1)
            return removeImp(child, addr << 2, item, depth + 1); // recursive
        if (child->m_item == item)
        {
            freeChildNode(parent, idx);
            return true;
        }
        return false;
    }


    inline static int getChildIndex(AddrRef addr)
    {
        return addr >> 30u;
    }
};


}; // namespace amD
