#pragma once
#include <qdIce/qdMem/fnvHash.h>
#include <stdint.h>


namespace qd {


struct ClassIdCC {
    const char* className = nullptr;
    uint32_t classNameHash = 0;

public:
    ClassIdCC() = default;

    inline constexpr ClassIdCC(const char* class_name, uint32_t class_hash)
        : className(class_name), classNameHash(class_hash) {
    }

    bool isSame(const ClassIdCC& rh) const {
        return classNameHash == rh.classNameHash;
    }

    template <typename TClassInd>
    static constexpr ClassIdCC makeByCC(TClassInd classInd) {
        return ClassIdCC(indToPrime(static_cast<uint32_t>(classInd)));
    }

    static constexpr ClassIdCC makeByInd(const char* class_name) {
        return {class_name, qd::fnv1aHash(class_name)};
    }

};  // struct ClassIdCC
//////////////////////////////////////////////////////////////////////////


#define CLASSID_CC_BASE(TName)                                                 \
private:                                                                       \
    typedef TName TThis;                                                       \
                                                                               \
public:                                                                        \
    constexpr static qd::ClassIdCC CLASSID = qd::ClassIdCC::makeByInd(#TName); \
    virtual const qd::ClassIdCC& classId() {                                   \
        return TName::CLASSID;                                                 \
    }                                                                          \
                                                                               \
private:


#define CLASSID_CC(TName, TBaseClass)                                          \
private:                                                                       \
    typedef TBaseClass TSuper;                                                 \
    typedef TName TThis;                                                       \
                                                                               \
public:                                                                        \
    constexpr static qd::ClassIdCC CLASSID = qd::ClassIdCC::makeByInd(#TName); \
    virtual const qd::ClassIdCC& classId() override {                          \
        return TName::CLASSID;                                                 \
    }                                                                          \
                                                                               \
private:


};  // namespace qd
