#pragma once
#include "qd/base/base.h"
#include "qd/debug/assert.h"
#include "qd/platform/compiler.h"
#include <EASTL/atomic.h>
#include <typeinfo>


#define SAFE_DELETE_REF_PTR(pPtr)               \
    {                                           \
        if (pPtr != nullptr)                    \
        {                                       \
            pPtr->ref_ptr_release_and_delete(); \
            pPtr = nullptr;                     \
        };                                      \
    }


template<typename T>
class ptr;
template<typename T>
class ref_ptr;
template<typename T>
class wref_ptr;
class RefCounted;
// Helper class as Base
#define vv(T, B) T



template<class T>
inline static ptr<T> make_ptr(const T* pPtr)
{
    return ptr<T>(pPtr);
}

template<class T>
inline static ptr<T> make_ptr(const ref_ptr<T>& pPtr)
{
    return ptr<T>(pPtr);
}
template<class T>
inline static ptr<T> make_ptr(const wref_ptr<T>& pPtr)
{
    return ptr<T>(pPtr);
}

template<typename T>
inline static T* get_ptr(const T* pPtr)
{
    return const_cast<T*>(pPtr);
}

template<typename T>
inline static T* get_ptr(const ref_ptr<T>& pPtr)
{
    return pPtr.get();
}

template<typename T>
inline static T* get_ptr(const wref_ptr<T>& pPtr)
{
    return pPtr.get();
}

template<typename T>
inline static T* get_ptr(const int& pPtr)
{
    assert(pPtr == 0);
    return nullptr;
}

template<class T>
inline static T* get_ptr_null(const ref_ptr<T>& pPtr)
{
    return (T*)(nullptr);
}

namespace qd::details {
// clang-format off
template<class T>
extern void referenced_static_delete(T* pThis);
template<typename T> class ref_ptr_base;
template<typename T> class ref_ptr_getter;
template<typename T> class wref_ptr_getter;
// clang-format on
}; // namespace qd::details


namespace qd::MP {
template<typename T>
struct InnerType {
    typedef T type;
};


template<class T, bool isPod = std::is_polymorphic<T>::value >
struct assert_cast {
    template<class T2>
    static void test(T2* pPtr)
    {
        c_def(0);
    };
};

template<class T>
struct assert_cast<T, true> {
    template<class T2>
    inline static void test(T2* pPtr)
    {
#if !defined(EA_COMPILER_NO_RTTI)
        assert((!pPtr || dynamic_cast<T*>(/*(T*)*/ pPtr)) && "BAD DYNAMIC CAST - WRONG TYPES");
#endif // EA_COMPILER_NO_RTTI
    }
};

}; // namespace MP
//////////////////////////////////////////////////////////////////////////




// USED IN "ref_ptr<T>"
class RefCounted
{
    mutable eastl::atomic<int> _ref_ptr_RefCount;
    mutable eastl::atomic<int> _ref_ptr_WeakRefCount;
    typedef int TRefInt;

    template<class T>
    friend void qd::details::referenced_static_delete(T* pThis);

    template<typename T>
    friend class qd::details::ref_ptr_base;
    template<typename T>
    friend class qd::details::ref_ptr_getter;
    template<typename T>
    friend class qd::details::wref_ptr_getter;

public:
    class CRefLock
    {
        const RefCounted& m_Obj;

    public:
        inline CRefLock(const RefCounted& pPtr)
            : m_Obj(pPtr)
        {
            m_Obj.ref_ptr_retain();
        }

        inline CRefLock(const RefCounted* pPtr)
            : m_Obj(*pPtr)
        {
            assert(pPtr);
            pPtr->ref_ptr_retain();
        }

        inline ~CRefLock() { m_Obj.ref_ptr_release(); }

        template<class T2>
        inline T2* get_() const
        {
            return m_Obj.get_<T2>;
        }
    }; // class CRefLock
    ////////////////////////////////////////////////////////////


