#pragma once
#include <stdint.h>


namespace qd {


struct ClassIdPrime {
    uint32_t classId = 0;

public:
    ClassIdPrime() = default;

    inline constexpr ClassIdPrime(uint32_t _primeClassId) : classId(_primeClassId) {
    }
    bool isSubClassOf(const ClassIdPrime& rh) const;

    bool isSame(const ClassIdPrime& rh) const {
        return classId == rh.classId;
    }

    template <typename TClassInd>
    static constexpr ClassIdPrime makeByInd(TClassInd classInd) {
        return ClassIdPrime(indToPrime(static_cast<uint32_t>(classInd)));
    }

    static constexpr ClassIdPrime makeByInd(uint32_t classInd,
                                            const ClassIdPrime& baseId) {  // generate classId by prime numbers
        return ClassIdPrime(indToPrime(classInd) * baseId.classId);
    }

    inline constexpr operator uint32_t() const {
        return classId;
    }

    static constexpr uint32_t indToPrime(uint32_t ind);
};  // struct ClassIdPrime
//////////////////////////////////////////////////////////////////////////


#define CLASSID_PRIME(TName, TEnumIdx, TBaseClass) \
private:                                           \
    typedef TBaseClass TSuper;                     \
    typedef TName TThis;                           \
                                                   \
public:                                            \
    constexpr static Scene::ClassIdPrime CLASSID = \
        Scene::ClassIdPrime::makeByInd((uint32_t)TEnumIdx, TBaseClass::CLASSID);


};  // namespace qd
