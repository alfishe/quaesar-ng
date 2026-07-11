#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "amDebugger/vm/vmInterface.h"
#include "ks_offsets.h"
#include "signature_manager.h"

namespace amD::os {

struct KickstartInfo {
    uint32_t execBase = 0;
    uint32_t romBase = 0;
    uint16_t version = 0;
    uint16_t revision = 0;
    std::string idString;
};

struct RomTag {
    uint32_t address;
    uint16_t matchWord;
    uint8_t flags;
    uint8_t version;
    uint8_t type;
    int8_t priority;
    std::string name;
    std::string idString;
    uint32_t size;
    uint32_t initFunc;
};

struct LvoEntry {
    int16_t offset;
    std::string funcName;
    uint32_t targetAddress;
    bool isJump;
};

enum class NodeType : uint8_t {
    TASK = 1,
    INTERRUPT = 2,
    DEVICE = 3,
    MSGPORT = 4,
    MESSAGE = 5,
    RESOURCE = 8,
    LIBRARY = 9,
    MEMORY = 10,
    PROCESS = 13,
    SEMAPHORE = 14,
    UNKNOWN = 0xFF
};

struct LibraryInfo {
    uint32_t baseAddress;
    uint16_t negSize;
    uint16_t posSize;
    uint16_t version;
    uint16_t revision;
    std::string name;
    std::string idString;
    uint16_t openCount;
    NodeType type;
    std::vector<LvoEntry> lvoEntries;
    std::string integrityStatus;
};

class OsIntrospector {
public:
    explicit OsIntrospector(IVm::VM* vm);
    ~OsIntrospector() = default;

    // Part 1: OS Modules
    bool isOsBooted();
    KickstartInfo getKickstartInfo() const { return m_cachedKs; }
    std::vector<RomTag> scanRomTags();
    std::vector<LibraryInfo> scanLibraries();

    SignatureManager& getSignatureManager() { return m_sigMgr; }

    // Helpers
    uint32_t readU32(uint32_t addr);
    uint16_t readU16(uint32_t addr);
    uint8_t readU8(uint32_t addr);
    std::string readCString(uint32_t addr, size_t maxLen = 256);

private:
    IVm::VM* m_vm;
    SignatureManager m_sigMgr;
    const KsOffsets* m_offsets = nullptr;
    KickstartInfo m_cachedKs;
    bool m_validated = false;
};

} // namespace amD::os
