#pragma once
#include <qd/base/base.h>


namespace qd
{
	
	template<class T, typename TInt>
	constexpr inline T ptrAddSelf(T& ptr, TInt offset)
	{
		uint8_t*& p8 = (uint8_t*&)ptr;
		p8 += offset;
		return ptr;
	}
	
	template<class T, typename TInt>
	EA_NODISCARD constexpr inline uint8_t* ptrAdd(const T* ptr, TInt offset)
	{
		uint8_t* p8 = (uint8_t*)ptr;
		p8 += offset;
		return p8;
	}
	
	template<typename TResPtr, typename T, typename TInt>
	EA_NODISCARD constexpr inline TResPtr* ptrAdd_(T ptr, TInt offset)
	{
		size_t pt = (size_t)(ptr) + (size_t)(offset);
		return reinterpret_cast<TResPtr*>(pt);
	}
	
	
	template<class TA, class TB>
	EA_NODISCARD constexpr inline size_t ptrDiff(TA ptrA, TB ptrB)
	{
		const size_t diff = (const uint8_t*)ptrA - (const uint8_t*)ptrB;
		return diff;
	}
	
	template<class T>
	EA_NODISCARD constexpr inline uint32_t ptr2DW(T pPtr)
	{
		return static_cast<uint32_t>((std::size_t)((void*)pPtr)); //-V205 //-V221
	}
	
	template<class T>
	EA_NODISCARD constexpr inline void* DW2Ptr(const T& pPtr)
	{
		return reinterpret_cast<void*>((std::size_t)pPtr);
	}
	
	
	// true constexpr for VC compiler
	template<typename TClass, typename TMember>
	int getOffsetOf(TMember TClass::*pMember)
	{
		return (uint8_t*)&(((TClass*)nullptr)->*pMember) - (uint8_t*)nullptr;
	
		// union Cast {
		// 	TMember TClass::*pMember;
		// 	uint8_t* pByte;
		// } cast = { pMember };
		// return (int)(cast.pByte); // - (uint8_t*)nullptr);
	
		//return __builtin_offsetof(TClass, *pMember);
		//return __builtin_offsetof(TClass, *(reinterpret_cast<TMember*>(nullptr)) ); 
	
		//return EA_OFFSETOF(TClass, *pMember);
	
		//return ((size_t)(((uintptr_t)&reinterpret_cast<const volatile char&>((((TClass*)65536)->*pMember))) - 65536));
	}


	inline uint16_t ptrU16(const void* ptr, uint32_t offset)
	{
		return *reinterpret_cast<const uint16_t*>((const uint8_t*)ptr + offset);
	}


	inline uint32_t ptrU16L(void* ptr, uint32_t offset)
	{
		union {
			uint32_t u32;
			uint16_t u16;
		} cast;
		cast.u32 = 0;
		cast.u16 = *reinterpret_cast<const uint16_t*>((const uint8_t*)ptr + offset);
		return cast.u32;
	}
	
	
	inline uint32_t ptrU32(void* ptr, uint32_t offset)
	{
		return *(uint32_t*)((uint8_t*)ptr + offset);
	}
	

	
}; // namespace qd
