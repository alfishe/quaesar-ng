#include "cdaServer.h"
#include <capstone/capstone.h>
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
        printf("Failed on cs_open() with error returned: %u\n", err);
        abort();
    }
    cs_option(*m_pCapstone, CS_OPT_DETAIL, CS_OPT_ON);
    cs_option(*m_pCapstone, CS_OPT_SKIPDATA, CS_OPT_ON);
}


struct CapstoneDisassemblerContext {
    csh* m_pCapstone = nullptr;
    cs_insn* m_pInstructions = nullptr;
    size_t m_nInstructions = 0;
    qtd::vector<uint8_t> m_buf;

public:
    CapstoneDisassemblerContext(csh* pCapstone)
        : m_pCapstone(pCapstone)
    {
    }

    void reset()
    {
        if (m_pInstructions)
            cs_free(m_pInstructions, m_nInstructions);
        m_pInstructions = nullptr;
        m_nInstructions = 0;
    }

    // Disassembles [begAddr, endAddr) as ONE linear run, so every produced
    // instruction address is exact relative to begAddr and monotonically
    // increasing - there's no per-chunk restart to lose sync at.
    //
    // Reads memory word-by-word through IVm::Memory::getU16() (UAE's own
    // memory_get_word(), which does the full bank/mirror address decode)
    // rather than grabbing a raw host pointer via getRealAddr() and handing
    // capstone a fixed-size window into it - ROM (and other regions) can be
    // mirrored at multiple address ranges, and a single named "bank" with a
    // fixed start/size cannot represent that, which made real, currently-
    // executing PC addresses look unmapped.
    void disasm(IVm::VM* vm, AddrRef begAddr, AddrRef endAddr)
    {
        reset();
        if (endAddr <= begAddr)
            return;
        uint32_t countBytes = (endAddr - begAddr) & ~1u; // word-aligned
        if (!countBytes)
            return;

        m_buf.resize(countBytes);
        for (uint32_t i = 0; i < countBytes; i += 2)
        {
            uint16_t w = vm->mem->getU16(begAddr + i);
            m_buf[i] = (uint8_t)(w >> 8);
            m_buf[i + 1] = (uint8_t)(w & 0xFF);
        }

        m_nInstructions =
            cs_disasm(*m_pCapstone, m_buf.data(), countBytes, begAddr, countBytes / 2, &m_pInstructions);
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


static qtd::unique_ptr<CodeItem> make_code_item(const cs_insn& insn)
{
    auto pItem = qtd::make_unique<CodeItem>();
    pItem->m_addr = (AddrRef)insn.address;
    pItem->m_bytesCount = insn.size;
    pItem->m_text = insn.mnemonic;
    do
    {
        pItem->m_text += ' ';
    } while (pItem->m_text.size() < 8);
    pItem->m_text += insn.op_str;

    pItem->m_bytesString.clear();
    for (uint16_t b = 0; b < insn.size; ++b)
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", insn.bytes[b]);
        pItem->m_bytesString += buf;
    }
    return pItem;
}


void M68CodeDisassembler::requestM68DisasmLines(IVm::VM* vm, AddrRef startAddr, int nLines,
    qtd::vector<amD::cda::Item*>* outItems, const AddrRef* pProvedInstructionStart)
{
    outItems->clear();
    m_curItems.clear();

    if (nLines <= 0)
        return;

    // Find a real instruction boundary to disassemble forward from, so
    // everything we produce is exact rather than guessed:
    //  1) if the anchor (typically the current PC) is at or behind our view
    //     start, it's already a validated boundary - use it directly;
    //  2) if the anchor is ahead of/inside our view (the common "recenter on
    //     PC" case - the view starts before PC to put it near the middle),
    //     walk backward FROM the anchor, one validated resync at a time,
    //     until we reach a boundary at or before startAddr. This is the
    //     correct way to reconstruct instruction boundaries going backward -
    //     probing near startAddr itself is not, since startAddr is only a
    //     heuristic guess (e.g. "PC minus N*8 bytes") and not itself known to
    //     be a real instruction start;
    //  3) otherwise (no usable anchor at all) probe a few bytes behind
    //     startAddr, looking for a decode that lands exactly on startAddr;
    //  4) otherwise just start at startAddr as a last resort (e.g. data area).
    AddrRef disasmStart = startAddr;
    bool anchored = false;

    if (pProvedInstructionStart && *pProvedInstructionStart)
    {
        AddrRef anchor = *pProvedInstructionStart;
        if (anchor <= startAddr)
        {
            if ((startAddr - anchor) <= g_maxAnchorBackGap)
            {
                disasmStart = anchor;
                anchored = true;
            }
        }
        else if ((anchor - startAddr) <= g_maxAnchorBackGap)
        {
            // Walk backward from the anchor, one validated resync at a time,
            // until we reach (or pass below) startAddr.
            AddrRef goodAddr = anchor;
            int iterations = 0;
            while (goodAddr > startAddr && iterations < 64)
            {
                bool found = false;
                for (uint32_t back = 2; back <= g_maxOpSize && back <= goodAddr; back += 2)
                {
                    AddrRef tryStart = goodAddr - back;
                    CapstoneDisassemblerContext probe(m_pCapstone);
                    probe.disasm(vm, tryStart, goodAddr + 2);
                    if (probe.hasM68InstructionAddr(goodAddr))
                    {
                        goodAddr = tryStart;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    break; // can't reliably walk back further
                ++iterations;
            }
            // Only trust this if the walk actually reached down to startAddr -
            // otherwise disasmStart would end up > startAddr, which underflows
            // the unsigned byte-count math below into a huge garbage value.
            if (goodAddr <= startAddr)
            {
                disasmStart = goodAddr;
                anchored = true;
            }
        }
    }

    if (!anchored)
    {
        for (uint32_t back = 1; back <= g_maxOpSize && back <= startAddr; ++back)
        {
            AddrRef tryStart = startAddr - back;
            CapstoneDisassemblerContext probe(m_pCapstone);
            probe.disasm(vm, tryStart, tryStart + g_maxOpSize * 2);
            if (probe.hasM68InstructionAddr(startAddr))
            {
                disasmStart = tryStart;
                break;
            }
        }
    }

    // Defensive: disasmStart must never end up past startAddr - guard against
    // it regardless, since the byte-count math below is unsigned and would
    // otherwise underflow into a huge garbage value.
    if (disasmStart > startAddr)
        disasmStart = startAddr;

    // Generous estimate of bytes needed to produce nLines instructions past
    // startAddr, widened below if capstone decodes fewer viable ones than needed.
    uint32_t neededBytes = (startAddr - disasmStart) + (uint32_t)nLines * 6u;
    constexpr uint32_t maxNeededBytes = 64u * 1024u; // sane upper bound on a single disasm window

    for (int attempt = 0; attempt < 6; ++attempt)
    {
        AddrRef disasmEnd = disasmStart + neededBytes;

        CapstoneDisassemblerContext cpd(m_pCapstone);
        cpd.disasm(vm, disasmStart, disasmEnd);

        m_curItems.clear();
        outItems->clear();
        for (size_t i = 0; i < cpd.m_nInstructions; ++i)
        {
            const cs_insn& curInsn = cpd.m_pInstructions[i];
            if ((AddrRef)curInsn.address < startAddr)
                continue;
            qtd::unique_ptr<CodeItem> pItem = make_code_item(curInsn);
            outItems->push_back(pItem.get());
            m_curItems.push_back(qtd::move(pItem));
            if ((int)outItems->size() >= nLines)
                break;
        }

        if ((int)outItems->size() >= nLines || neededBytes >= maxNeededBytes)
            break;

        neededBytes *= 2; // didn't get enough lines - widen the window and retry
    }

    // TEMPORARY DIAGNOSTIC - remove once the empty-widget issue is confirmed fixed.
    {
        static AddrRef s_lastLoggedStart = 0xFFFFFFFF;
        static size_t s_lastLoggedCount = (size_t)-1;
        if (disasmStart != s_lastLoggedStart || outItems->size() != s_lastLoggedCount)
        {
            s_lastLoggedStart = disasmStart;
            s_lastLoggedCount = outItems->size();
            qd::logInfo("cda-diag: startAddr=%08X disasmStart=%08X anchored=%d wanted=%d got=%d",
                (uint32_t)startAddr, (uint32_t)disasmStart, (int)anchored, nLines, (int)outItems->size());
        }
    }
}


void M68CodeDisassembler::destroy()
{
    m_curItems.clear();
    SAFE_DELETE(m_pCapstone);
}


}; // namespace amD::cda
