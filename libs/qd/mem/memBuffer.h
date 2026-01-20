#pragma once
#include "qd/base/base.h"
#include "qd/debug/assert.h"
#include "qd/mem/ptrMath.h"
#include "qd/stl/ref_ptr.h"
#include "qd/stl/string.h"


namespace qd {


class MemSpan : public qd::RefCounted
{
    typedef MemSpan TThis;

public:
    uint8_t* m_pBuffer;
    uint32_t m_nCapacity;

public:
    inline MemSpan()
        : m_pBuffer(nullptr)
        , m_nCapacity(0) {}

    inline MemSpan(void* pData, uint32_t size)
        : m_pBuffer((uint8_t*)pData)
        , m_nCapacity(size) {}

    inline MemSpan(const void* pData, uint32_t size)
        : m_pBuffer((uint8_t*)pData)
        , m_nCapacity(size) {}

    inline explicit MemSpan(void* pDataBegin, void* pDataEnd)
        : m_pBuffer((uint8_t*)pDataBegin)
        , m_nCapacity((uint32_t)qd::ptrDiff(pDataEnd, pDataBegin)) {
        assert(pDataEnd >= pDataBegin);
    }

    bool isValid() const { return m_pBuffer; }
    bool empty() const { return !m_pBuffer; }

    inline uint8_t* getBuffer() const { return (uint8_t*)m_pBuffer; }
    uint8_t* data() { return (uint8_t*)m_pBuffer; }
    const uint8_t* data() const { return (uint8_t*)m_pBuffer; }

    uint32_t getSize() const { return m_nCapacity; }
    uint32_t size() const { return m_nCapacity; }
    uint32_t getCapacity() const { return m_nCapacity; }
    uint32_t capacity() const { return m_nCapacity; }

    uint8_t* begin() const { return m_pBuffer; }
    uint8_t* end() const { return m_pBuffer + getCapacity(); }


    inline uint8_t* getBuffer(uint32_t nPos) const {
        assert((uint32_t)(nPos * 1) <= m_nCapacity); // may return end() pointer
        uint8_t* v = (uint8_t*)m_pBuffer + nPos;
        return v;
    }
    inline uint32_t getU32(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 4) <= m_nCapacity);
        uint32_t* pBuf = (uint32_t*)m_pBuffer;
        uint32_t v = pBuf[nPos];
        return v;
    }
    inline uint16_t getU16(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 2) <= m_nCapacity);
        uint16_t* pBuf = (uint16_t*)m_pBuffer;
        uint16_t v = pBuf[nPos];
        return v;
    }
    inline uint8_t getU8(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 1) <= m_nCapacity);
        uint8_t v = ((uint8_t*)m_pBuffer)[nPos];
        return v;
    }
    inline void setU32(uint32_t nPos, uint32_t v) const {
        assert((uint32_t)(nPos * 4) <= m_nCapacity);
        uint32_t* pBuf = (uint32_t*)m_pBuffer;
        pBuf[nPos] = v;
    }
    inline void setU16(uint32_t nPos, unsigned short v) const {
        assert((uint32_t)(nPos * 2) <= m_nCapacity);
        ((unsigned short*)m_pBuffer)[nPos] = v;
    }
    inline void setU8(uint32_t nPos, uint8_t v) const {
        assert((uint32_t)(nPos * 1) < m_nCapacity);
        ((uint8_t*)m_pBuffer)[nPos] = v;
    }
    inline uint32_t* getU32Ptr(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 4) < m_nCapacity);
        uint32_t* pBuf = (uint32_t*)m_pBuffer;
        uint32_t* v = pBuf + nPos;
        return v;
    }
    inline uint16_t* getU16Ptr(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 2) < m_nCapacity);
        uint16_t* pBuf = (uint16_t*)m_pBuffer;
        uint16_t* v = pBuf + nPos;
        return v;
    }
    inline uint8_t* getU8Ptr(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 1) < m_nCapacity);
        uint8_t* v = ((uint8_t*)m_pBuffer) + nPos;
        return v;
    }

}; // class MemSpan
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// SOME ALOCATED MEMORY BUFFER - without used size
class MemBuf : public MemSpan
{
    typedef MemBuf TThis;
    bool m_bNeedFree = false;

public:
    MemBuf() = default;

