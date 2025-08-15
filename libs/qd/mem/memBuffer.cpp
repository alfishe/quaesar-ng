#include "memBuffer.h"
#include "qd/debug/exception.h"


namespace qd {

void MemBuf::expandBuffer(uint32_t newSize)
{
    assert((int)newSize >= 0 && "CMemBuf() has Size greater then INT, overflow maybe");
    if (mpBuffer)
    {
        if (mCapacity >= newSize)
            return;

        // REDUCE BUFFER SIZE
        void* pMovedBuffer = realloc(mpBuffer, newSize);
        if (!pMovedBuffer)
        {
            ASSERT_F(0, "MemBuf Can't realloc buffer size %u", newSize);
        }
        else
            mpBuffer = (uint8_t*)pMovedBuffer;

        assert(mpBuffer);
    }
    else
    {
        if (newSize)
        {
            mpBuffer = (uint8_t*)malloc(newSize);
            if (!mpBuffer)
                ASSERT_F(0, "CMemBuf - Out of memory size:%u", newSize);
        }
    }
    mCapacity = newSize;
    mbNeedFree = true;
}


void MemBuf::memMove(uint32_t srcOffset, uint32_t destOffset, uint32_t nBytes)
{
    uint8_t* pSrcBuf = (uint8_t*)mpBuffer + srcOffset;
    uint8_t* pDestBuf = (uint8_t*)mpBuffer + destOffset;
    uint8_t* pEndBuf = (uint8_t*)mpBuffer + mCapacity;
    if (srcOffset + nBytes > mCapacity)
        throw Exception(EException::OUT_OF_RANGE, "CMemBuf::MemMove - Buffer overflow");
    int err;
    err = memmove_s(pDestBuf, pEndBuf - pDestBuf, pSrcBuf, nBytes);
    assert(!err);
}


}; // namespace qd
