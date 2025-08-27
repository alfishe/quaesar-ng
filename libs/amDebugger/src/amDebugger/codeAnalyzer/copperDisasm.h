#pragma once
#include "qd/base/base.h"
#include "qd/stl/string.h"


namespace amD {


struct DecodedCopperList {
    struct CopInst {
        AddrRef addr = 0;
        uint16_t w1 = -1;
        uint16_t w2 = -1;
    };

    struct Entry : public CopInst {
        int vpos = -1;
        int hpos = -1;
        eastl::fixed_string<char, 16, false> strInsn;
        eastl::fixed_string<char, 128, false> comment;
    };
    qd::vector<DecodedCopperList::Entry> decoded;

public:
    void decodeInstr(Entry& ent)
    {
        uint32_t insn = ent.w1 << 16 | ent.w2;
        uint32_t insn_type;
        insn_type = insn & 0x00010001;

        ent.comment.clear();
        switch (insn_type)
        {
        case 0x00010000: /* WAIT insn */
            ent.strInsn = "WAIT";
            disassembleWait(ent, insn);
            if (insn == 0xfffffffe)
                ent.comment = "End of Copperlist";
            break;

        case 0x00010001: /* SKIP insn */
            ent.strInsn = "SKIP";
            disassembleWait(ent, insn);
            break;

        case 0x00000000:
        case 0x00000001: /* MOVE insn */
        {
            ent.strInsn = "MOVE";
            AddrRef addr = ((insn >> 16) & 0x1fe) + 0xdff000;
            CustReg crg = CustReg::getRegByAddr(addr);
            if (crg.isValid())
                ent.comment.sprintf("0x%04x -> %s", insn & 0xffff, crg.toStringC());
            else
                ent.comment.sprintf("%04x := 0x%04x", addr, insn & 0xffff);
        }
        break;

        default:
            ent.comment = ("bad copper command");
            break;
        }
    }


    void disassembleWait(Entry& out, uint32_t insn)
    {
        int vp, hp, ve, he, bfd, v_mask, h_mask;
        int doout = 0;

        vp = (insn & 0xff000000) >> 24;
        hp = (insn & 0x00fe0000) >> 16;
        ve = (insn & 0x00007f00) >> 8;
        he = (insn & 0x000000fe);
        bfd = (insn & 0x00008000) >> 15;

        /* bit15 can never be masked out*/
        v_mask = vp & (ve | 0x80);
        h_mask = hp & he;
        if (v_mask > 0)
        {
            doout = 1;
            out.comment.append("vpos ");
            if (ve != 0x7f)
            {
                out.comment.append_sprintf("& 0x%02x ", ve);
            }
            out.comment.append_sprintf(">= 0x%02x", v_mask);
        }
        if (he > 0)
        {
            if (v_mask > 0)
            {
                out.comment.append(" and");
            }
            out.comment.append(" hpos ");
            if (he != 0xfe)
            {
                out.comment.append_sprintf("& 0x%02x ", he);
            }
            out.comment.append_sprintf(">= 0x%02x", h_mask);
        }
        else
        {
            if (doout)
                out.comment.append(", ");
            out.comment.append(", ignore horizontal");
        }

        out.comment.append_sprintf(", VP %02x, VE %02x; HP %02x, HE %02x; BFD %d", vp, ve, hp, he, bfd);
    }


    void decodeLines(IVm::VM* vm, AddrRef startAddr, int num_lines)
    {
        decoded.clear();
        AddrRef addr = startAddr;
        decoded.reserve(num_lines);
        for (int i = 0; i < num_lines; ++i)
        {
            Entry& curEnt = decoded.push_back();
            curEnt.addr = addr;
            if (vm->mem->getU16(addr, &curEnt.w1) && vm->mem->getU16(addr + 2, &curEnt.w2))
            {
                decodeInstr(curEnt);
            }
            else
            {
                break;
            }
            addr += 4;
        }
    }
}; // struct DecodedCopperList


}; // namespace amD