    inline explicit MemBuf(uint32_t size) {
        m_bNeedFree = true;
        m_pBuffer = new uint8_t[size];
        m_nCapacity = size;
    }

    inline explicit MemBuf(void* pData, uint32_t size, bool takeOwnership = true)
        : MemSpan() {
        setBuffer(pData, size, takeOwnership);
    }


    void moveFrom(MemBuf&& rh) {
        if (m_pBuffer && m_bNeedFree)
            delete[] m_pBuffer;
        m_pBuffer = rh.m_pBuffer;
        m_bNeedFree = rh.m_bNeedFree;
        m_nCapacity = rh.m_nCapacity;

        rh.m_pBuffer = nullptr;
        rh.m_nCapacity = 0;
    }

    MemBuf(TThis&& r) { // REFERENCED COPY
        m_pBuffer = nullptr;
        moveFrom(eastl::move(r));
    }

    inline TThis& operator= (TThis&& r) {
        moveFrom(eastl::move(r));
        return *this;
    }

    void expandBuffer(uint32_t newSize);

    void setBuffer(const void* pBuffer, uint32_t size, bool takeOwnership = true) {
        if (m_pBuffer)
            freeBuf();
        ASSERT_F(!takeOwnership || ((pBuffer && (int)size > 0)), "CMemBuf Size overflow");
        m_pBuffer = (uint8_t*)const_cast<void*>(pBuffer);
        m_bNeedFree = takeOwnership;
        m_nCapacity = size;
    }


    void cloneTo(MemBuf*& pDest) const {
        if (m_pBuffer == pDest->m_pBuffer)
            return;
        pDest->expandBuffer(m_nCapacity);
        ::memcpy(pDest->m_pBuffer, m_pBuffer, (size_t)m_nCapacity);
    }

    uint8_t* releaseBuffer() {
        uint8_t* pBuf = m_pBuffer;
        m_bNeedFree = false;
        freeBuf();
        return pBuf;
    }

    bool isNeedFree() const { return m_bNeedFree; }

    void freeBuf() {
        if (m_pBuffer && m_bNeedFree)
            delete[] m_pBuffer;
        m_pBuffer = nullptr;
        m_nCapacity = 0;
        m_bNeedFree = true;
    }

    inline void write(uint32_t Offset, const void* pSrc, uint32_t nBytes) { copyFrom(pSrc, nBytes, Offset); }

    void copyFrom(const void* pSrc, uint32_t nBytes, uint32_t nToOffset = 0);

    inline void _copyFrom(const void* pSrc, uint32_t nBytes, uint32_t nToOffset = 0) {
        assert(m_pBuffer);
        assert((nToOffset + nBytes) <= m_nCapacity);
        memcpy(m_pBuffer + nToOffset, pSrc, (size_t)nBytes);
    }


    void fillU8(uint8_t byteFill, uint32_t nBytes = ~0u, uint32_t offset = 0) {
        assert(m_pBuffer);
        if((offset + nBytes) > m_nCapacity)
            nBytes = m_nCapacity - offset;
        memset(m_pBuffer + offset, byteFill, (size_t)nBytes);
    }

    void memMove(uint32_t srcOffset, uint32_t destOffset, uint32_t nBytes);


    /*virtual*/ ~MemBuf() { freeBuf(); }

private:
    MemBuf(const MemBuf&) = delete;
    MemBuf& operator= (const MemBuf&) = delete;

}; // class MemBuf
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// BUFFER WITH OCCUPIZED SIZE EXACT - for write/append for Memory buffer with Capacity and used size
//
class MemData : public qd::RefCounted
{
    typedef MemData TThis;
    ref_ptr<MemBuf> m_pMemBuf;
    uint32_t m_nUsedSize = 0; // used Size of the MemBuffer

public:
    MemData() = default;
    ~MemData() { m_pMemBuf.reset(); }


    MemData(uint32_t nCapacity)
        : m_pMemBuf(new MemBuf(nCapacity)) {}

    explicit MemData(const ref_ptr<MemBuf>& pMemBuf, uint32_t nUsedSize)
        : m_pMemBuf(pMemBuf)
        , m_nUsedSize(nUsedSize) {}

    MemData(void* pData, uint32_t Size, bool bFreeBuf /*= true*/) { set(pData, Size, bFreeBuf); }

    MemData(MemData&& rh)
        : m_pMemBuf(std::move(rh.m_pMemBuf))
        , m_nUsedSize(rh.m_nUsedSize) {}

