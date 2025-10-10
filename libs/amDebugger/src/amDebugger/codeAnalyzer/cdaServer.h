#pragma once
#include <EASTL/intrusive_list.h>
#include "amDebugger/base.h"
#include "amDebugger/codeAnalyzer/quadTreeAddrMap.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/baseTypes.h"
#include "qd/stl/vector.h"

typedef size_t csh;


namespace amD::cda {

class Item;
class CodeItem;

static constexpr uint32_t g_minOpSize = 2;
static constexpr uint32_t g_maxOpSize = 24; // max for M68
static constexpr uint32_t g_chunkSizeInBits = 6; // for quad tree depth
static constexpr uint32_t g_chunkSize = (1 << g_chunkSizeInBits); // 64 bytes per chunk
static constexpr uint32_t g_chunkMask = g_chunkSize - 1;
static constexpr uint32_t g_maxPages = 64;


struct CodeChunk : public eastl::intrusive_list_node {
    AddrRef m_addr = 0;
    qd::array<uint8_t, cda::g_chunkSize> m_bytes = {}; // copy of memory for comparison
    qd::array<cda::CodeItem*, cda::g_chunkSize / 2> m_codeItems = {};
    uint16_t m_idx = 0;
    union {
        EA_DISABLE_VC_WARNING(4201) // nameless struct/union
        uint16_t m_flags = 0;
        struct {
            bool m_bAddrValid :1;
            bool m_bCodeValid :1;
            bool m_bBytesValid :1;
        };
    };

    bool isValid() const { return m_bCodeValid; }
    bool isIn(AddrRef addr) const { return addr >= m_addr && (uint64_t)addr < ((uint64_t)m_addr + cda::g_chunkSize); }
    bool empty() const { return m_bAddrValid == false; }
    void removeCodeItems();

    void reset()
    {
        m_addr = 0;
        m_flags = 0;
        removeCodeItems();
    }

    AddrRef getDisasmCodeValidAddr(int off = 0) const;

    ~CodeChunk() { reset(); }

}; // struct CodeChunk
//////////////////////////////////////////////////////////////////////////


class M68CodeDisassembler
{
    SINGLETON_DECLARE(M68CodeDisassembler);

public:
    csh* m_pCapstone = nullptr;
    qd::array<CodeChunk, g_maxPages> m_disasmChunkStorage; // Cached storage disasm lines as pages
    eastl::intrusive_list<CodeChunk> m_chunkUseHistory;
    amD::QuadTreeAddrMap<uint16_t, (32 - g_chunkSizeInBits) / g_minOpSize> m_chunksQuadTree;

public:
    M68CodeDisassembler()
        : m_chunksQuadTree(0u, ~0u)
    {
        init();
    }

    void init();
    void destroy();
    void requestM68DisasmLines(IVm::VM* vm, AddrRef addr, int nItems, qd::vector<amD::cda::Item*>* outItems,
        const AddrRef* pCheckAddr = nullptr);

protected:
    CodeChunk& requestCodeChunk(IVm::VM* vm, AddrRef addr);
    CodeChunk& getOrCreateCodePage(AddrRef addr, bool* bOutPageWasFound = nullptr);

}; // class M68CodeDisassembler
//////////////////////////////////////////////////////////////////////////


}; // namespace amD::cda
