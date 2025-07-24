#pragma once
#include "qd/base/base.h"
#include "qd/base/types.h"
#include "qd/stl/vector.h"
#include "qd/stl/string.h"
#include "amDebugger/base.h"


namespace amD::cda {

enum class EItemType : uint8_t {
    Label,
    Code,
    Data,
    CommentBlock,
    CommentLine,
    FunctionDescLine,

    Unknown
};

enum class EOperandType : uint8_t {
    Unknown = 0,
};


#define DECLARE_CDA_ITEM_TYPE(in_type)                     \
public:                                                    \
    static constexpr amD::cda::EItemType s_type = in_type; \
    amD::cda::EItemType m_type = in_type; \



class Item
{
public:
    DECLARE_CDA_ITEM_TYPE(EItemType::Unknown);
    AddrRef m_addr = 0; // optional operand address
    uint16_t m_bytesCount = 0;
    qd::string m_bytesString;

public:
    Item(EItemType type_ = EItemType::Unknown)
        : m_type(type_)
    {
    }

    template<class TItem>
    TItem* cast_() const
    {
        Item* pThis = const_cast<Item*>(this);
        if (pThis->m_type != TItem::s_type)
            return nullptr;
        return static_cast<TItem*>(pThis);
    }

    virtual ~Item() = default;
}; // class Item
//////////////////////////////////////////////////////////////////////////


class CodeItem : public Item
{
    DECLARE_CDA_ITEM_TYPE(EItemType::Code);
public:
    EOperandType m_operandType = EOperandType::Unknown;
    qd::string m_text; // Disassembly text

public:
    CodeItem() : Item(EItemType::Code) {}
    virtual ~CodeItem() = default;

}; // class CodeItem
//////////////////////////////////////////////////////////////////////////



class DataInfo : public Item
{
    DECLARE_CDA_ITEM_TYPE(EItemType::Data);
public:
    DataInfo()
        : Item(EItemType::Data)
    {}

}; // class CodeItem


}; // namespace amD::cda
