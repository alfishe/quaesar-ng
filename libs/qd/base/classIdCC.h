#pragma once
#include <qd/mem/fnvHash.h>
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


    static constexpr ClassIdCC makeByInd(const char* class_name) {
        return {class_name, qd::fnv1aHash(class_name)};
    }

};  // struct ClassIdCC
//////////////////////////////////////////////////////////////////////////


#define CLASSID_CC_BASE(TName)                                                  \
private:                                                                        \
    typedef TName TThis;                                                        \
                                                                                \
public:                                                                         \
    constexpr static qd::ClassIdCC CLASS_ID = qd::ClassIdCC::makeByInd(#TName); \
    virtual const qd::ClassIdCC& getClassId() const {                           \
        return TName::CLASS_ID;                                                 \
    }                                                                           \
                                                                                \
private:


#define CLASSID_CC(TName, TBaseClass)                                           \
private:                                                                        \
    typedef TBaseClass TSuper;                                                  \
    typedef TName TThis;                                                        \
                                                                                \
public:                                                                         \
    constexpr static qd::ClassIdCC CLASS_ID = qd::ClassIdCC::makeByInd(#TName); \
    virtual const qd::ClassIdCC& getClassId() const override {                  \
        return TName::CLASS_ID;                                                 \
    }                                                                           \
                                                                                \
private:


};  // namespace qd
