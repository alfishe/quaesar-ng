#pragma once
#include "qd/debug/assert.h"


//------------------------------------------------------------------------
// qd::ptr is a smart pointer that can be used with any pointer type, including raw pointers
// It is used for auto casting and type checking

namespace qd
{

template<typename T>
class ptr;


template<class T>
inline static ptr<T> make_ptr(const T* pPtr)
{
    return ptr<T>(pPtr);
}

//////////////////////////////////////////////////////////////////////////
template<class T>
class ptr
{
    typedef ptr<T> TThis;
    T* _ptr;

public:
    ptr(T* pPtr = nullptr)
        : _ptr(pPtr)
    {}

    inline ptr(const std::nullptr_t&)
        : _ptr((T*)0)
    {}

    inline const TThis& operator= (const std::nullptr_t&)
    {
        _ptr = (T*)0;
        return *this;
    }


    ptr(void* pPtr)
        : _ptr(static_cast<T*>(pPtr))
    {
        MP::assert_cast<T>::test(static_cast<T*>(pPtr));
        // assert( !pPtr || dynamic_cast<T*>( (T*)pPtr ) );
    }


    template<class T2>
    ptr(T2* pPtr)
        : _ptr(static_cast<T*>(pPtr))
    {
        MP::assert_cast<T>::test(pPtr);
        // assert( !pPtr || dynamic_cast<T*>( /*(T*)*/pPtr ) );
    }

    // 	ptr(const T* pPtr)
    // 		: _ptr( const_cast<T*>(pPtr) )
    // 	{}

    template<class T2>
    ptr(const T2* pPtr)
        : _ptr(static_cast<T*>(const_cast<T2*>(pPtr)))
    {
#if !defined EA_COMPILER_NO_RTTI
        assert(!pPtr || dynamic_cast<T*>(const_cast<T2*>(pPtr)));
#endif // EA_COMPILER_NO_RTTI
    }

    inline ptr(const ptr<T>& pPtr)
        : _ptr(pPtr.get())
    {}

    template<class T2>
    inline ptr(const ptr<T2>& pPtr)
        : _ptr(pPtr.template get_<T>())
    {}


    inline operator T* () const { return get(); }

    inline T& operator* ()
    {
        assert(valid());
        return *_ptr;
    }
    inline const T& operator* () const
    {
        assert(valid());
        return *_ptr;
    }
    inline T* operator->() const
    {
        assert(valid());
        return _ptr;
    }

    inline T* operator& () { return _ptr; }

    inline T* get() const { return _ptr; }

    inline void assign(T* pPtr) { _ptr = pPtr; }

    template<class T2>
    inline T2* get_() const
    {
        assert(is_<T2>());
        return static_cast<T2*>((T2*)_ptr);
    }

    template<class T2>
    inline bool operator== (const T2& p) const
    {
        return (get() == (T*)p);
    }

    template<class T2>
    inline bool operator!= (const T2& p) const
    {
        return (get() != (T*)p);
    }

    inline bool valid() const { return _ptr != nullptr; }

    template<class T2>
    inline bool is_() const
    {
        if (_ptr == nullptr)
            return true;
#if !defined EA_COMPILER_NO_RTTI
        T2* p = dynamic_cast<T2*>(_ptr);
        return p != nullptr;
#else
        return true;
#endif // EA_COMPILER_NO_RTTI
    }

    template<class T2>
    inline bool eq_() const
    {
        if (_ptr == nullptr)
            return false;
        return typeid(*_ptr) == typeid(T2);
    }

    template<typename... Args>
    static inline ptr<T> make(Args&&... args)
    {
        return new T(std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline T* makeSelf(Args&&... args)
    {
        _ptr = new T(std::forward<Args>(args)...);
        return _ptr;
    }

}; // class ptr
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
