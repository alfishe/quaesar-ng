#include <qd/debug/exceptTryCatch.h>
#include <qd/debug/exception.h>


namespace qd {

bool qd::Exception::_assert_debug = false;


void qd::Exception::setError(const qtd::string& Error, EException::eType ErrType /*= EException::DEFAULT */) {
    m_Error = Error;
    m_ErrType = ErrType;
}


qd::Exception::Exception(EException::eType errType, const char* pError, ...) : m_ErrType(errType) {
    QD_TRY {
        va_list arg_list;
        va_start(arg_list, pError);
        setErrorMessageVA(pError, arg_list);
        QDASSERT_EX(0, "%s", m_Error.c_str());
    }
    QD_CATCH(...) {
        m_Error = "EXCEPTION ERROR in var_list";
        ASSERT_F(0, "EXCEPTION:%s", m_Error.c_str());
    };
}



qd::Exception::Exception(const char* pError, ...) : m_ErrType(EException::DEFAULT) {
    QD_TRY {
        va_list arg_list;
        va_start(arg_list, pError);
        setErrorMessageVA(pError, arg_list);
    }
    QD_CATCH(...) {
        ASSERT_F(0, "EXCEPTION - VA ERROR in Exception:'%s'", pError);
    };
    QDASSERT_EX(0, "%s", m_Error.c_str());
}


void qd::Exception::setErrorMessageVA(const char* pError, va_list& arg_list) {
    m_Error.clear();
    m_Error = qd::string_format(pError, arg_list);
}


};  // namespace qd