    MemData& operator= (const MemData& r) {
        m_pMemBuf = r.m_pMemBuf;
        m_nUsedSize = r.m_nUsedSize;
        return *this;
    }

    void set(void* pData, uint32_t Size, bool bFreeBuf /*= true*/) {
        assert(pData);
        m_pMemBuf = new MemBuf(pData, Size, bFreeBuf);
        m_nUsedSize = Size;
    }

    uint8_t* data() { return (uint8_t*)m_pMemBuf->getBuffer(); }
    const uint8_t* data() const { return (uint8_t*)m_pMemBuf->getBuffer(); }
    inline void* getBuffer() const { return m_pMemBuf->getBuffer(); }
    inline void* getBuffer(uint32_t nPos) const { return m_pMemBuf->getBuffer(nPos); }

    inline uint32_t size() const { return m_nUsedSize; }
    inline uint32_t getSize() const { return m_nUsedSize; }
    inline uint32_t getBufSize() const { return getSize(); }

    inline bool isEmpty() const { return m_nUsedSize == 0; }

    void resetSize() { m_nUsedSize = 0; }

    void setSize(uint32_t size) {
        if (size > getCapacity()) {
            assert2(0, "ERROR: CMemoryData Size:%u is greater then Capacity:%u", size, getCapacity());
        }
        m_nUsedSize = size;
    }

    inline uint32_t getCapacity() const { return m_pMemBuf->size(); }


    void expandBuffer(uint32_t nCapacity);


    inline void freeBuf() {
        m_pMemBuf->freeBuf();
        m_nUsedSize = 0;
    }

    void popFront(uint32_t nBytes) {
        if (nBytes >= m_nUsedSize) {
            m_nUsedSize = 0;
            return;
        }
        m_pMemBuf->memMove(nBytes, 0, m_nUsedSize - nBytes);
        m_nUsedSize -= nBytes;
    }

    uint32_t getRemCapacity() const {
        uint32_t nCapacity = m_pMemBuf->getCapacity();
        if (m_nUsedSize > nCapacity) {
            assert2(0, "Bad size", 0);
            return 0;
        }
        return nCapacity - m_nUsedSize;
    }


    // APPEND TO THE END OF BUFFER
    void write(const void* pSrc, uint32_t nBytes);


    uint8_t* releaseBuffer() {
        uint8_t* pBuf = m_pMemBuf->releaseBuffer();
        m_nUsedSize = 0;
        return pBuf;
    }


    const ref_ptr<MemBuf>& getMemBuf() const { return m_pMemBuf; }
    void setMemBuf(const ref_ptr<MemBuf>& pMemBuf, uint32_t dataSize = 0) {
        m_pMemBuf = pMemBuf;
        m_nUsedSize = dataSize;
    }


    inline uint32_t getU32(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 4) <= m_nUsedSize);
        return m_pMemBuf->getU32(nPos);
    }
    inline uint16_t getU16(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 2) <= m_nUsedSize);
        return m_pMemBuf->getU16(nPos);
    }
    inline uint8_t getU8(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 1) <= m_nUsedSize);
        return m_pMemBuf->getU8(nPos);
    }
    inline void setU32(uint32_t nPos, uint32_t v) const {
        assert((uint32_t)(nPos * 4) <= m_nUsedSize);
        m_pMemBuf->setU32(nPos, v);
    }
    inline void setU16(uint32_t nPos, unsigned short v) const {
        assert((uint32_t)(nPos * 2) <= m_nUsedSize);
        m_pMemBuf->setU16(nPos, v);
    }
    inline void setU8(uint32_t nPos, uint8_t v) const {
        assert((uint32_t)(nPos * 1) < m_nUsedSize);
        m_pMemBuf->setU8(nPos, v);
    }
    inline uint32_t* getU32Ptr(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 4) < m_nUsedSize);
        return m_pMemBuf->getU32Ptr(nPos);
    }
    inline uint16_t* getU16Ptr(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 2) < m_nUsedSize);
        return m_pMemBuf->getU16Ptr(nPos);
    }
    inline uint8_t* getU8Ptr(uint32_t nPos = 0) const {
        assert((uint32_t)(nPos * 1) < m_nUsedSize);
        return m_pMemBuf->getU8Ptr(nPos);
    }


}; // class CMemoryData
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
