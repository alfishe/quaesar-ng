#pragma once
#include <stdint.h>

namespace amD::os {

struct KsOffsets {
    uint16_t version;
    uint16_t taskReadyOffset;
    uint16_t taskWaitOffset;
    uint16_t thisTaskOffset;
    uint16_t libListOffset;
    uint16_t devListOffset;
    uint16_t resourceListOffset;
    uint16_t portListOffset;
    uint16_t memListOffset;
    uint16_t semSegListOffset;
};

inline const KsOffsets* getKsOffsets(uint16_t /*ksVersion*/) {
    // In AmigaOS, these list offsets in ExecBase are statically located
    // and identical across all Kickstart versions (1.2 to 3.2+).
    // These values have been verified against the official Commodore
    // Amiga NDK headers (specifically exec/execbase.i and exec/execbase.h).
    static const KsOffsets defaultOffsets = {
        0,  // version - filled in at runtime if needed
        0x0196,  // taskReadyOffset (406)
        0x01A4,  // taskWaitOffset (420)
        0x0114,  // thisTaskOffset (276)
        0x017A,  // libListOffset (378)
        0x015E,  // devListOffset (350)
        0x0150,  // resourceListOffset (336)
        0x0188,  // portListOffset (392)
        0x0142,  // memListOffset (322)
        0x0214   // semSegListOffset (532)
    };
    
    return &defaultOffsets;
}

} // namespace amD::os
