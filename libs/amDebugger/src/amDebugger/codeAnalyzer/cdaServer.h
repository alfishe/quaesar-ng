#pragma once
#include "qd/base/types.h"
#include "qd/stl/vector.h"
#include "amDebugger/base.h"
#include "amDebugger/vm/vm.h"
#include "amDebugger/codeAnalyzer/QuadTreeAddrMap.h"
#include "EASTL/intrusive_list.h"

typedef size_t csh;


namespace amD::cda {

class Item;
class CodeItem;

static constexpr uint32_t g_m68MaxOpSize = 24;
static constexpr uint32_t g_chunkSize = 64; // 6 bit
static constexpr uint32_t g_chunkMask = g_chunkSize - 1;
static constexpr uint32_t g_maxPages = 64;


struct CodeChunk : public eastl::intrusive_list_node {
    AddrRef m_addr = 0;
    qd::array<uint8_t, cda::g_chunkSize> m_bytes;
    qd::array<cda::CodeItem*, cda::g_chunkSize / 2> m_codeItems = {};
    uint16_t m_idx = 0;
    union
    {
        uint16_t m_flags = 0;
        struct {
            bool m_codeValid : 1;
            bool m_bytesValid : 1;
        };
    };

    bool isValid() const { return m_codeValid; }
    bool isIn(AddrRef addr) const { return addr >= m_addr && (uint64_t)addr < ((uint64_t)m_addr + cda::g_chunkSize); }
    bool empty() const { return m_addr == 0; }
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


class CodeAnalyzerServer
{
    SINGLETON_DECLARE(CodeAnalyzerServer);
public:
    csh* m_pCapstone = nullptr;
    qd::array<CodeChunk, g_maxPages> m_disasmChunkStorage;
    eastl::intrusive_list<CodeChunk> m_chunkUseHistory;
    QuadTreeAddrMap<uint16_t, (32 - 6) / 2> m_chunksQuadTree;

public:
    CodeAnalyzerServer()
        : m_chunksQuadTree(0u, ~0u)
    {
        init();
    }

    void init();
    void destroy();

public:
    void requestAnalyzedBlock(amD::VM* vm, AddrRef addr, int nItems, qd::vector<amD::cda::Item *> *outItems, const AddrRef* pCheckAddr = nullptr);

    CodeChunk &requestCodeChunk(amD::VM *vm, AddrRef addr);

    CodeChunk &getOrCreateCodePage(AddrRef addr, bool *bOutPageWasFound = nullptr);


}; // class CodeAnalyzerServer
//////////////////////////////////////////////////////////////////////////


}; // namespace amD::cda