    inline RefCounted()
        : _ref_ptr_RefCount(0)
        , _ref_ptr_WeakRefCount(0)
    {}

    inline RefCounted(const RefCounted& r)
        : _ref_ptr_RefCount(0)
        , _ref_ptr_WeakRefCount(0)
    {}

    // DEEP COPY SHOULD NO COPY REF_COUNTERS
    inline void operator= (const RefCounted& r) {}


    // STATIC_CAST TO ANY DERIVED
    template<class T2>
    inline T2* get_() const
    {
#if !defined EA_COMPILER_NO_RTTI
        assert((!c_def(this) || dynamic_cast<T2*>((qd::RefCounted*)this)) && "BAD DYNAMIC_CAST");
#endif // EA_COMPILER_NO_RTTI
        return static_cast<T2*>((T2*)this);
    }

#if !defined EA_COMPILER_NO_RTTI
    template<class T2>
    inline bool is_() const
    {
        return dynamic_cast<const T2*>(/*(const T2* )*/ this) !=
               0 /*nullptr*/; // NOT WORKED IF COMPLEX DERIVITY HIERACTION
    }
#endif // EA_COMPILER_NO_RTTI

    template<class T2>
    inline bool isSame_() const
    {
        return typeid(*this) == typeid(T2);
    }

    template<class T2>
    inline typename qd::MP::InnerType<T2>::type* get_if_() const
    {
#if !defined EA_COMPILER_NO_RTTI
        T2* p = dynamic_cast<T2*>((RefCounted*)this);
        return p;
#else
        return static_cast<T2*>(this);
#endif // EA_COMPILER_NO_RTTI
    }


    virtual ~RefCounted()
    {
        assert((!c_def(this) || (int)_ref_ptr_RefCount >= 0) && "ref_ptr<T> Already Removed!");
        // assert(_ref_ptr_RefCount == 0); // VERY STRICT RULE TO NOT DELETE REFERENCED OBJECTS
    }

    // qd::ref_retain_(this) - template instead
    inline RefCounted* ref_ptr_retain() /*const*/
    {
        this->_ref_ptr_retain();
        return this;
    }
    inline const RefCounted* ref_ptr_retain() const
    {
        this->_ref_ptr_retain();
        return this;
    }

    template<class TPtr>
    inline TPtr* ref_ptr_retain_() const
    {
        if (!c_def(this))
            return const_cast<TPtr*>(this);
        this->_ref_ptr_retain();
        return const_cast<TPtr*>(this);
    }

    inline TRefInt ref_ptr_release() const { return _ref_ptr_release(); }

    inline TRefInt ref_ptr_release_and_delete() /*const*/
    {
        if (!c_def(this))
            return 0;
        return _ref_ptr_release_and_delete(this);
    }

    TRefInt ref_ptr_count() const { return _ref_ptr_RefCount; }

    TRefInt wref_ptr_count() const { return _ref_ptr_WeakRefCount; }

    inline bool is_ref_ptr_valid() const { return c_def(this) && this->_is_ref_ptr_valid(); }

    inline static void operator delete (void* ptr) { ::operator delete (ptr); }

private:
    inline bool _is_ref_ptr_valid() const
    {
        assert((int)_ref_ptr_RefCount >= 0);
        return _ref_ptr_RefCount != 0;
    }

    EA_FORCE_INLINE void _ref_ptr_retain() const
    {
        assert((int)_ref_ptr_RefCount >= 0 && "BAD ref_ptr<T> POINTER");
        TRefInt r = ++_ref_ptr_RefCount;
        assert((int)r > 0);
        EA_UNUSED(r);
    }


    EA_FORCE_INLINE TRefInt _ref_ptr_release() const
    {
        assert((int)_ref_ptr_RefCount > 0);
        TRefInt r = (--_ref_ptr_RefCount);
        assert((int)r >= 0);
        return r;
    }

