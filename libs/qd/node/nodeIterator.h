#pragma once
#include "qd/base/base.h"


namespace qd {
class Node;


class INodeIterator
{
public:
    virtual bool hasNext() const = 0;
    virtual Node* next() = 0;
    virtual ~INodeIterator() = default;
}; // lass INodeIterator


class NodeIterator
{
protected:
    static constexpr size_t IteratorBufferSize = 256;
    static constexpr size_t IteratorBufferAlignment = alignof(std::max_align_t);
    std::aligned_storage_t<IteratorBufferSize, IteratorBufferAlignment> m_inplaceBuf;
    INodeIterator* m_pIter = nullptr;

public:
    bool hasNext() const { return m_pIter->hasNext(); };
    Node* next() { return m_pIter->next(); }
    ~NodeIterator();

    template<typename TIter, typename... Args>
    TIter& makeIter_(Args&&... args)
    {
        assert(!m_pIter);
        static_assert(std::is_base_of_v<INodeIterator, TIter>, "TIter must be derived from NodeIterator");
        static_assert(sizeof(TIter) < IteratorBufferSize);
        m_pIter = new (&m_inplaceBuf) TIter(std::forward<Args>(args)...);
        return static_cast<TIter&>(*m_pIter);
    }
}; // class NodeIterator



}; // namespace qd
