#include "exprValue.h"
#include "exprParser/parser/common.h"
#include "exprParser/parser/parser_oop.h"
#include "exprParser/parser/resolve_oop.h"
#include "qd/log/log.h"


namespace amD {
class AmDbgResolver : public ExprResolver
{
public:
    ExprCallback0 resolveFunc0(const char* name);
    ExprCallback1 resolveFunc1(const char* name);
    ExprCallback2 resolveFunc2(const char* name);
    ExprCallback3 resolveFunc3(const char* name);
    bool resolveVariable(const char* name, ExprValuePtr& result);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resolver

ExprCallback0 AmDbgResolver::resolveFunc0(const char* /*name*/)
{
    //     if (!strcmp(name, "fn0"))
    //         return fn0;
    return nullptr;
}

ExprCallback1 AmDbgResolver::resolveFunc1(const char* /*name*/)
{
    //     if (!strcmp(name, "fn1"))
    //         return fn1;
    return nullptr;
}

ExprCallback2 AmDbgResolver::resolveFunc2(const char* /*name*/)
{
    //     if (!strcmp(name, "fn2"))
    //         return fn2;
    return nullptr;
}

ExprCallback3 AmDbgResolver::resolveFunc3(const char* /*name*/)
{
    //     if (!strcmp(name, "fn3"))
    //         return fn3;
    return nullptr;
}

bool AmDbgResolver::resolveVariable(const char* name, ExprValuePtr& result)
{
    if (!strcmp(name, "var.32") || !strcmp(name, "var_32"))
    {
        // result.ptr = &value32;
        result.sizeInBytes = 4;
        return true;
    }
    return false;
}


class AmDbgEvaluator : public ExprEvaluator
{
public:
    AmDbgEvaluator()
        : ExprEvaluator(0)
    {}

    uint8_t memByte(ExprValue address) const { return address + 0x10; }
    uint16_t memWord(ExprValue address) const { return address - 0xb0; }
    uint32_t memDword(ExprValue address) const { return address * 4; }
};

void ExprValStr::parse()
{
    SAFE_DELETE(m_pParsedExpr);
    try
    {
        AmDbgResolver r;
        m_pParsedExpr = ParserOop::Expr::parse(m_strVal.c_str(), r);
    }
    catch (const ExprError& e)
    {
        qd::log_debug("[ FAIL ] ParserOop: \"%s\" unexpected error: %s\n", m_strVal.c_str(), e.message());
    }
}


bool ExprValStr::evaluate(const IVm::VM* /*vm*/, qd::Var16& res)
{
    if (!m_pParsedExpr)
        return false;

    AmDbgEvaluator e;
    ExprValue result = m_pParsedExpr->evaluate(e);

    res.setU32((uint32_t)result);
    return true;
}


ExprValStr::~ExprValStr()
{
    SAFE_DELETE(m_pParsedExpr);
}


}; // namespace amD
