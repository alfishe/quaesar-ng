#include "cdaServer.h"
#include <capstone/capstone.h>
#include "EASTL/optional.h"
#include "qd/math/mathBase.h"
#include "cdaTypes.h"
#include "qd/stl/fixed_vector.h"


namespace amD::cda {



void CodeAnalyzerServer::init()
{
    m_chunkUseHistory.clear();
    m_chunksQuadTree.clear();
    for (uint16_t i = 0; i < g_maxPages; ++i)
    {
        m_disasmChunkStorage[i].m_idx = i;
        if (i == 0)
            continue;
        m_chunkUseHistory.push_front(m_disasmChunkStorage[i]);
    }

    m_pCapstone = new csh();

    // TODO: Pick correct CPU depending on starting CPU
    cs_err err = cs_open(CS_ARCH_M68K, (cs_mode)(CS_MODE_BIG_ENDIAN | CS_MODE_M68K_000), m_pCapstone);
    if (err)
    {
        printf("Failed on cs_open() with error returned: %u\n", err);
        abort();
    }
    cs_option(*m_pCapstone, CS_OPT_DETAIL, CS_OPT_ON);
}


struct DisasmContext {
    AddrRef minAddr = 0;
    AddrRef maxAddr = 0;
    int offset = 0;
    CodeChunk* m_pPrevChunk = nullptr;
    CodeChunk* m_pCurrChunk = nullptr;

public:

    void setMinMaxAddr(AddrRef begAddr, AddrRef endAddr)
    {
        minAddr = (begAddr) & (~g_chunkMask);
        maxAddr = (endAddr + (g_chunkSize - 1)) & (~g_chunkMask);
    }

    void expandArea(AddrRef curAddr)
    {
        if (minAddr > curAddr)
            minAddr = curAddr;
        if (maxAddr < curAddr)
            maxAddr = curAddr;
        minAddr &= ~1u;
    }

    int getBytesCount() const
    {
        int n = (int)(maxAddr - minAddr) - offset;
        return n < 0 ? 0 : n;
    }

    AddrRef getStartAddr() const { return minAddr + offset; }

}; // struct DisasmContext
//////////////////////////////////////////////////////////////////////////



struct CapstoneDisasm {
    csh* m_pCapstone = nullptr;
    cs_insn* m_pInstructions = nullptr;
    size_t m_nInstructions = 0;

public:
    CapstoneDisasm(csh* pCapstone)
        : m_pCapstone(pCapstone)
    {
        cs_option(*m_pCapstone, CS_OPT_SKIPDATA, CS_OPT_ON);
    }

    void reset()
    {
        if (m_pInstructions)
            cs_free(m_pInstructions, m_nInstructions);
        m_pInstructions = nullptr;
        m_nInstructions = 0;
    }

    void disasm(IVm::VM* vm, AddrRef begAddr, AddrRef endAddr)
    {
        reset();
        const uint8_t* startDisasmDat = vm->mem->getRealAddr(begAddr);
        uint32_t countBytes = endAddr - begAddr;
        m_nInstructions =
            cs_disasm(*m_pCapstone, startDisasmDat, countBytes, begAddr, countBytes / 2, &m_pInstructions);
    }

    bool hasInstructionAddr(AddrRef addr) const
    {
        for (int i = 0; i < m_nInstructions; ++i)
        {
            if (m_pInstructions[i].address == addr)
                return true;
        }
        return false;
    }

