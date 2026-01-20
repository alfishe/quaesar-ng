#include "memBuffer.h"
#include "qd/debug/exception.h"


namespace qd {

void MemBuf::expandBuffer(uint32_t newSize) {

    assert((int)newSize >= 0 && "CMemBuf() has Size greater then INT, overflow maybe");
    if (m_pBuffer) {
        if (m_nCapacity >= newSize)
            return;

        // REDUCE BUFFER SIZE
        void* pMovedBuffer = realloc(m_pBuffer, newSize);
        if (!pMovedBuffer) {
            ASSERT_F(0, "MemBuf Can't realloc buffer size %u", newSize);
        }
        else
            m_pBuffer = (uint8_t*)pMovedBuffer;

        assert(m_pBuffer);
    }
    else {
        if (newSize) {
            m_pBuffer = (uint8_t*)malloc(newSize);
            if (!m_pBuffer) {
                ASSERT_F(0, "CMemBuf - Out of memory size:%u", newSize);
            }
        }
    }
    m_nCapacity = newSize;
    m_bNeedFree = true;
}


void MemBuf::memMove(uint32_t srcOffset, uint32_t destOffset, uint32_t nBytes) {

    uint8_t* pSrcBuf = (uint8_t*)m_pBuffer + srcOffset;
    uint8_t* pDestBuf = (uint8_t*)m_pBuffer + destOffset;
    if (srcOffset + nBytes > m_nCapacity)
        throw Exception(EException::OUT_OF_RANGE, "CMemBuf::MemMove - Buffer overflow");
    memmove(pDestBuf, pSrcBuf, nBytes);
}


}; // namespace qd
