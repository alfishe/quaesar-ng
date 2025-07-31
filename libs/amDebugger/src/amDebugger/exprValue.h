#pragma once
#include "qd/base/base.h"
#include "qd/stl/string.h"
#include "qd/base/variant16.h"

FORWARD_DECLARATION_2(ParserOop, Expr);
FORWARD_DECLARATION_2(amD, AbsEmu);

namespace amD
{
	class ExprValStr
	{
	qd::string m_strVal;
    ParserOop::Expr *m_pParsedExpr = nullptr;
	
	public:
	
	    const qd::string& getStrVal() const {
	        return m_strVal;
	    }
	    void setStrVal(const qd::string_view& Val) {
	        if (m_strVal == Val)
	            return;
	        m_strVal = Val;
	        parse();
	    }
	
	    void parse();
        bool evaluate(const amD::AbsEmu* vm, qd::Var16& res);

        ~ExprValStr();

	}; // class ExprValStr
	
	
}; // namespace amD
