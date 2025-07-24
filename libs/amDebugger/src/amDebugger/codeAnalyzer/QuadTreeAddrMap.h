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
        AddrRef m_startAddr = 0;
        AddrRef m_endAddr = 0;
        TItem m_item = {};
        Node* m_children[4] = {nullptr, nullptr, nullptr, nullptr};
        uint16_t m_idx = -1;
    public:
        Node(AddrRef start, AddrRef end)
            : m_startAddr(start)
            , m_endAddr(end)
        {}

        void reset(AddrRef start, AddrRef end)
        {
            m_startAddr = start;
            m_endAddr = end;
            for (int i = 0; i < 4; ++i)
                m_children[i] = nullptr;
        }

        bool isLeaf() const { return !m_children[0] && !m_children[1] && !m_children[2] && !m_children[3]; }
    };

public:
    QuadTreeAddrMap(AddrRef startAddr, AddrRef endAddr) { m_root = allocateNode(startAddr, endAddr); }

    ~QuadTreeAddrMap() = default;

    void insert(AddrRef addr, const TItem& item) { insertImp(m_root, addr, item, 0); }

    void query(AddrRef start, AddrRef end, qd::vector<TItem>& outItems) const { queryImp(m_root, start, end, outItems); }
    bool querySingle(AddrRef start, TItem *outItem) const
    {
        return querySingleImp(m_root, start, outItem);
    }

    bool remove(AddrRef addr, const TItem& item) { return removeImp(m_root, addr, item); }

    void clear()
    {
        while (m_nodes.size() > 1)
        {
            Node& node = m_nodes.back();
            freeNode(&node);
            m_nodes.pop_back();
        }
    }

private:
    std::deque<Node> m_nodes; // All nodes are stored here
    std::vector<uint16_t> m_freeIndices; // Indices of reusable nodes
    Node* m_root = nullptr;

public:
    Node* allocateNode(AddrRef start, AddrRef end)
    {
        // Reuse node from m_freeIndices if available
        if (!m_freeIndices.empty())
        {
            size_t idx = m_freeIndices.back();
            m_freeIndices.pop_back();
            m_nodes[idx].reset(start, end);
            return &m_nodes[idx];
        }
        // Otherwise, emplace new node
        Node& node = m_nodes.emplace_back(start, end);
        node.m_idx = (uint16_t)(m_nodes.size() - 1);
        return &node;
    }


    void freeNode(Node* node)
    {
        if (!node)
            return;
        m_freeIndices.push_back(node->m_idx);
    }


    void insertImp(Node* node, AddrRef addr, const TItem& item, int depth)
    {
        if (depth >= MAX_DEPTH)
        {
            node->m_item = item;
            return;
        }
        int idx = getChildIndex(node, addr);
        Node* child = node->m_children[idx];
        if (!child)
        {
            AddrRef start = node->m_startAddr + (node->m_endAddr - node->m_startAddr) / 4u * idx;
            AddrRef end = start + (node->m_endAddr - node->m_startAddr) / 4u;
            child = allocateNode(start, end);
            node->m_children[idx] = child;
        }
        insertImp(child, addr, item, depth + 1); // recursive
    }


    void queryImp(const Node* node, AddrRef start, AddrRef end, qd::vector<TItem>& outItems) const
    {
        if (!node || node->m_endAddr < start || node->m_startAddr > end)
            return;
        outItems.push_back(node->m_item);
        if (!node->isLeaf())
        {
            for (auto* child : node->m_children)
                queryImp(child, start, end, outItems); // recursive
        }
    }


    bool querySingleImp(const Node* node, const AddrRef addr, TItem *outItem, const int depth = 0) const
    {
        if (depth < MAX_DEPTH)
        {
            for (const Node* child : node->m_children)
            {
                if (!child || !(addr >= child->m_startAddr && addr < child->m_endAddr))
                    continue;
                return querySingleImp(child, addr, outItem, depth + 1);
            }
            return false;
        }
        *outItem = node->m_item;
        return true;
    }


    bool removeImp(Node* node, AddrRef addr, const TItem& item)
    {
        if (!node || addr < node->m_startAddr || addr > node->m_endAddr)
            return false;
        if (node->isLeaf())
        {
            if (node->m_item == item)
            {
                node->m_item = TItem{};
                return true;
            }
            return false;
        }
        int idx = getChildIndex(node, addr);
        return removeImp(node->m_children[idx], addr, item); // recursive
    }


    void subdivide(Node* node)
    {
        AddrRef mid = node->m_startAddr + (node->m_endAddr - node->m_startAddr) / 2u;
        AddrRef q1 = node->m_startAddr + (mid - node->m_startAddr) / 2u;
        AddrRef q3 = mid + (node->m_endAddr - mid) / 2u;

        node->m_children[0] = allocateNode(node->m_startAddr, q1);
        node->m_children[1] = allocateNode(q1 + 1, mid);
        node->m_children[2] = allocateNode(mid + 1, q3);
        node->m_children[3] = allocateNode(q3 + 1, node->m_endAddr);
    }


    inline int getChildIndex(const Node* node, AddrRef addr) const
    {
        AddrRef mid = node->m_startAddr + (node->m_endAddr - node->m_startAddr) / 2u;
        AddrRef q1 = node->m_startAddr + (mid - node->m_startAddr) / 2u;
        AddrRef q3 = mid + (node->m_endAddr - mid) / 2u;

        if (addr <= q1)
            return 0;
        if (addr <= mid)
            return 1;
        if (addr <= q3)
            return 2;
        return 3;
    }
};


}; // namespace amD