    template<class T>
    static inline TRefInt _ref_ptr_release_and_delete(T* pThis)
    {
        assert(/*c_def(this) &&*/ (int)pThis->_ref_ptr_RefCount > 0);
        TRefInt r = (--pThis->_ref_ptr_RefCount);
        if (r)
        {
            assert((int)r > 0); // check for overflow
            return r;
        }
        qd::details::referenced_static_delete<T>(pThis); // NO INLINE CALL
        return 0;
    }


    inline void wref_ptr_retain() const
    {
        assert((int)_ref_ptr_WeakRefCount >= 0 && "BAD ref_ptr<T> POINTER");
        ++_ref_ptr_WeakRefCount;
    }

    template<class T>
    static inline void _wref_ptr_release(T* pThis)
    {
        assert((int)pThis->_ref_ptr_WeakRefCount >= 0);
        if ((--pThis->_ref_ptr_WeakRefCount == 0) && (pThis->_ref_ptr_RefCount == 0))
        {
            T::operator delete (pThis);
            // qd::details::weak_ref_static_delete(this); // NOINLINE CALL
        }
    }


}; // class RefCounted
//////////////////////////////////////////////////////////////////////////




namespace qd::details {


template<typename T>
class ref_ptr_base;

//////////////////////////////////////////////////////////////////////////
template<typename T>
class ref_ptr_getter
{
    typedef ref_ptr_getter<T> TThis;

public:
    typedef ref_ptr<T> TRef_ptr;

    // 2022/07/11 - try not check valid() for ref_ptr<>
    inline static T* get(const ref_ptr_base<T>* pRefPtr) { return pRefPtr->_ptr; }

    template<class T2>
    static inline T2* get_(const ref_ptr_base<T>* pRefPtr)
    {
        assert((!(pRefPtr->_ptr) || pRefPtr->template is_<T2>()) && "Wrong class type while dynamic_cast<class>");
        return static_cast<T2*>((T2*)pRefPtr->_ptr);
    }


    static inline void destroy(const ref_ptr_base<T>* pRefPtr)
    {
        if (pRefPtr->_ptr && (RefCounted::_ref_ptr_release_and_delete(pRefPtr->_ptr) == 0))
        {
            pRefPtr->_ptr = nullptr;
        }
    }

    // RESET - don't free OLD_PTR
    static inline void reset(const ref_ptr_base<T>* pRefPtr, T* pNewInst)
    {
        // intentionally complex - simplification causes regressions
        typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
        (void)sizeof(type_must_be_complete);

        assert(pRefPtr->_ptr == nullptr);
        if (pNewInst)
        {
#if !defined EA_COMPILER_NO_RTTI
            assert(dynamic_cast<T*>(pNewInst) && "Wrong ref_ptr class assigned");
#endif // EA_COMPILER_NO_RTTI
            pNewInst->_ref_ptr_retain();
            pRefPtr->_ptr = pNewInst;
        }
        else
        {
            // pRefPtr->_ptr = nullptr;  // already must be a null
        }
    }


    // ASSIGN - free OLD_PTR and retain count of NEW PTR
    static inline void assign(const ref_ptr_base<T>* pRefPtr, T* pNewInst)
    {
        T* old_ptr = pRefPtr->_ptr;
        if (old_ptr == pNewInst) // not delete same pointer
            return;
        pRefPtr->_ptr = nullptr;
        TThis::reset(pRefPtr, pNewInst);

        if (old_ptr)
        {
            assert(old_ptr->is_ref_ptr_valid()); // safe valid
            RefCounted::_ref_ptr_release_and_delete(old_ptr);
        }
    }

    inline static void set_raw(ref_ptr_base<T>* pRefPtr, T* pNewPtr) { pRefPtr->_ptr = pNewPtr; }

}; // class ref_ptr_getter
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
template<typename T>
class wref_ptr_getter
{
public:
    typedef wref_ptr<T> TRef_ptr;

    inline static T* get(const ref_ptr_base<T>* pRefPtr) { return pRefPtr->valid() ? pRefPtr->_ptr : nullptr; }

