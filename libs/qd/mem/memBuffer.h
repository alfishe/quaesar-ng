#pragma once
#include "qd/base/base.h"
#include "qd/stl/string.h"
#include "qd/debug/assert.h"
#include "qd/mem/ptrMath.h"
#include "qd/stl/ref_ptr.h"


namespace qd {


class MemSpan : public qd::RefCounted
{
	typedef MemSpan TThis;
	public:
	uint8_t* mpBuffer;
	uint32_t mCapacity;

	public:
	inline MemSpan() : mpBuffer(nullptr), mCapacity(0)
	{}

	inline MemSpan(void* pData, uint32_t size)
			: mpBuffer((uint8_t*)pData)
			, mCapacity(size)
	{}
	inline MemSpan(const void* pData, uint32_t size)
			: mpBuffer((uint8_t*)pData)
			, mCapacity(size)
	{}
	inline explicit MemSpan(void* pDataBegin, void* pDataEnd)
			: mpBuffer((uint8_t*)pDataBegin)
			, mCapacity((uint32_t)qd::ptrDiff(pDataEnd, pDataBegin))
	{
        assert(pDataEnd >= pDataBegin);
    }

	bool isValid() const {
		return mpBuffer;
	}

	bool empty() const {
		return !mpBuffer;
	}

	void* data() { return mpBuffer; }
	const void* data() const { return mpBuffer; }

	uint32_t getSize() const { return mCapacity; }
	uint32_t size() const { return mCapacity; }
	uint32_t getCapacity() const { return mCapacity; }
	uint32_t capacity() const { return mCapacity; }

	inline uint8_t* getBuffer() const { return (uint8_t*)mpBuffer; }

	uint8_t* begin() const { return mpBuffer; }

	uint8_t* end() const { return mpBuffer + getCapacity(); }



	inline uint8_t* getBuffer(uint32_t nPos) const
	{
		assert((uint32_t)(nPos * 1) <= mCapacity); // may return end() pointer
		uint8_t* v = (uint8_t*)mpBuffer + nPos;
		return v;
	}
	inline uint32_t getU32(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 4) <= mCapacity);
		uint32_t* pBuf = (uint32_t*)mpBuffer;
		uint32_t v = pBuf[nPos];
		return v;
	}
	inline uint16_t getU16(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 2) <= mCapacity);
		uint16_t* pBuf = (uint16_t*)mpBuffer;
		uint16_t v = pBuf[nPos];
		return v;
	}
	inline uint8_t getU8(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 1) <= mCapacity);
		uint8_t v = ((uint8_t*)mpBuffer)[nPos];
		return v;
	}
	inline void setU32(uint32_t nPos, uint32_t v) const
	{
		assert((uint32_t)(nPos * 4) <= mCapacity);
		uint32_t* pBuf = (uint32_t*)mpBuffer;
		pBuf[nPos] = v;
	}
	inline void setU16(uint32_t nPos, unsigned short v) const
	{
		assert((uint32_t)(nPos * 2) <= mCapacity);
		((unsigned short*)mpBuffer)[nPos] = v;
	}
	inline void setU8(uint32_t nPos, uint8_t v) const
	{
		assert((uint32_t)(nPos * 1) < mCapacity);
		((uint8_t*)mpBuffer)[nPos] = v;
	}
	inline uint32_t* getU32Ptr(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 4) < mCapacity);
		uint32_t* pBuf = (uint32_t*)mpBuffer;
		uint32_t* v = pBuf + nPos;
		return v;
	}
	inline uint16_t* getU16Ptr(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 2) < mCapacity);
		uint16_t* pBuf = (uint16_t*)mpBuffer;
		uint16_t* v = pBuf + nPos;
		return v;
	}
	inline uint8_t* getU8Ptr(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 1) < mCapacity);
		uint8_t* v = ((uint8_t*)mpBuffer) + nPos;
		return v;
	}

}; // class MemSpan



//////////////////////////////////////////////////////////////////////////
// SOME ALOCATED MEMORY BUFFER - without used size
class MemBuf : public MemSpan
{
	typedef MemBuf TThis;
	bool mbNeedFree = false;

	public:
	MemBuf() = default;

