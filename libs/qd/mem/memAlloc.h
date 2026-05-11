#pragma once
#include "qd/base/base.h"
#include "qd/thread/mutex.h"
#include "qd/debug/assert.h"
#include "qd/debug/exception.h"
#include "qd/stl/algorithm.h"


//////////////////////////////////////////////////////////////////////////
// ALIGN x to
#define ALLOC_ALIGN(x, y) (((x) + ((y)-1u)) & ~((y)-1u))
#define ALLOC_ALIGN4(x)   ALLOC_ALIGN(x, 4)
// ALIGN to 4 byte on ARCHITECTURE x32 and 8byte to x64  (for ARM) and Performance Improvement
#define ALLOC_ALIGN_PTR(x) ALLOC_ALIGN(((size_t)x), sizeof(void*))
//////////////////////////////////////////////////////////////////////////



namespace qd {
namespace MemAlloc {
template<typename T>
constexpr inline uint32_t Kb(T nKb) {
	return (uint32_t)(nKb * 1024);
}

template <typename T> constexpr inline uint32_t Mb(T nMb) {
	return (uint32_t)(nMb * 1024) * 1024u;
}


//////////////////////////////////////////////////////////////////////////
class CBlockNode_t
{
	CBlockNode_t* m_pPrev;

public:
	void* getData() {
		return this + 1;
	}

	static inline void alloc(/*Inout*/ CBlockNode_t*& pHead, uint32_t nAllocSize) {
		uint32_t nBlockStructSize = (uint32_t)sizeof(CBlockNode_t);
		CBlockNode_t* p = (CBlockNode_t*)malloc(size_t(nBlockStructSize + nAllocSize));
		if (!p)
			throw qd::Exception("No memory!");
		p->m_pPrev = pHead;
		pHead = p;
	}


	static inline void freeDataChain(CBlockNode_t* p) {
		while (p != nullptr) {
			void* pBytes = (void*)p;
			CBlockNode_t* pPrev_ = p->m_pPrev;
			free(pBytes);
			p = pPrev_;
		}
	}
}; // class CBlockNode_t
//////////////////////////////////////////////////////////////////////////



struct SectNode_t {
	union {
		// UNION
		SectNode_t* m_pNext;
		// byte m_Buffer[]; // UNION BYTE ARRAY
	};
	inline uint8_t* GetBuffer() const {
		return (uint8_t*)this;
	}
}; // struct SectNode_t



//////////////////////////////////////////////////////////////////////////
template <class TMutex /* Thread::NoMutex */>
class CFixedAlloc
{
	typedef CFixedAlloc<TMutex> TThis;
	SectNode_t* m_pNodeHead = nullptr;
	CBlockNode_t* m_pBlockHead = nullptr;
	TMutex m_Mutex;
	uint32_t m_nSectorSize; // one sector size
	uint32_t m_nSectorsInBlock; // count of sectors in one Block
	uint32_t m_UsedSectors = 0;

public:
	// allocates m_nSectorSize * m_nSectorsInBlock
	CFixedAlloc(uint32_t _nSectorSize, uint32_t _nSectorsInBlock = 64)
		: m_nSectorSize((uint32_t)ALLOC_ALIGN_PTR(_nSectorSize))
		, m_nSectorsInBlock(_nSectorsInBlock) {
		assert(m_nSectorsInBlock > 1 && "Need Memory Blocks");
		assert(m_nSectorSize >= sizeof(SectNode_t));
		assert(m_nSectorSize == ALLOC_ALIGN_PTR(m_nSectorSize) &&
			   "Memory should be aligned due to ARM memory alignment troubles");
	}

	~CFixedAlloc() {
		freeAll();
	}

	constexpr inline uint32_t getAllocSize() const {
		return /*ALLOC_ALIGN_PTR*/ (m_nSectorSize);
	}

	void* alloc() {
		qd::Locker_<TMutex> ml(m_Mutex);
		// CG_ASSERT(ALLOC_ALIGN_PTR(m_nSectorSize) >= sizeof(void*  /*SectNode_t*/));

		++m_UsedSectors;

		if (m_pNodeHead == nullptr) {
			uint32_t nBlockSize = m_nSectorsInBlock * m_nSectorSize;
			CBlockNode_t::alloc(m_pBlockHead, nBlockSize);

			SectNode_t* pStartNode = (SectNode_t*)m_pBlockHead->getData();
			m_pNodeHead = pStartNode;

			SectNode_t* pNode = pStartNode;
			SectNode_t* pLastNode = ((SectNode_t*)((uint8_t*)pStartNode + (nBlockSize - m_nSectorSize)));
			while (pNode != pLastNode) {
				SectNode_t* pNextNode = (SectNode_t*)((uint8_t*)pNode + (size_t)m_nSectorSize);
				pNode->m_pNext = pNextNode; // FIRST IS nullptr
				pNode = pNextNode;
			}
			pNode->m_pNext = nullptr; // LAST NODE = nullptr
		}
		assert(m_pNodeHead != nullptr);

		SectNode_t* pNode = m_pNodeHead;
		m_pNodeHead = pNode->m_pNext;

#if defined(_DEBUG) && 0 // TEST MEMORY  // TEST READONLY BUFFER
		pNode->m_Buffer[0] = 0x11;
		if (m_pHeadNode) {
			CNode* pTestNode = m_pHeadNode->m_pNext;
			if (!pTestNode || !pTestNode->m_pNext)
				c_def(0);
		}
#endif // _DEBUG
		return pNode;
	}