    template<class T2>
    inline static T2* get_(const ref_ptr_base<T>* pRefPtr)
    {
        if (!pRefPtr->valid())
            return nullptr;
        assert(pRefPtr->template is_<T2>());
        return static_cast<T2*>((T2*)pRefPtr->_ptr);
    }

    static inline void destroy(const ref_ptr_base<T>* pRefPtr)
    {
        if (pRefPtr->_ptr)
        {
            RefCounted::_wref_ptr_release(pRefPtr->_ptr);
            pRefPtr->_ptr = nullptr;
        }
    }

    static inline void reset(const ref_ptr_base<T>* pRefPtr, T* pNewInst)
    {
        // intentionally complex - simplification causes regressions
        typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
        (void)sizeof(type_must_be_complete);

        assert(pRefPtr->_ptr == nullptr);

        if (!pNewInst)
            return;
        if ((pNewInst->_ref_ptr_RefCount) /*|| (pNewInst->_ref_ptr_WeakRefCount)*/)
        {
            pNewInst->wref_ptr_retain();
            pRefPtr->_ptr = pNewInst;
        }
    }


    static EA_FORCE_INLINE void assign(const ref_ptr_base<T>* pRefPtr, T* pNewInst)
    {
        T* tmp_ptr = pRefPtr->_ptr;

        pRefPtr->_ptr = nullptr;
        reset(pRefPtr, pNewInst);

        if (tmp_ptr)
        {
            RefCounted::_wref_ptr_release(tmp_ptr);
        }
    }

}; // class wref_ptr_getter
//////////////////////////////////////////////////////////////////////////



template<typename T>
class ref_ptr_base
{
    friend class qd::details::ref_ptr_getter<T>;
    friend class qd::details::wref_ptr_getter<T>;

protected:
    mutable T* _ptr = nullptr;

    inline ref_ptr_base() {}

private:
    inline ref_ptr_base(const ref_ptr_base& rp) = delete; // NO COPY CONSTRUCTOR
    ref_ptr_base& operator= (const ref_ptr_base&) { return *this; }

public:
    inline bool valid() const
    {
        RefCounted* pPtr = (RefCounted*)(this->_ptr);
        return pPtr != nullptr && (pPtr->_ref_ptr_RefCount != 0); // _ptr - may
    }

    inline bool refvalid() const
    {
        RefCounted* p = (RefCounted*)_ptr;
        return p ? p->is_ref_ptr_valid() : true;
    }

    inline bool is_null() const { return _ptr == nullptr; }

    inline T* _get_raw() const { return _ptr; }

    template<class T2>
    inline T2* _get_() const
    {
        if (!valid())
            return nullptr;
        assert(is_<T2>());
        return static_cast<T2*>(_ptr);
    }

    template<class T2>
    inline bool is_() const
    {
        if (!valid())
            return false;
#if !defined EA_COMPILER_NO_RTTI
        return dynamic_cast<const T2*>(_ptr) != nullptr;
#else
        return true;
#endif // EA_COMPILER_NO_RTTI
    }

    template<class T2>
    inline bool eq_() const
    {
        return valid() && typeid(*_ptr) == typeid(T2);
    }

}; // class ref_ptr_base
//////////////////////////////////////////////////////////////////////////






template<typename T, typename TRefPtrGetter = qd::details::ref_ptr_getter<T> >
class ref_ptr_base2 : public ref_ptr_base<T>
{
    typedef ref_ptr_base2<T, TRefPtrGetter> TThis;
    typedef ref_ptr_base<T> TSuper;

public:
    ref_ptr_base2() = default;
    ref_ptr_base2(const TThis&) = delete;
    ref_ptr_base2& operator= (const TThis&) = delete;

    inline T* get() const throw() { return TRefPtrGetter::get(this); }

    inline T* getsafe() const
    {
        if (!TSuper::valid())
            throw std::invalid_argument("ref_ptr is nullptr");
        return TSuper::_ptr;
    }

    inline T& getref() const { return *this->getsafe(); }

