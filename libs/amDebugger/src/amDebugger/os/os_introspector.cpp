#include "os_introspector.h"
#include "fd_tables.h"
#include <algorithm>

namespace amD::os {

OsIntrospector::OsIntrospector(IVm::VM* vm) : m_vm(vm) {
}

uint32_t OsIntrospector::readU32(uint32_t addr) {
    if (!m_vm || !m_vm->mem) return 0;
    return m_vm->mem->getU32(addr);
}

uint16_t OsIntrospector::readU16(uint32_t addr) {
    if (!m_vm || !m_vm->mem) return 0;
    return m_vm->mem->getU16(addr);
}

uint8_t OsIntrospector::readU8(uint32_t addr) {
    if (!m_vm || !m_vm->mem) return 0;
    return m_vm->mem->getU8(addr);
}

std::string OsIntrospector::readCString(uint32_t addr, size_t maxLen) {
    if (!m_vm || !m_vm->mem || addr == 0) return "";
    std::string result;
    result.reserve(32);
    for (size_t i = 0; i < maxLen; ++i) {
        // Checking bounds could be done by getRealAddr but here we rely on getU8 returning 0 or safe data.
        uint8_t c = m_vm->mem->getU8(addr + i);
        if (c == 0) break;
        result.push_back((char)c);
    }
    return result;
}

bool OsIntrospector::isOsBooted() {
    if (!m_vm || !m_vm->mem) return false;

    // Fast path: if already validated and execbase hasn't changed.
    uint32_t currentExecBase = readU32(0x00000004);
    if (m_validated && currentExecBase == m_cachedKs.execBase) {
        return true;
    }

    m_validated = false;
    m_offsets = nullptr;

    if (currentExecBase < 0x00000400 || currentExecBase > 0x0FFFFFFF) {
        // Plausible RAM region check (very basic, covers all possible Amiga RAM spaces).
        return false;
    }

    // Signature Check: ln_Type == NT_LIBRARY (3)
    // Offset of ln_Type in Node is 8.
    uint8_t lnType = readU8(currentExecBase + 8);
    if (lnType != (uint8_t)NodeType::LIBRARY) {
        return false;
    }

    // Strong Signature Check: ln_Name == "exec.library"
    // Offset of ln_Name in Node is 10.
    uint32_t namePtr = readU32(currentExecBase + 10);
    std::string name = readCString(namePtr, 64);
    if (name != "exec.library") {
        return false;
    }

    // Passed checks.
    m_cachedKs.execBase = currentExecBase;
    m_cachedKs.romBase = 0x00F80000; // Standard Kickstart base
    
    // Read version from ExecBase (offset 20 is lib_Version, 22 is lib_Revision, 24 is lib_IdString)
    m_cachedKs.version = readU16(currentExecBase + 20);
    m_cachedKs.revision = readU16(currentExecBase + 22);
    uint32_t idPtr = readU32(currentExecBase + 24);
    m_cachedKs.idString = readCString(idPtr, 128);

    m_offsets = getKsOffsets(m_cachedKs.version);
    m_validated = m_offsets != nullptr;

    return m_validated;
}

std::vector<RomTag> OsIntrospector::scanRomTags() {
    std::vector<RomTag> tags;
    if (!isOsBooted()) return tags;

    // Scan the entire 1MB ROM region ($00F00000 - $01000000). 
    // This covers standard 256KB/512KB Kickstarts and UAE extended ROMs.
    uint32_t addr = 0x00F00000;
    uint32_t endAddr = 0x01000000; 
    
    while (addr < endAddr) {
        uint16_t matchWord = readU16(addr);
        if (matchWord == 0x4AFC) {
            uint32_t matchTag = readU32(addr + 2);
            if (matchTag == addr) {
                // Valid RomTag
                RomTag tag;
                tag.address = addr;
                tag.matchWord = matchWord;
                uint32_t endSkip = readU32(addr + 6);
                tag.size = endSkip > addr ? (endSkip - addr) : 0;
                tag.flags = readU8(addr + 10);
                tag.version = readU8(addr + 11);
                tag.type = readU8(addr + 12);
                tag.priority = (int8_t)readU8(addr + 13);
                
                uint32_t namePtr = readU32(addr + 14);
                tag.name = readCString(namePtr, 64);
                
                uint32_t idPtr = readU32(addr + 18);
                tag.idString = readCString(idPtr, 128);
                
                tag.initFunc = readU32(addr + 22);
                tags.push_back(tag);
                
                if (endSkip > addr && endSkip < endAddr) {
                    addr = endSkip;
                    continue;
                }
            }
        }
        addr += 2;
    }

    std::sort(tags.begin(), tags.end(), [](const RomTag& a, const RomTag& b) {
        return a.priority > b.priority;
    });

    return tags;
}

std::vector<LibraryInfo> OsIntrospector::scanLibraries() {
    std::vector<LibraryInfo> libs;
    if (!isOsBooted() || !m_offsets) return libs;

    const int MAX_NODES = 1000;
    int count = 0;

    uint32_t headPtr = m_cachedKs.execBase + m_offsets->libListOffset;
    uint32_t currNode = readU32(headPtr); // lh_Head

    while (currNode != 0 && count < MAX_NODES) {
        uint32_t nextNode = readU32(currNode); // ln_Succ
        if (nextNode == 0) break;

        LibraryInfo lib;
        lib.baseAddress = currNode;
        lib.type = (NodeType)readU8(currNode + 8); // ln_Type
        
        uint32_t namePtr = readU32(currNode + 10);
        lib.name = readCString(namePtr, 64);
        
        // padding 15
        lib.negSize = readU16(currNode + 16);
        lib.posSize = readU16(currNode + 18);
        lib.version = readU16(currNode + 20);
        lib.revision = readU16(currNode + 22);
        
        uint32_t idPtr = readU32(currNode + 24);
        lib.idString = readCString(idPtr, 128);
        
        lib.openCount = readU16(currNode + 32);

        // Fetch LVOs
        if (lib.negSize > 0) {
            for (int offset = 6; offset <= lib.negSize; offset += 6) {
                uint32_t lvoAddr = currNode - offset;
                uint16_t opcode = readU16(lvoAddr);
                if (opcode == 0x4EF9) { // JMP abs.L
                    LvoEntry lvo;
                    lvo.offset = -offset;
                    lvo.isJump = true;
                    lvo.targetAddress = readU32(lvoAddr + 2);
                    lvo.funcName = getLvoName(lib.name, offset);
                    lib.lvoEntries.push_back(lvo);
                }
            }
        }

        // Signature Analysis
        if (m_vm->mem) {
            uint8_t* hostPtr = m_vm->mem->getRealAddr(lib.baseAddress - lib.negSize);
            if (hostPtr) {
                bool isStandard = m_sigMgr.verifyIntegrity(lib.name, hostPtr, (size_t)lib.negSize + lib.posSize);
                lib.integrityStatus = isStandard ? "Verified Standard" : "Custom / Modified";
            } else {
                lib.integrityStatus = "Unknown (Mem Read Failed)";
            }
        }

        libs.push_back(lib);

        currNode = nextNode;
        count++;
    }

    return libs;
}

} // namespace amD::os
