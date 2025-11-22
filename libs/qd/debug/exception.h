#pragma once
#include "qd/debug/assert.h"
#include "qd/debug/exceptTryCatch.h"
#include "qd/enum/enumBase.h"
#include "qd/stl/string.h"


namespace qd {

struct EException {
    enum eType {
        DEFAULT = 0,
        IO_ERROR,
        OPERATION_ERR,
        NOT_FOUND,
        LOGIC_ERROR,
        RUNTIME_ERROR,
        INVALID_ARGUMENT,
        OUT_OF_RANGE,
        RANGE_ERROR,
        BAD_ALLOC,
        NOT_SUPPORTED,
    };
    ENUM_DECLARE_BASE(qd::, EException, eType, EException::DEFAULT);
}; // struct EException
//////////////////////////////////////////////////////////////////////////


#if !defined(RELEASE)
#define QDASSERT_EX(Val, ...)           \
    qd::Exception::debugBreakPoint();   \
    if (qd::Exception::isDebugAssert()) \
        assert2(Val, __VA_ARGS__);
#else
#define QDASSERT_EX(Val, ...)
#endif


//////////////////////////////////////////////////////////////////////////
class Exception : public std::exception
{
    EException::eType m_ErrType;
    qd::string m_Error;
    static bool _assert_debug;

protected:
    void setError(const qd::string& Error, EException::eType ErrType = EException::DEFAULT);

public:
    void debugBreakPoint() { c_def(0); }

    static bool isDebugAssert() { return _assert_debug; }

    explicit Exception(bool bAssert = true)
        : m_ErrType(EException::DEFAULT) {
        if (bAssert) {
            QDASSERT_EX(0, "Unnamed Exception Found!");
        }
    }

    explicit Exception(EException::eType errType)
        : m_ErrType(errType) {
        QDASSERT_EX(0, "Base Exception ERROR");
    }

    explicit Exception(EException::eType errType, const char* pError, ...);

    explicit Exception(EException::eType errType, const string& Error)
        : m_ErrType(errType)
        , m_Error(Error) {
        QDASSERT_EX(0, "%s", m_Error.c_str());
    }

    explicit Exception(const qd::string& Error)
        : m_ErrType(EException::DEFAULT)
        , m_Error(Error) {
        QDASSERT_EX(0, "%s", m_Error.c_str());
    }

    explicit Exception(const char* pError, ...);


    qd::EException getErrType() const { return EException(m_ErrType); }

    void setErrorType(qd::EException errType) { m_ErrType = errType; }

    void setErrorMessageVA(const char* pError, va_list& arg_list);

    explicit Exception(const std::exception& Exception)
        : m_ErrType(EException::DEFAULT)
        , m_Error(Exception.what()) {}

    virtual ~Exception() throw() {}

    const string& getError() const { return m_Error; }

    virtual const char* what() const throw() override { return m_Error.c_str(); }


}; // class Exception
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