    inline const T& getcref() const { return static_cast<const T&>(*this->getsafe()); }

    template<class T2>
    inline T2* get_() const
    {
        return TRefPtrGetter::template get_<T2>(this);
    }

    inline operator bool () const { return get() != nullptr; }
    inline bool operator!() const { return !(this->operator bool ()); }

    template<class T2>
    inline const T2* get_const() const
    {
        assert(TSuper::template is_<T2>());
        return static_cast<const T2*>(get());
    }

    template<class T2>
    inline bool operator== (const T2& p) const
    {
        return (get() == (T*)get_ptr(p));
    }

    template<class T2>
    inline bool operator!= (const T2& p) const
    {
        return (get() != (T*)get_ptr(p));
    }

    inline bool operator== (const std::nullptr_t p) const { return !(this->operator bool ()); }

    inline bool operator!= (const std::nullptr_t p) const { return this->operator bool (); }

    inline bool operator< (const ref_ptr_base2& rp) const { return (get() < rp.get()); }

    inline bool operator> (const ref_ptr_base2& rp) const { return (get() > rp.get()); }

    inline bool operator> (const T* p) const { return (get() > p); }

    static const typename TRefPtrGetter::TRef_ptr& NullPtr()
    {
        static typename TRefPtrGetter::TRef_ptr pNullPtr;
        return pNullPtr;
    }

protected:
    inline void destroy() { TRefPtrGetter::destroy(this); }

    inline void assign(const T* pNewInst) { TRefPtrGetter::assign(this, const_cast<T*>(pNewInst)); }

    inline void reset(const T* pNewInst) { TRefPtrGetter::reset(this, const_cast<T*>(pNewInst)); }

    inline ~ref_ptr_base2() { TRefPtrGetter::destroy(this); }

}; // class ref_ptr_base2
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
/* NO_INLINE DELETER */
template<class T>
void referenced_static_delete(T* pThis)
{
    // SELF DELETE
    assert(pThis);
    assert(pThis->_ref_ptr_RefCount == 0);

    RefCounted* pRefThis(pThis); // RefThis Due to - Private destructors for some objects!

    uint32_t nWeakCount = uint32_t(++(pThis->_ref_ptr_WeakRefCount)); // inc to call destructor properly

    if (/*nStrongRefs == 0 &&*/ (--nWeakCount) == 0)
    {
        if (c_def(true) /*pRefThis->_ref_ptr_RefCount == 0*/)
        { // test once again to be sure //-V571
            pThis->_ref_ptr_WeakRefCount = 0;
            delete pRefThis;
            return;
        }
    }
    // CALL DESTRUCTOR FIRST
    // WARNING: MAY CALL DESTRUCTOR 2 TIMES
    else /*if( pRefThis->_ref_ptr_WeakRefCount != 0 )*/
    {

        // ALREADY DO ++(pThis->_ref_ptr_WeakRefCount); // inc to call destructor properly
        pRefThis->~RefCounted(); // CALL THE VIRTUAL DESTRUCTOR

        // RECURSIVE DELETETING
        if (--(pThis->_ref_ptr_WeakRefCount) == 0)
        {
            T::operator delete (pThis);
        }
    }
}


}; // namespace details
//////////////////////////////////////////////////////////////////////////


template<typename T>
class wref_ptr;

template<typename T>
class ptr;