    ~CapstoneDisasm() { reset(); }

}; // struct CapstoneDisasm
//////////////////////////////////////////////////////////////////////////


struct InstructionProcessor
{
    void processInstruction(const cs_insn &insn, CodeChunk* pCurPage)
    {
        AddrRef insnAddr = (AddrRef)insn.address;
        if (!pCurPage->isIn(insnAddr))
            return;

        if (!pCurPage)
        {
            assert(0);
            return;
        }

        amD::cda::CodeItem *pCodeInfo = new amD::cda::CodeItem();
        pCodeInfo->m_addr = (AddrRef)insn.address;
        pCodeInfo->m_bytesCount = insn.size;

        pCodeInfo->m_text = insn.mnemonic;
        do
        {
            pCodeInfo->m_text += ' ';
        } while (pCodeInfo->m_text.size() < 8);
        pCodeInfo->m_text += insn.op_str;

        pCodeInfo->m_bytesString.clear();
        for (uint16_t b = 0; b < insn.size; ++b)
            pCodeInfo->m_bytesString.append_sprintf("%02X", insn.bytes[b]);

        int ind = (pCodeInfo->m_addr - pCurPage->m_addr) / 2;
        assert(!pCurPage->m_codeItems[ind]);
        pCurPage->m_codeItems[ind] = pCodeInfo;
    }
};


void CodeAnalyzerServer::requestAnalyzedBlock(IVm::VM* vm, AddrRef reqAddr, int nItems,
    qd::vector<amD::cda::Item*>* outItems, const AddrRef* pAnchorAddr)
{
    outItems->clear();

    AddrRef begAddr = (reqAddr) & (~g_chunkMask) - g_chunkSize;
    AddrRef endAddr = (reqAddr + nItems * 2 + (g_chunkSize - 1)) & (~g_chunkMask);

    constexpr uint32_t algn = g_chunkSize - 1;
    DisasmContext dc;
    dc.setMinMaxAddr(reqAddr - algn, reqAddr + nItems * 2 + algn);

    eastl::optional<AddrRef> optAnchorAddr;
    if (*pAnchorAddr && qd::is_in_10(*pAnchorAddr, begAddr, endAddr))
        optAnchorAddr = *pAnchorAddr;

    dc.m_pPrevChunk = &requestCodeChunk(vm, begAddr - g_chunkSize);
    dc.m_pCurrChunk = &requestCodeChunk(vm, begAddr);
    assert(dc.m_pCurrChunk);

    for (;;)
    {
        if (!dc.m_pCurrChunk->m_codeValid)
        {
            AddrRef startFrom = dc.m_pPrevChunk->getDisasmCodeValidAddr(-1);

            CapstoneDisasm cpd(m_pCapstone);
            bool hasValidAddr = false;
            for (int offset = 0; offset < 8; ++offset)
            {
                cpd.disasm(vm, startFrom + offset, dc.m_pCurrChunk->m_addr + g_chunkSize + offset);
                if (cpd.hasInstructionAddr(startFrom + offset))
                    break;
            }
            dc.m_pCurrChunk->m_codeValid = true;

            // fill chunk pages with instructions
            InstructionProcessor proc;
            for (size_t i = 0; i < cpd.m_nInstructions; ++i)
            {
                const cs_insn& curInsn = cpd.m_pInstructions[i];
                if (curInsn.address < dc.m_pCurrChunk->m_addr)
                    continue;
                proc.processInstruction(curInsn, dc.m_pCurrChunk);
            }
        }

        // copy CodeItems to output
        for (CodeItem* curItem : dc.m_pCurrChunk->m_codeItems)
        {
            if (!curItem || curItem->m_addr < reqAddr)
                continue;
            outItems->push_back(curItem);
        }
        if (outItems->size() >= nItems)
            break;

        dc.m_pPrevChunk = dc.m_pCurrChunk;
        dc.m_pCurrChunk = &requestCodeChunk(vm, dc.m_pCurrChunk->m_addr + g_chunkSize);
    }

}


void CodeAnalyzerServer::destroy()
{
    SAFE_DELETE(m_pCapstone);
}


CodeChunk& CodeAnalyzerServer::getOrCreateCodePage(AddrRef addr, bool* bOutPageWasFound)
{
    bool bWasfound = true;
    uint16_t foundPageId = 0;
    addr = addr & (~g_chunkMask);
    if (!m_chunksQuadTree.querySingle(addr, &foundPageId))
    {
        CodeChunk& curChunk = m_chunkUseHistory.back();
        if (curChunk.m_addr)
        {
            m_chunksQuadTree.remove(curChunk.m_addr, curChunk.m_idx);
            curChunk.reset(); // reuse old chunk
        }
        assert(!curChunk.m_addr && curChunk.m_idx);
        assert(&m_disasmChunkStorage[curChunk.m_idx] == &curChunk && "not belongs to storage");
        curChunk.m_addr = addr;
        m_chunksQuadTree.insert(curChunk.m_addr, curChunk.m_idx);
        assert(m_chunksQuadTree.querySingle(addr, &foundPageId) && foundPageId == curChunk.m_idx);
        foundPageId = curChunk.m_idx;
        bWasfound = false;
    }
    assert(foundPageId != 0);
    if (bOutPageWasFound)
        *bOutPageWasFound = bWasfound;
    CodeChunk& curChunk = m_disasmChunkStorage[foundPageId];
    assert(curChunk.isIn(addr));
    return curChunk;
}


amD::cda::CodeChunk& CodeAnalyzerServer::requestCodeChunk(IVm::VM* vm, AddrRef addr)
{
    bool bExistChunk = false;
    CodeChunk& curCodeChunk = getOrCreateCodePage(addr, &bExistChunk);
    m_chunkUseHistory.splice(m_chunkUseHistory.begin(), curCodeChunk); // move page in used history to the front

    const uint8_t* pRealMem = vm->mem->getRealAddr(curCodeChunk.m_addr);
    if (!pRealMem)
    {
        curCodeChunk.m_codeValid = false;
        curCodeChunk.m_bytesValid = false;
        return curCodeChunk;
    }
    if (!bExistChunk || memcmp(curCodeChunk.m_bytes.data(), pRealMem, g_chunkSize) != 0)
    {
        // has differences
        curCodeChunk.removeCodeItems();
        memcpy(curCodeChunk.m_bytes.data(), pRealMem, g_chunkSize);
        curCodeChunk.m_bytesValid = true;
    }
    return curCodeChunk;
}


void CodeChunk::removeCodeItems()
{
    for (cda::CodeItem*& info : m_codeItems)
        SAFE_DELETE(info);
    m_codeValid = false;
}


amD::AddrRef CodeChunk::getDisasmCodeValidAddr(int off /*= 0*/) const
{
    if (!m_codeValid)
        return m_addr;
    if (off < 0)
    {
        for (auto rIt = m_codeItems.rbegin(); rIt != m_codeItems.rend(); ++rIt)
        {
            const CodeItem* curItem = *rIt;
            if (curItem)
                return curItem->m_addr;
        }
    }
    else
    {
        for (auto it = m_codeItems.begin(); it != m_codeItems.end(); ++it)
        {
            const CodeItem* curItem = *it;
            if (curItem)
                return curItem->m_addr;
        }
    }
    return m_addr;
}



}; // namespace amD::cda