	// inplace new
	template <class T> inline T* new_() {
		assert(sizeof(T) <= m_nSectorSize);
		void* pPtr = alloc();
		T* pObj = new (pPtr) T(); // CALLS CONSTRUCTOR
		return pObj;
	}

	void freeMem(void* p) {
		qd::Locker_<TMutex> ml(m_Mutex);
		--m_UsedSectors;

		if (p != nullptr) {
			SectNode_t* pPrevHead = m_pNodeHead;
			m_pNodeHead = (SectNode_t*)p;
			m_pNodeHead->m_pNext = pPrevHead;

#if 0 && defined(_DEBUG) // TEST MEMORY
                        CNode* pTestNode = m_pHeadNode->m_pNext;
                        if (!pTestNode || !pTestNode->m_pNext)
                            c_def(0);
#endif // _DEBUG
		}
	}


	void freeAll() {
		qd::Locker_<TMutex> ml(m_Mutex);
		if (!m_pBlockHead)
			return;
		CBlockNode_t::freeDataChain(m_pBlockHead);
		m_pBlockHead = nullptr;
		m_pNodeHead = nullptr;
		m_UsedSectors = 0;
	}


	uint32_t getUsedSectors() const {
		return m_UsedSectors;
	}
}; // class CFixedAlloc
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
class CFixedAllocMutex : public MemAlloc::CFixedAlloc<qd::Mutex>
{
	typedef MemAlloc::CFixedAlloc<qd::Mutex> TSuper;

public:
	CFixedAllocMutex(uint32_t _ccAllocSize, uint32_t _ccBlocksCount = 64)
		: TSuper(_ccAllocSize, _ccBlocksCount) {}
}; // class CFixedAllocMutex
//////////////////////////////////////////////////////////////////////////


template <uint32_t nSectorSize, uint32_t _nSectorsInBlocks = 64>
class CFixedAllocMutex_ : public MemAlloc::CFixedAlloc<qd::Mutex>
{
	typedef MemAlloc::CFixedAlloc<qd::Mutex> TSuper;

public:
	CFixedAllocMutex_()
		: TSuper(nSectorSize, _nSectorsInBlocks) {}
}; // class CFixedAllocMutex
//////////////////////////////////////////////////////////////////////////


template <uint32_t nSectorSize, uint32_t _nSectorsInBlocks = 64>
class CFixedAllocNoMutex_ : public MemAlloc::CFixedAlloc<qd::DummyMutex>
{
	typedef MemAlloc::CFixedAlloc<qd::DummyMutex> TSuper;

public:
	CFixedAllocNoMutex_()
		: TSuper(nSectorSize, _nSectorsInBlocks) {}

}; // class CFixedAllocNoMutex
//////////////////////////////////////////////////////////////////////////



class CSmallObjectAllocator
{
	// HEAP MONOID: SINGLETON
	enum {
		ALIGN = alignof(size_t)
	};

private:
	static bool m_bSingleDestroyed;
	static CSmallObjectAllocator* m_pSingleInstance;

	static void destroySingleton() {
		SAFE_DELETE(m_pSingleInstance);
		m_bSingleDestroyed = true;
	}

	MemAlloc::CFixedAllocMutex_<0x008 + ALIGN, 256> m_Alloc0x008;
	MemAlloc::CFixedAllocMutex_<0x010 + ALIGN, 256> m_Alloc0x010;
	MemAlloc::CFixedAllocMutex_<0x020 + ALIGN, 256> m_Alloc0x020;
	MemAlloc::CFixedAllocMutex_<0x040 + ALIGN, 128> m_Alloc0x040;
	MemAlloc::CFixedAllocMutex_<0x080 + ALIGN, 064> m_Alloc0x080;
	MemAlloc::CFixedAllocMutex_<0x0C0 + ALIGN, 064> m_Alloc0x0C0;
	MemAlloc::CFixedAllocMutex_<0x100 + ALIGN, 064> m_Alloc0x100;
	MemAlloc::CFixedAllocMutex_<0x180 + ALIGN, 064> m_Alloc0x180;

public:
	static CSmallObjectAllocator* get();

	enum eAllocCode : uint8_t {
		ALLOC_0x008 = 0x80,
		ALLOC_0x010,
		ALLOC_0x020,
		ALLOC_0x040,
		ALLOC_0x080,
		ALLOC_0x0C0,
		ALLOC_0x100,
		ALLOC_0x180,
		ALLOC_HEAP,
		_MAX_VAL_,
	};

