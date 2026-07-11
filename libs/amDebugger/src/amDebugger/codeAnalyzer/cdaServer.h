#pragma once
#include "qd/base/baseTypes.h"
#include "qd/base/compiler.h"
#include "qd/stl/unique_ptr.h"
#include "qd/stl/vector.h"
#include "amDebugger/base.h"
#include "amDebugger/vm/vmInterface.h"

typedef size_t csh;


namespace amD::cda {

class Item;
class CodeItem;

static constexpr uint32_t g_minOpSize = 2;
static constexpr uint32_t g_maxOpSize = 24; // max for M68
// How far behind the view's left edge we're willing to walk to find a real
// instruction boundary (anchor) to disassemble forward from.
static constexpr uint32_t g_maxAnchorBackGap = 4096;


class M68CodeDisassembler
{
    QD_SINGLETON_DECLARE(M68CodeDisassembler);

public:
    csh* m_pCapstone = nullptr;

    // Backing storage for the items handed out via requestM68DisasmLines()'s
    // outItems. Rebuilt in full on every call.
    qtd::vector<qtd::unique_ptr<cda::CodeItem>> m_curItems;

public:
    M68CodeDisassembler()
    {
        init();
    }

    void init();
    void destroy();

    // Disassembles a single, contiguous, monotonically-increasing run of
    // instructions covering [addr, ...) - at least nItems of them when the
    // underlying memory bank has enough room. When pProvedInstructionStart
    // points at a known-good instruction boundary (typically the current PC)
    // at or before addr, disassembly is anchored there so every boundary in
    // between is exact instead of guessed.
    void requestM68DisasmLines(IVm::VM* vm, AddrRef addr, int nItems, qtd::vector<amD::cda::Item*>* outItems,
        const AddrRef* pProvedInstructionStart = nullptr);

}; // class M68CodeDisassembler
//////////////////////////////////////////////////////////////////////////


}; // namespace amD::cda
