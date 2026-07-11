#include "cdaServer.h"
#include "capstone/capstone.h"  // quotes, not <>: ensures bundled header wins over homebrew
#include <stddef.h>  // offsetof
#include "qd/stl/optional.h"
#include "qd/math/mathBase.h"
#include "cdaTypes.h"
#include "qd/stl/fixed_vector.h"
#include "qd/log/log.h"


namespace amD::cda {



void M68CodeDisassembler::init()
{
    m_curItems.clear();

    m_pCapstone = new csh();

    // TODO: Pick correct CPU depending on starting CPU
    cs_err err = cs_open(CS_ARCH_M68K, (cs_mode)(CS_MODE_BIG_ENDIAN | CS_MODE_M68K_000), m_pCapstone);
    if (err)
    {
        qd::logErr("cdaServer: cs_open FAILED err=%u", err);
        return;
    }
    qd::logInfo("cdaServer: cs_open OK sizeof(cs_insn)=%zu", sizeof(cs_insn));
    // Do NOT enable CS_OPT_DETAIL or CS_OPT_SKIPDATA for M68K:
    // - DETAIL+SKIPDATA conflict makes the detail pointer invalid.
    // - SKIPDATA produces pseudo-instructions whose size exceeds the
    //   24-byte bytes[] array, causing buffer overruns.
    // Without SKIPDATA, cs_disasm_iter stops cleanly at undecodable data.
}

void M68CodeDisassembler::destroy()
{
    m_curItems.clear();
    if (m_pCapstone)
    {
        if (*m_pCapstone)
            cs_close(m_pCapstone);
        SAFE_DELETE(m_pCapstone);
    }
}


// ──────────────────────────────────────────────────────────────────────────
// Memory reader: fetches bytes from UAE via the IVm interface.
// ──────────────────────────────────────────────────────────────────────────
struct MemReader
{
    IVm::VM* vm = nullptr;
    qtd::vector<uint8_t> buf;