	inline explicit MemBuf(uint32_t size)
	{
		mbNeedFree = true;
		mpBuffer = new uint8_t[size];
		mCapacity = size;
	}

	inline explicit MemBuf(void* pData, uint32_t size, bool takeOwnership = true)
			: MemSpan()
	{
		setBuffer(pData, size, takeOwnership);
	}


	void moveFrom(MemBuf&& rh)
	{
		if (mpBuffer && mbNeedFree)
			delete[] mpBuffer;
		mpBuffer = rh.mpBuffer;
		mbNeedFree = rh.mbNeedFree;
		mCapacity = rh.mCapacity;

		rh.mpBuffer = nullptr;
		rh.mCapacity = 0;
	}

	MemBuf(TThis&& r)
	{ // REFERENCED COPY
		mpBuffer = nullptr;
		moveFrom(eastl::move(r));
	}

	inline TThis& operator= (TThis&& r)
	{
		moveFrom(eastl::move(r));
		return *this;
	}

	void expandBuffer(uint32_t newSize);

	void setBuffer(const void* pBuffer, uint32_t size, bool takeOwnership = true)
	{
		if (mpBuffer)
			freeBuf();
		ASSERT_F(!takeOwnership || ((pBuffer && (int)size > 0)), "CMemBuf Size overflow");
		mpBuffer = (uint8_t*)const_cast<void*>(pBuffer);
		mbNeedFree = takeOwnership;
		mCapacity = size;
	}


	void cloneTo(MemBuf*& pDest) const
	{
		if (mpBuffer == pDest->mpBuffer)
			return;
		pDest->expandBuffer(mCapacity);
		::memcpy(pDest->mpBuffer, mpBuffer, (size_t)mCapacity);
	}

	uint8_t* releaseBuffer()
	{
		uint8_t* pBuf = mpBuffer;
		mbNeedFree = false;
		freeBuf();
		return pBuf;
	}

	bool isNeedFree() const { return mbNeedFree; }

	void freeBuf()
	{
		if (mpBuffer && mbNeedFree)
			delete[] mpBuffer;
		mpBuffer = nullptr;
		mCapacity = 0;
		mbNeedFree = true;
	}

	inline void write(uint32_t Offset, const void* pSrc, uint32_t nBytes) { copyFrom(pSrc, nBytes, Offset); }

	void copyFrom(const void* pSrc, uint32_t nBytes, uint32_t nToOffset = 0);

	inline void _copyFrom(const void* pSrc, uint32_t nBytes, uint32_t nToOffset = 0)
	{
		assert(mpBuffer);
		assert((nToOffset + nBytes) <= mCapacity);
		memcpy(mpBuffer + nToOffset, pSrc, (size_t)nBytes);
	}


	void fill(uint32_t Offset, uint32_t nBytes, uint8_t byteFill)
	{
		assert((Offset + nBytes) > mCapacity);
		assert(!mpBuffer);
		memset(mpBuffer + Offset, byteFill, (size_t)nBytes);
	}

    void memMove(uint32_t srcOffset, uint32_t destOffset, uint32_t nBytes);


	/*virtual*/ ~MemBuf() { freeBuf(); }

	private:
	MemBuf(const MemBuf& r) : MemSpan() {} // NO COPY
	MemBuf& operator= (const MemBuf&) { return *this; }

}; // class MemBuf
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// BUFFER WITH OCCUPIZED SIZE EXACT - for write/append for Memory buffer with Capacity and used size
//
class MemData : public qd::RefCounted
{
	typedef MemData TThis;
    ref_ptr<MemBuf> mpMemBuf;
	uint32_t mnUsedSize = 0; // used Size of the MemBuffer

	public:
	MemData() = default;
    ~MemData() { mpMemBuf.reset(); }


    MemData(uint32_t nCapacity)
        : mpMemBuf(new MemBuf(nCapacity))
    {}

    explicit MemData(const ref_ptr<MemBuf>& pMemBuf, uint32_t nUsedSize)
        : mpMemBuf(pMemBuf)
        , mnUsedSize(nUsedSize)
    {}

	MemData(void* pData, uint32_t Size, bool bFreeBuf /*= true*/) { set(pData, Size, bFreeBuf); }