	static inline void* putMemMark(void* pPtr, const eAllocCode& alCode) {
		*((uint8_t*)pPtr) = alCode;
		pPtr = ((uint8_t*)pPtr) + sizeof(size_t);
		return pPtr;
	}

public:
	void freeMem(void* pPtr);

	template<uint32_t nSize>
	inline void* alloc_()
	{
		ASSERT_AND_DO(0, return nullptr, "NOT IMPLEMENTED!");
	}


	inline void* allocHeap(size_t sz) {
		return putMemMark(malloc(sz + sizeof(size_t)), CSmallObjectAllocator::ALLOC_HEAP);
	}

	/*Force inline required*/
	QD_FORCE_INLINE void* alloc(const size_t& sz);

}; // class CSmallObjectAllocator
//////////////////////////////////////////////////////////////////////////



template <> inline void* CSmallObjectAllocator::alloc_<0x008>() {
	return putMemMark(m_Alloc0x008.alloc(), ALLOC_0x008);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x010>() {
	return putMemMark(m_Alloc0x010.alloc(), ALLOC_0x010);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x020>() {
	return putMemMark(m_Alloc0x020.alloc(), ALLOC_0x020);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x040>() {
	return putMemMark(m_Alloc0x040.alloc(), ALLOC_0x040);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x080>() {
	return putMemMark(m_Alloc0x080.alloc(), ALLOC_0x080);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x0C0>() {
	return putMemMark(m_Alloc0x0C0.alloc(), ALLOC_0x0C0);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x100>() {
	return putMemMark(m_Alloc0x100.alloc(), ALLOC_0x100);
}
template <> inline void* CSmallObjectAllocator::alloc_<0x180>() {
	return putMemMark(m_Alloc0x180.alloc(), ALLOC_0x180);
}


/*Force inline required*/
inline void* CSmallObjectAllocator::alloc(const size_t& sz) {
	if (sz <= 0x008)
		return alloc_<0x008>();
	else if (sz <= 0x010)
		return alloc_<0x010>();
	else if (sz <= 0x020)
		return alloc_<0x020>();
	else if (sz <= 0x040)
		return alloc_<0x040>();
	else if (sz <= 0x080)
		return alloc_<0x080>();
	else if (sz <= 0x0C0)
		return alloc_<0x0C0>();
	else if (sz <= 0x100)
		return alloc_<0x100>();
	else if (sz <= 0x180)
		return alloc_<0x180>();
	else
		return allocHeap(sz);
}

//////////////////////////////////////////////////////////////////////////
class CSmallObject
{
public:
	CSmallObject() {}

	virtual ~CSmallObject() {}

	// CUSTOM ALLOCATORS
	static inline void* operator new (size_t sz) {
		MemAlloc::CSmallObjectAllocator* pAlloc = MemAlloc::CSmallObjectAllocator::get();
		return pAlloc->alloc(sz);
	}
	static inline void operator delete (void* ptr) throw() {
		MemAlloc::CSmallObjectAllocator* pAlloc = MemAlloc::CSmallObjectAllocator::get();
		pAlloc->freeMem(ptr);
	}
	// no throw new/delete
	// 	__declspec( nothrow ) static inline void* operator new( size_t sz, const std::nothrow_t& nt ) throw( ) {
	// 		MemAlloc::CSmallObjectAllocator* pAlloc = MemAlloc::CSmallObjectAllocator::Get();
	// 		return pAlloc->Alloc( sz );
	// 	}
	// 	__declspec( nothrow ) static inline void operator delete( void* ptr, const std::nothrow_t& nt ) throw( ) {
	// 		MemAlloc::CSmallObjectAllocator* pAlloc = MemAlloc::CSmallObjectAllocator::Get();
	// 		pAlloc->Free( ptr );
	// 	}
	// private:
	// in-place new-delete pair  for stl vector - inplace creation
	// buffer on stack
	// unsigned char buf[sizeof( int ) * 2];
	// placement new in buf
	// int *pInt = new ( buf ) int( 3 );

/*
	static inline void* operator new (size_t sz, void* pWhere) {
		return ::operator new (sz, pWhere);
	}
	static inline void operator delete (void* ptr, void* ptr2) {
		::operator delete (ptr, ptr2);
	}
*/
}; // class CSmallObject
//////////////////////////////////////////////////////////////////////////



}; // namespace MemAlloc
//////////////////////////////////////////////////////////////////////////


template<typename T, typename... Args>
T* mkNewSelf_(T** pObj, Args&&... args)
{
	//MemAlloc::CSmallObjectAllocator* pAlloc = MemAlloc::CSmallObjectAllocator::get();
	//T* pObj = new (pAlloc->alloc(sizeof(T))) T(eastl::forward<Args>(args)...);
	if (!*pObj)
		*pObj = new T(qtd::forward<Args>(args)...);
	return *pObj;
}

}; // namespace qd
