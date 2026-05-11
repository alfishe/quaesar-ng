#pragma once
#include "qd/base/base.h"
#include "qd/stl/algorithm.h"

namespace qd {

//------------------------------------------------------------------------
template<typename T, bool ptr = true, typename B = bool>
struct InitOnDemand {
public:
    InitOnDemand() = default;
    InitOnDemand(const InitOnDemand&) = delete;
    ~InitOnDemand() {
        demandDestroy();
    }

    inline operator T* () const { return obj; }
    T& operator* () const { return *obj; }
    T* operator->() const { return obj; }
    T* get() const { return obj; }
    explicit operator bool () const { return obj != nullptr; }

    template<typename... Args>
    T* demandInit(Args&&... args) {
        if (!obj)
            obj = new T(qtd::forward<Args>(args)...);
        return obj;
    }
    void demandDestroy() { obj ? (delete obj, obj = nullptr) : (0); }

private:
    T* obj = nullptr;
}; // struct InitOnDemand
//////////////////////////////////////////////////////////////////////////


#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable :4582 4583) // warning C4582: 'InitOnDemand<...>::obj': constructor|destructor is not implicitly called
#endif

template<typename T, typename B>
struct InitOnDemand<T, false, B> {
public:
    InitOnDemand() {}
    InitOnDemand(const InitOnDemand&) = delete;
    ~InitOnDemand() { demandDestroy(); }


    operator T* () const { return getObj(); }
    T& operator* () const { return *getObj(); }
    T* operator->() const { return getObj(); }

    T* get() const { return ((bool)*this) ? getObj() : nullptr; }

    explicit operator bool () const {
        if constexpr (qtd::is_volatile_v<B>)
            return interlocked_relaxed_load(inited);
        else
            return inited;
    }

    template<typename... Args>
    T* demandInit(Args&&... args) {
        if (!inited) {
            new (objBuf) T(qtd::forward<Args>(args)...);
            if constexpr (qtd::is_volatile_v<B>)
                interlocked_release_store(inited, true);
            else
                inited = true;
        }
        return getObj();
    }
    void demandDestroy() {
        if constexpr (qtd::is_volatile_v<B>) {
            if (!interlocked_exchange(inited, false))
                return;
            getObj()->~T();
        }
        else if (inited) {
            getObj()->~T();
            inited = false;
        }
    }

private:
    T* getObj() const { return const_cast<T*>(&obj); }

private:
    union {
        alignas(T) char objBuf[sizeof(T)];
        T obj;
    };
    B inited = false;
};

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

}; // namespace qd
