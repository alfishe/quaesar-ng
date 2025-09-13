#pragma once
#include <stdint.h>
#include <deque>
#include "qd/debug/assert.h"


namespace qd {

constexpr uint32_t index_to_prime(uint32_t ind);


struct ClassPrimeId {
    static constexpr uint64_t DEFAULT_PRIME_ID = 1;

    uint64_t m_primeId = DEFAULT_PRIME_ID;

public:
    ClassPrimeId() = default;

    //inline constexpr ClassPrimeId(uint64_t _primeClassId) : m_primeId(_primeClassId) {}

    bool isDerivedFrom(const ClassPrimeId& rh) const;

    bool isSame(const ClassPrimeId& rh) const {
        return m_primeId == rh.m_primeId;
    }

//     template <typename TClassInd>
//     static constexpr ClassPrimeId makeByInd(TClassInd classInd) {
//         return ClassPrimeId(index_to_prime(static_cast<uint32_t>(classInd)));
//     }
//
//     static constexpr ClassPrimeId makeByInd(uint32_t classInd,
//                                             const ClassPrimeId& baseId) {  // generate classId by prime numbers
//         return ClassPrimeId(index_to_prime(classInd) * baseId.m_primeId);
//     }

    static constexpr ClassPrimeId makeByPrimeId(uint64_t primeId) {  // generate classId by prime numbers
        ClassPrimeId r;
        r.m_primeId = primeId;
        return r;
    }

    inline constexpr operator uint64_t() const {
        return m_primeId;
    }

    bool operator<(const ClassPrimeId& rh) const {
        return m_primeId < rh.m_primeId; }

    bool operator==(const ClassPrimeId& rh) const { return m_primeId == rh.m_primeId; }

    bool isValid() const { return m_primeId != DEFAULT_PRIME_ID; }

    void addBaseClass(const ClassPrimeId& baseId) {
        assert(!isDerivedFrom(baseId));
        m_primeId *= baseId.m_primeId;
    }

};  // struct ClassPrimeId
//////////////////////////////////////////////////////////////////////////


#define CLASSID_PRIME(TName, TEnumIdx, TBaseClass)  \
private:                                            \
    typedef TBaseClass TSuper;                      \
    typedef TName TThis;                            \
                                                    \
public:                                             \
    constexpr static Scene::ClassPrimeId CLASS_ID = \
        Scene::ClassPrimeId::makeByInd((uint32_t)TEnumIdx, TBaseClass::CLASS_ID);



class ClassPrimeIdMgr
{
protected:
    std::deque<ClassPrimeId> m_storage;

public:
    const ClassPrimeId& registerNewType();

}; // class ClassPrimeIdMgr
//////////////////////////////////////////////////////////////////////////


template<class T>
class ClassPrimeIdMgr_ : public ClassPrimeIdMgr
{
    typedef ClassPrimeIdMgr_<T> TThis;

public:
    static TThis& get()
    {
        static TThis inst;
        return inst;
    }
};

};  // namespace qd
//////////////////////////////////////////////////////////////////////////


namespace eastl {
    template<>
    struct hash<qd::ClassPrimeId> {
        size_t operator()(const qd::ClassPrimeId& id) const noexcept {
            return eastl::hash<uint64_t>()(id.m_primeId);
        }
    };
}
