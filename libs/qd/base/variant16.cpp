#include "variant16.h"


namespace qd
{



bool Var16::operator== (const Var16& r) const
{
    if (memcmp(this, &r, g_sizeOf) != 0)
        return false;
    return true;
}


const qd::Var16& Var16::operator= (const Var16& in_clone)
{
    _headData = in_clone._headData;
    _bodyData = in_clone._bodyData;
    return *this;
}

#if 0

void VariantFast::setString(const qd::string_view& Value)
{
    if (m_type == DATA_STRING)
    {
        Details::CAnyTypeHolder_<qd::string>* pVar = (Details::CAnyTypeHolder_<qd::string>*)m_pExtHolder;
        pVar->m_Value = Value;
    }
    else
    {
        reset();
        _setString(Value);
    }
}


const qd::string_view& VariantFast::getString() const
{
    assert(m_type == DATA_STRING);
    Details::CAnyTypeHolder_<qd::string>* pVal = getPtr_< Details::CAnyTypeHolder_<qd::string> >();
    return pVal->getValue();
}


bool VariantFast::toFloat(float& val) const
{
    switch (m_type)
    {
    case VariantFast::DATA_INT32:
        val = (float)m_Int32;
        return true;
    case VariantFast::DATA_UINT32:
        val = (float)m_UInt32;
        return true;
    case VariantFast::DATA_FLOAT:
        val = (float)m_Float;
        return true;
    case VariantFast::DATA_BOOL:
        val = (float)m_Bool;
        return true;
    default:
        throw Exception("User Data has no valid Value");
    }
}


bool VariantFast::toInt32(int& val) const
{
    switch (m_type)
    {
    case VariantFast::DATA_INT32:
    case VariantFast::DATA_UINT32:
    case VariantFast::DATA_BOOL:
        val = (int)m_Int32;
        return true;
    case VariantFast::DATA_FLOAT:
        val = (int)m_Float;
        return true;
    default:
        return false;
        // throw Exception("User Data has no valid Value");
    }
}


void VariantFast::_setString(const qd::string_view& Value)
{
    Details::CAnyTypeHolder_<qd::string>* pVar = new Details::CAnyTypeHolder_<qd::string>(Value);
    m_pPointer = (void*)pVar;
    m_type = DATA_STRING;
    m_SizeOf = (uint8_t)sizeof(void*);
}


bool VariantFast::toString(qd::string& outVal) const
{
    switch (m_type)
    {
    case VariantFast::DATA_INT32:
        outVal = qd::string("%1").argI(m_Int32);
        return true;
    case VariantFast::DATA_UINT32:
        outVal = qd::string("%1").argU(m_UInt32);
        return true;
    case VariantFast::DATA_BOOL:
        outVal = qd::string("%1").argB(m_Bool != 0);
        return true;
    case VariantFast::DATA_FLOAT:
        outVal = stringFormat("%f", m_Float);
        return true;
    case VariantFast::DATA_INT64:
        outVal = stringFormat("%"
                              "lld" /*PRId64*/,
            m_Int64);
        return true;
    case VariantFast::DATA_UINT64:
        outVal = stringFormat("%"
                              "llu" /*PRIu64*/,
            m_UInt64);
        return true;
    case VariantFast::DATA_REF_PTR:
    case VariantFast::DATA_POINTER:
        outVal = stringFormat("0x%X", getPtr());
        return true;

    case VariantFast::DATA_STRING:
        outVal = getString();
        return true;
    case VariantFast::DATA_STRING_W:
        outVal = getStringW().toStr();
        return true;
    default:
        return false;
    }
}


bool VariantFast::operator< (const VariantFast& r) const
{
    if (m_type != r.m_type)
        return m_type < r.m_type;

    switch (m_type)
    {
    case VariantFast::DATA_BOOL:
        return m_Bool < r.m_Bool;
    case VariantFast::DATA_INT32:
        return m_Int32 < r.m_Int32;
    case VariantFast::DATA_FLOAT:
        return m_Float < r.m_Float;
    case VariantFast::DATA_UINT32:
        return m_UInt32 < r.m_UInt32;
    case VariantFast::DATA_STRING:
        return getString() < r.getString();
    case VariantFast::DATA_STRING_W:
        return getStringW() < r.getStringW();
    case VariantFast::DATA_POINTER:
    case VariantFast::DATA_REF_PTR:
        return m_pPointer < r.m_pPointer;
    default:
        assert(0 && "CAnyValue: operator< ERROR: Can't Compare Empty or Unknown Values");
        return false;
    }
}


#endif //


void Var16::reset()
{
    _headData = 0;
    _bodyData = 0;
}

}; // namespace qd