//////////////////////////////////////////////////////////////////////////
template<typename XT>
class ref_ptr
    : public qd::details::ref_ptr_base2< /*T*/ typename std::remove_const<XT>::type,
          qd::details::ref_ptr_getter</*T*/ typename std::remove_const<XT>::type > >
{
    typedef typename std::remove_const<XT>::type T;
    typedef ref_ptr<T> TThis;
    typedef qd::details::ref_ptr_base2<T, qd::details::ref_ptr_getter<T> > TSuper;

public:
    typedef T TRefClass;

    operator T* () const
    {
        return this->_ptr; // TSuper::_get_raw();
    }

    T* operator->() const
    {
        assert(TSuper::valid() && "NULL POINTER EXCEPTION");
        return this->_ptr; // TSuper::_get_raw();
    }

    T& operator* ()
    {
        assert(TSuper::valid());
        return *TSuper::get();
    }

    const T& operator* () const
    {
        assert(TSuper::valid());
        return *TSuper::get();
    }

#ifdef RVALUE_REFERENCES_SUPPORTED
    inline ref_ptr(TThis&& rv)
    {
        this->_ptr = rv._ptr;
        rv._ptr = nullptr;
    }
    template<class T2>
    inline ref_ptr(ref_ptr<T2>&& rv)
    {
        qd::details::ref_ptr_getter<T2> pt;
        this->_ptr = pt.template get_<T>(&rv); // rv.get_<T>();
        pt.set_raw(&rv, nullptr); // rv._ptr = nullptr;
    }

#endif // RVALUE_REFERENCES_SUPPORTED

    inline ref_ptr() {}

    /*explicit*/ inline ref_ptr(T* p) { TSuper::reset(p); }

    /*explicit*/ inline ref_ptr(const T* p) { TSuper::reset(p); }


    inline ref_ptr(const ref_ptr<T>& rp)
    {
        // test with is_valid protection (by ref_ptr::get() ) ( RefCounted* already can't be have a null refCounter
        TSuper::reset(rp.gcc_only_template get());
    }

    inline ref_ptr(const ref_ptr<const T>& rp)
    {
        // test with is_valid protection (by ref_ptr::get() ) ( RefCounted* already can't be have a null refCounter
        TSuper::reset(rp.gcc_only_template get());
    }


    inline ref_ptr(const wref_ptr<T>& rp)
    {
        TSuper::reset(rp.gcc_only_template get()); // removed for clang
    }

    template<class T2>
    inline ref_ptr(const ref_ptr<T2>& rp)
    {
        // test with is_valid protection (by ref_ptr::get() ) ( RefCounted* already can't be have a null refCounter
        TSuper::reset(rp.template /*_get*/ get_<T>());
    }

    // WARNING: it's VERY DANGEROUS THING
    // 		template<typename T2>
    // 		inline ref_ptr(T2* p) { // TRYING TO RE-CAST POINTER FROM 'T2' to 'T'
    // 			TSuper::reset( p->template get_<T>() );
    // 		}


    template<class T2>
    inline ref_ptr(const wref_ptr<T2>& rp)
    {
        TSuper::reset(rp.template get_<T>());
    }

    inline ref_ptr(const std::nullptr_t&) {}

    inline const TThis& operator= (const std::nullptr_t&)
    {
        TSuper::assign((T*)nullptr);
        return *this;
    }

    template<class T2>
    inline ref_ptr(const ptr<T2>& rp)
    {
        TSuper::reset(rp.gcc_template get_<T>());
    }

    inline const TThis& operator= (T* pPtr)
    {
        TSuper::assign(pPtr);
        return *this;
    }

    inline TThis& reset(T* pPtr = nullptr)
    {
        TSuper::assign(pPtr);
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (T2* p2)
    {
        assert(!p2 || dynamic_cast<T*>(p2)); // tested
        TSuper::assign(static_cast<T*>((T*)p2));
        return *this;
    }

    inline const TThis& operator= (const ref_ptr<T>& rp)
    {
        TSuper::assign(rp.gcc_only_template _get_raw());
        return *this;
    }

    inline const TThis& operator= (ref_ptr<T>&& rv)
    {
        TSuper::destroy();
        this->_ptr = rv.template _get_<T>();
        rv._ptr = nullptr;
        return *this;
    }


    template<class T2>
    inline const TThis& operator= (const ref_ptr<T2>& rp)
    {
        TSuper::assign(rp.gcc_template _get_<T>());
        return *this;
    }

    inline const TThis& operator= (const wref_ptr<T>& rp)
    {
        TSuper::assign(rp.gcc_only_template get());
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (const wref_ptr<T2>& rp)
    {
        TSuper::assign(rp.gcc_template get_<T>());
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (const ptr<T2>& rp)
    {
        TSuper::assign(rp.gcc_template get_<T>());
        return *this;
    }


    static inline ref_ptr<T> make() { return new T(); }
    template<typename T1>
    static inline ref_ptr<T> make(const T1& p1)
    {
        return new T(p1);
    }
    template<typename T1, typename T2>
    static inline ref_ptr<T> make(const T1& p1, const T2& p2)
    {
        return new T(p1, p2);
    }
    template<typename T1, typename T2, typename T3>
    static inline ref_ptr<T> make(const T1& p1, const T2& p2, const T3& p3)
    {
        return new T(p1, p2, p3);
    }
    template<typename T1, typename T2, typename T3, typename T4>
    static inline ref_ptr<T> make(const T1& p1, const T2& p2, const T3& p3, const T4& p4)
    {
        return new T(p1, p2, p3, p4);
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    static inline ref_ptr<T> make(const T1& p1, const T2& p2, const T3& p3, const T4& p4, const T5& p5)
    {
        return new T(p1, p2, p3, p4, p5);
    }

    inline T* makeSelf()
    {
        T* pPtr = new T();
        TSuper::assign(pPtr);
        return pPtr;
    }
    template<typename T1>
    inline T* makeSelf(const T1& p1)
    {
        T* pPtr = new T(p1);
        TSuper::assign(pPtr);
        return pPtr;
    }
    template<typename T1, typename T2>
    inline T* makeSelf(const T1& p1, const T2& p2)
    {
        T* pPtr = new T(p1, p2);
        TSuper::assign(pPtr);
        return pPtr;
    }
    template<typename T1, typename T2, typename T3>
    inline T* makeSelf(const T1& p1, const T2& p2, const T3& p3)
    {
        T* pPtr = new T(p1, p2, p3);
        TSuper::assign(pPtr);
        return pPtr;
    }
    template<typename T1, typename T2, typename T3, typename T4>
    inline T* makeSelf(const T1& p1, const T2& p2, const T3& p3, const T4& p4)
    {
        T* pPtr = new T(p1, p2, p3, p4);
        TSuper::assign(pPtr);
        return pPtr;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    inline T* makeSelf(const T1& p1, const T2& p2, const T3& p3, const T4& p4, const T5& p5)
    {
        T* pPtr = new T(p1, p2, p3, p4, p5);
        TSuper::assign(pPtr);
        return pPtr;
    }

}; // class ref_ptr
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
template<typename T>
class wref_ptr : public qd::details::ref_ptr_base2< T, qd::details::wref_ptr_getter<T> >
{
    typedef wref_ptr<T> TThis;
    typedef qd::details::ref_ptr_base2< T, qd::details::wref_ptr_getter<T> > TSuper;

public:
    inline wref_ptr() {}

    inline wref_ptr(TThis&& rv)
    {
        this->_ptr = rv._ptr;
        rv._ptr = nullptr;
    }

    inline operator T* () const { return TSuper::get(); }
    inline T& operator* ()
    {
        assert(TSuper::valid());
        return *TSuper::get();
    }
    inline const T& operator* () const
    {
        assert(TSuper::valid());
        return *TSuper::get();
    }
    inline T* operator->() const
    {
        assert(TSuper::valid());
        return TSuper::get();
    }

    inline wref_ptr(T* p) { TSuper::reset(p); }

    //     WARNING: IT's VERY DANGEROUS THING
    // 		template<class T2>
    // 		inline wref_ptr(T2* p) {  // TRYING TO RE-CAST POINTER FROM 'T2' to 'T'
    // 			TSuper::reset( p->template get_<T>() );
    // 		}

    inline wref_ptr(const ref_ptr<T>& rp) { TSuper::reset(rp._get_raw()); }

    inline wref_ptr(const wref_ptr<T>& rp) { TSuper::reset(rp.get()); }

    template<class T2>
    inline wref_ptr(const ref_ptr<T2>& rp)
    {
        TSuper::reset(rp.template _get_<T>());
    }

    template<class T2>
    inline wref_ptr(const wref_ptr<T2>& rp)
    {
        TSuper::reset(rp.template get_<T>());
    }

    template<class T2>
    inline wref_ptr(const ptr<T2>& rp)
    {
        TSuper::reset(rp.gcc_template get_<T>());
    }

    template<class T2>
    inline const TThis& operator= (const ptr<T2>& rp)
    {
        TSuper::assign(rp.gcc_template get_<T>());
        return *this;
    }

// NULL
#if defined(COMPILER_NULLPTR_NOT_SUPPORTED)
    inline wref_ptr(const qd::CNullPtr&) {}
    inline const TThis& operator= (const qd::CNullPtr&)
    {
        TSuper::assign((T*)nullptr);
        return *this;
    }

#endif // COMPILER_NULLPTR_NOT_SUPPORTED

    inline wref_ptr(const std::nullptr_t&) {}
    inline const TThis& operator= (const std::nullptr_t&)
    {
        TSuper::assign((T*)nullptr);
        return *this;
    }


    inline const TThis& operator= (T* ptr)
    {
        TSuper::assign(ptr);
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (T2* ptr)
    {
        TSuper::assign(ptr->template get_<T>());
        return *this;
    }

    inline const TThis& operator= (const ref_ptr<T>& rp)
    {
        TSuper::assign(rp._get_raw());
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (const ref_ptr<T2>& rp)
    {
        TSuper::assign(rp.template _get_<T>());
        return *this;
    }

    inline const TThis& operator= (const wref_ptr<T>& rp)
    {
        TSuper::assign(rp./*gcc_template*/ get());
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (const wref_ptr<T2>& rp)
    {
        TSuper::assign(rp.gcc_template get_<T>());
        return *this;
    }

    inline TThis& reset(T* pPtr = nullptr)
    {
        TSuper::assign(pPtr);
        return *this;
    }

    template<class T2>
    inline const TThis& operator= (const TThis&& rv)
    {
        TSuper::destroy();
        this->_ptr = rv.template _get_<T>();
        rv._ptr = nullptr;
        return *this;
    }


}; // class wref_ptr
//////////////////////////////////////////////////////////////////////////



template<class T>
inline T* ref_retain_(T& _Obj)
{
    _Obj.ref_ptr_retain();
    return &_Obj;
}

template<class T>
inline T* ref_retain_(T* pObj)
{
    pObj->ref_ptr_retain();
    return pObj;
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
inline bool isPtrNull(const T& pPtr)
{
    return pPtr == (T)0;
}

// PTR VALID
template<class T>
inline bool isPtrValid(T* pPtr)
{
    return pPtr != nullptr;
}


template<typename T>
inline bool isPtrNull(const ptr<T>& pPtr)
{
    return !pPtr.valid();
}

template<typename T>
inline bool isPtrNull(const ref_ptr<T>& pPtr)
{
    return !pPtr.valid();
}

template<typename T>
inline bool isPtrNull(const qd::details::ref_ptr_base<T>& pPtr)
{
    return !pPtr.valid();
}


template<class T>
inline bool isPtrValid(const qd::details::ref_ptr_base<T>& pPtr)
{
    return pPtr.valid();
}



// DYNAMIC INHERITS qd::RefCounted for ref_ptr SUPPORT
template<class T>
class rptr
    : public T
    , public RefCounted
{
public:
    rptr() {}

    template<typename P1>
    rptr(P1& p1)
        : T(p1)
    {}

    template<typename P1, typename P2>
    rptr(P1& p1, P2& p2)
        : T(p1, p2)
    {}

    template<typename P1, typename P2, typename P3>
    rptr(P1& p1, P2& p2, P3& p3)
        : T(p1, p2, p3)
    {}

    virtual ~rptr() {}
}; // class rptr
//////////////////////////////////////////////////////////////////////////


//}; // namespace qd
