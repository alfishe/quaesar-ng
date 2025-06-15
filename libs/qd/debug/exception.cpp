#include <qd/debug/exceptTryCatch.h>
#include <qd/debug/exception.h>


namespace qd {

bool qd::Exception::_assert_debug = false;


void qd::Exception::setError(const qd::string& Error, EException::eType ErrType /*= EException::DEFAULT */) {
    m_Error = Error;
    m_ErrType = ErrType;
}


qd::Exception::Exception(EException::eType errType, const char* pError, ...) : m_ErrType(errType) {
    G_TRY {
        va_list arg_list;
        va_start(arg_list, pError);
        setErrorMessageVA(pError, arg_list);
        QDASSERT_EX(0, "%s", m_Error.c_str());
    }
    G_CATCH(...) {
        m_Error = "EXCEPTION ERROR in var_list";
        assert2(0, "EXCEPTION:%s", m_Error.c_str());
    };
}



qd::Exception::Exception(const char* pError, ...) : m_ErrType(EException::DEFAULT) {
    G_TRY {
        va_list arg_list;
        va_start(arg_list, pError);
        setErrorMessageVA(pError, arg_list);
    }
    G_CATCH(...) {
        assert(0 && "EXCEPTION - VA ERROR in Exception" && pError);
    };
    QDASSERT_EX(0, "%s", m_Error.c_str());
}


void qd::Exception::setErrorMessageVA(const char* pError, va_list& arg_list) {
    m_Error.clear();
    m_Error.append_sprintf_va_list(pError, arg_list);
}


};  // namespace qd
