#include "cdaServer.h"
#include <capstone/capstone.h>
#include "EASTL/optional.h"
#include "qd/math/mathBase.h"
#include "cdaTypes.h"
#include "qd/stl/fixed_vector.h"


namespace amD::cda {



void M68CodeDisassembler::init()
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



struct CapstoneDisassemblerContext {
    csh* m_pCapstone = nullptr;
    cs_insn* m_pInstructions = nullptr;
    size_t m_nInstructions = 0;

public:
    CapstoneDisassemblerContext(csh* pCapstone)
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
        if (!startDisasmDat)
            return;
        uint32_t countBytes = endAddr - begAddr;
        m_nInstructions =
            cs_disasm(*m_pCapstone, startDisasmDat, countBytes, begAddr, countBytes / 2, &m_pInstructions);
    }

    bool hasM68InstructionAddr(AddrRef addr) const
    {
        for (size_t i = 0; i < m_nInstructions; ++i)
        {
            if (m_pInstructions[i].address == addr)
                return true;
        }
        return false;
    }

    ~CapstoneDisassemblerContext() { reset(); }

}; // struct CapstoneDisassemblerContext
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


void M68CodeDisassembler::requestM68DisasmLines(IVm::VM* vm, AddrRef startAddr, int nLines,
    qd::vector<amD::cda::Item*>* outItems, const AddrRef* pProvedInstructionStart)
{
    outItems->clear();

    AddrRef begAddrPre = qd::clamp_max(startAddr - g_chunkSize, startAddr);
    AddrRef begAddr = (begAddrPre & ~g_chunkMask);
    AddrRef endAddr = (startAddr + nLines * 2 + (g_chunkSize - 1)) & (~g_chunkMask);

    constexpr uint32_t algn = g_chunkSize - 1;
    DisasmContext dc;
    dc.setMinMaxAddr(qd::clamp_max(startAddr - algn, startAddr), startAddr + nLines * 2 + algn);

    eastl::optional<AddrRef> optAnchorAddr;
    if (*pProvedInstructionStart && qd::is_in_10(*pProvedInstructionStart, begAddr, endAddr))
        optAnchorAddr = *pProvedInstructionStart;

    dc.m_pPrevChunk = &requestCodeChunk(vm, begAddrPre);
    dc.m_pCurrChunk = &requestCodeChunk(vm, begAddr);
    assert(dc.m_pCurrChunk);

    for (;;)
    {
        if (!dc.m_pCurrChunk->m_bCodeValid)
        {
            // disasm again changed code block
            AddrRef startFrom = dc.m_pPrevChunk->getDisasmCodeValidAddr(-1);

            CapstoneDisassemblerContext cpd(m_pCapstone);

            // try to find valid instruction start address
            for (int offset = 0; offset < 8; ++offset)
            {
                cpd.disasm(vm, startFrom + offset, dc.m_pCurrChunk->m_addr + g_chunkSize + offset);
                if (cpd.hasM68InstructionAddr(startFrom + offset))
                    break;
            }
            dc.m_pCurrChunk->m_bCodeValid = true;

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
            if (!curItem || curItem->m_addr < startAddr)
                continue;
            outItems->push_back(curItem);
        }
        if ((int)outItems->size() >= nLines)
            break;

        dc.m_pPrevChunk = dc.m_pCurrChunk;
        dc.m_pCurrChunk = &requestCodeChunk(vm, dc.m_pCurrChunk->m_addr + g_chunkSize);
    }
}


void M68CodeDisassembler::destroy()
{
    SAFE_DELETE(m_pCapstone);
}


CodeChunk& M68CodeDisassembler::getOrCreateCodePage(AddrRef addr, bool* bOutPageWasFound)
{
    bool bWasfound = true;
    uint16_t foundPageId = 0;
    addr = addr & (~g_chunkMask);
    if (!m_chunksQuadTree.querySingle(addr, &foundPageId))
    {
        // get last unused chunk
        CodeChunk& curChunk = m_chunkUseHistory.back();
        if (curChunk.m_bAddrValid)
        {
            m_chunksQuadTree.remove(curChunk.m_addr, curChunk.m_idx);
            curChunk.reset(); // reuse old chunk
        }
        assert(!curChunk.m_addr && curChunk.m_idx);
        assert(&m_disasmChunkStorage[curChunk.m_idx] == &curChunk && "not belongs to storage");
        curChunk.m_addr = addr;
        curChunk.m_bAddrValid = true;
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


amD::cda::CodeChunk& M68CodeDisassembler::requestCodeChunk(IVm::VM* vm, AddrRef addr)
{
    bool bExistChunk = false;
    CodeChunk& curCodeChunk = getOrCreateCodePage(addr, &bExistChunk);
    m_chunkUseHistory.splice(m_chunkUseHistory.begin(), curCodeChunk); // move page in used history to the front

    const uint8_t* pRealMem = vm->mem->getRealAddr(curCodeChunk.m_addr);
    if (!pRealMem)
    {
        curCodeChunk.m_bCodeValid = false;
        curCodeChunk.m_bBytesValid = false;
        return curCodeChunk;
    }
    if (!bExistChunk || memcmp(curCodeChunk.m_bytes.data(), pRealMem, g_chunkSize) != 0)
    {
        // has differences
        curCodeChunk.removeCodeItems();
        memcpy(curCodeChunk.m_bytes.data(), pRealMem, g_chunkSize);
        curCodeChunk.m_bBytesValid = true;
    }
    return curCodeChunk;
}


void CodeChunk::removeCodeItems()
{
    for (cda::CodeItem*& info : m_codeItems)
        SAFE_DELETE(info);
    m_bCodeValid = false;
}


amD::AddrRef CodeChunk::getDisasmCodeValidAddr(int off /*= 0*/) const
{
    if (!m_bCodeValid)
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