    void read(AddrRef addr, uint32_t nBytes)
    {
        if (!vm || nBytes == 0)
            return;
        buf.resize(nBytes);
        for (uint32_t i = 0; i < nBytes; i += 2)
        {
            uint16_t w = vm->mem->getU16(addr + i);
            buf[i]     = (uint8_t)(w >> 8);
            if (i + 1 < nBytes)
                buf[i + 1] = (uint8_t)(w & 0xFF);
        }
        // If nBytes is odd, the last byte was already written above as
        // the low byte of the final word read — but we may have read one
        // extra word.  That's fine; buf is sized to nBytes.
    }
};


// ──────────────────────────────────────────────────────────────────────────
// Iterator-based decoder:  uses cs_disasm_iter() which decodes ONE
// instruction at a time into a single caller-owned cs_insn.  This avoids
// cs_disasm()'s internal allocation of an cs_insn array, where a struct-
// layout mismatch (the bundled header is newer than what the library
// binary was built against) can corrupt every field after `id`.
// ──────────────────────────────────────────────────────────────────────────
static void decode_range(
    csh handle, IVm::VM* vm,
    AddrRef begAddr, uint32_t nBytes,
    int maxItems,
    qtd::vector<qtd::unique_ptr<CodeItem>>* outStorage,
    qtd::vector<cda::Item*>* outItems)
{
    if (!vm || nBytes == 0 || maxItems <= 0 || !handle)
        return;

    MemReader mr;
    mr.vm = vm;
    mr.read(begAddr, nBytes);

    cs_insn* insn = cs_malloc(handle);
    if (!insn)
        return;

    const uint8_t* code     = mr.buf.data();
    size_t         codeSize = mr.buf.size();
    uint64_t       address  = begAddr;

    while (codeSize > 0 && (int)outItems->size() < maxItems)
    {
        bool ok = cs_disasm_iter(handle, &code, &codeSize, &address, insn);
        if (ok)
        {
            auto pItem = qtd::make_unique<CodeItem>();
            pItem->m_addr       = (AddrRef)insn->address;
            pItem->m_bytesCount = insn->size;
            pItem->m_text       = insn->mnemonic;
            // Pad mnemonic to fixed width for column alignment
            while (pItem->m_text.size() < 8)
                pItem->m_text += ' ';
            pItem->m_text += insn->op_str;

            // Bytes string — clamped to array bounds
            pItem->m_bytesString.clear();
            uint16_t nB = (insn->size <= sizeof(insn->bytes)) ? insn->size : (uint16_t)sizeof(insn->bytes);
            for (uint16_t b = 0; b < nB; ++b)
            {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X", insn->bytes[b]);
                pItem->m_bytesString += hex;
            }

            outItems->push_back(pItem.get());
            outStorage->push_back(qtd::move(pItem));
        }
        else
        {
            // Undecodable: advance by 2 bytes (one m68k word) and retry.
            // Without SKIPDATA, cs_disasm_iter leaves code/size unchanged
            // on failure, so we must advance manually.
            if (codeSize < 2)
                break;
            code     += 2;
            codeSize -= 2;
            address  += 2;
        }
    }

    cs_free(insn, 1);
}


// Check whether a forward decode from `start` produces an instruction at
// `target`.  Used by the backward-walk anchor finder.
static bool decode_hits_target(csh handle, IVm::VM* vm, AddrRef start, AddrRef target, AddrRef end)
{
    if (!handle || !vm || start >= end)
        return false;

    MemReader mr;
    mr.vm = vm;
    uint32_t nBytes = (uint32_t)(end - start) & ~1u;
    mr.read(start, nBytes);

    cs_insn* insn = cs_malloc(handle);
    if (!insn)
        return false;

    const uint8_t* code     = mr.buf.data();
    size_t         codeSize = mr.buf.size();
    uint64_t       address  = start;
    bool           found    = false;

    while (codeSize > 0)
    {
        bool ok = cs_disasm_iter(handle, &code, &codeSize, &address, insn);
        if (ok)
        {
            if ((AddrRef)insn->address == target)
            {
                found = true;
                break;
            }
        }
        else
        {
            if (codeSize < 2)
                break;
            code     += 2;
            codeSize -= 2;
            address  += 2;
        }
    }

    cs_free(insn, 1);
    return found;
}


void M68CodeDisassembler::requestM68DisasmLines(
    IVm::VM* vm, AddrRef startAddr, int nLines,
    qtd::vector<amD::cda::Item*>* outItems,
    const AddrRef* pProvedInstructionStart)
{
    outItems->clear();
    m_curItems.clear();

    if (!vm || nLines <= 0 || !m_pCapstone || !*m_pCapstone)
        return;

    csh handle = *m_pCapstone;

    // Find a real instruction boundary to disassemble forward from.
    AddrRef disasmStart = startAddr;
    bool anchored = false;

    if (pProvedInstructionStart && *pProvedInstructionStart)
    {
        AddrRef anchor = *pProvedInstructionStart;
        if (anchor <= startAddr && (startAddr - anchor) <= g_maxAnchorBackGap)
        {
            disasmStart = anchor;
            anchored = true;
        }
        else if (anchor > startAddr && (anchor - startAddr) <= g_maxAnchorBackGap)
        {
            // Walk backward from the anchor to find a boundary at/below startAddr.
            AddrRef goodAddr = anchor;
            int iterations = 0;
            while (goodAddr > startAddr && iterations < 64)
            {
                bool found = false;
                for (uint32_t back = 2; back <= g_maxOpSize && back <= goodAddr; back += 2)
                {
                    AddrRef tryStart = goodAddr - back;
                    if (decode_hits_target(handle, vm, tryStart, goodAddr, goodAddr + 2))
                    {
                        goodAddr = tryStart;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    break;
                ++iterations;
            }
            if (goodAddr <= startAddr)
            {
                disasmStart = goodAddr;
                anchored = true;
            }
        }
    }

    // Local probe if no anchor worked.
    if (!anchored)
    {
        for (uint32_t back = 2; back <= g_maxOpSize && back <= startAddr; back += 2)
        {
            AddrRef tryStart = startAddr - back;
            if (decode_hits_target(handle, vm, tryStart, startAddr, startAddr + g_maxOpSize * 2))
            {
                disasmStart = tryStart;
                break;
            }
        }
    }

    // Fallback: anchor itself.
    if (disasmStart > startAddr && pProvedInstructionStart && *pProvedInstructionStart)
    {
        AddrRef anchor = *pProvedInstructionStart;
        if (anchor >= startAddr)
            disasmStart = anchor;
    }

    // Decode the range [disasmStart, disasmStart + neededBytes).
    uint32_t prefixBytes = (startAddr > disasmStart) ? (uint32_t)(startAddr - disasmStart) : 0;
    uint32_t neededBytes = prefixBytes + (uint32_t)nLines * 6u;
    constexpr uint32_t maxNeededBytes = 64u * 1024u;

    for (int attempt = 0; attempt < 6; ++attempt)
    {
        m_curItems.clear();
        outItems->clear();

        // Over-decode so we have enough items after filtering prefix.
        int maxDecode = nLines + (int)(prefixBytes / 2) + 4;
        decode_range(handle, vm, disasmStart, neededBytes, maxDecode, &m_curItems, outItems);

        // Filter out prefix instructions (address < startAddr).
        qtd::vector<cda::Item*> filtered;
        for (cda::Item* pItem : *outItems)
        {
            if (pItem->m_addr >= startAddr)
                filtered.push_back(pItem);
            if ((int)filtered.size() >= nLines)
                break;
        }
        *outItems = qtd::move(filtered);

        if ((int)outItems->size() >= nLines || neededBytes >= maxNeededBytes)
            break;

        neededBytes *= 2;
    }
}


}; // namespace amD::cda