	MemData(MemData&& rh)
			: mnUsedSize(rh.mnUsedSize)
			, mpMemBuf(eastl::move(rh.mpMemBuf))
	{}

    MemData& operator= (const MemData& r)
    {
        mpMemBuf = r.mpMemBuf;
        mnUsedSize = r.mnUsedSize;
        return *this;
    }

    void set(void* pData, uint32_t Size, bool bFreeBuf /*= true*/)
    {
        assert(pData);
        mpMemBuf = new MemBuf(pData, Size, bFreeBuf);
        mnUsedSize = Size;
    }

	inline uint32_t getSize() const { return mnUsedSize; }

	inline uint32_t getBufSize() const { return getSize(); }

	inline bool isEmpty() const { return mnUsedSize == 0; }

	void resetSize() { mnUsedSize = 0; }

	void setSize(uint32_t size)
	{
		if (size > getCapacity()) {
			assert2(0, "ERROR: CMemoryData Size:%u is greater then Capacity:%u", size, getCapacity());
		}
		mnUsedSize = size;
	}

	inline uint32_t getCapacity() const { return mpMemBuf->size(); }

	inline void* getBuffer() const { return mpMemBuf->getBuffer(); }

	inline void* getBuffer(uint32_t nPos) const { return mpMemBuf->getBuffer(nPos); }

	void expandBuffer(uint32_t nCapacity);


	inline void freeBuf()
	{
		mpMemBuf->freeBuf();
		mnUsedSize = 0;
	}

    void popFront(uint32_t nBytes)
    {
        if (nBytes >= mnUsedSize)
        {
            mnUsedSize = 0;
            return;
        }
        mpMemBuf->memMove(nBytes, 0, mnUsedSize - nBytes);
        mnUsedSize -= nBytes;
    }

	uint32_t getRemCapacity() const
	{
		uint32_t nCapacity = mpMemBuf->getCapacity();
		if (mnUsedSize > nCapacity)
		{
			assert(0 && "Bad size");
			return 0;
		}
		return nCapacity - mnUsedSize;
	}


	// APPEND TO THE END OF BUFFER
	void write(const void* pSrc, uint32_t nBytes);


	uint8_t* releaseBuffer()
	{
		uint8_t* pBuf = mpMemBuf->releaseBuffer();
		mnUsedSize = 0;
		return pBuf;
	}


    const ref_ptr<MemBuf>& getMemBuf() const { return mpMemBuf; }
    void setMemBuf(const ref_ptr<MemBuf>& pMemBuf, uint32_t dataSize = 0)
    {
        mpMemBuf = pMemBuf;
        mnUsedSize = dataSize;
    }


	inline uint32_t getU32(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 4) <= mnUsedSize);
		return mpMemBuf->getU32(nPos);
	}
	inline uint16_t getU16(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 2) <= mnUsedSize);
		return mpMemBuf->getU16(nPos);
	}
	inline uint8_t getU8(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 1) <= mnUsedSize);
		return mpMemBuf->getU8(nPos);
	}
	inline void setU32(uint32_t nPos, uint32_t v) const
	{
		assert((uint32_t)(nPos * 4) <= mnUsedSize);
		mpMemBuf->setU32(nPos, v);
	}
	inline void setU16(uint32_t nPos, unsigned short v) const
	{
		assert((uint32_t)(nPos * 2) <= mnUsedSize);
		mpMemBuf->setU16(nPos, v);
	}
	inline void setU8(uint32_t nPos, uint8_t v) const
	{
		assert((uint32_t)(nPos * 1) < mnUsedSize);
		mpMemBuf->setU8(nPos, v);
	}
	inline uint32_t* getU32Ptr(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 4) < mnUsedSize);
		return mpMemBuf->getU32Ptr(nPos);
	}
	inline uint16_t* getU16Ptr(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 2) < mnUsedSize);
		return mpMemBuf->getU16Ptr(nPos);
	}
	inline uint8_t* getU8Ptr(uint32_t nPos = 0) const
	{
		assert((uint32_t)(nPos * 1) < mnUsedSize);
		return mpMemBuf->getU8Ptr(nPos);
	}


}; // class CMemoryData
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
