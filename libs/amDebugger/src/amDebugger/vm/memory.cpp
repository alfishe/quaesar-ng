#include "memory.h"


const char* IVm::EMemSrc::to_string(EMemSrc value)
{
    switch (value)
    {
    case EMemSrc::NONE:
        return "NONE";
    case EMemSrc::CHIP:
        return "CHIP";
    case EMemSrc::CHIP_MIRROR:
        return "CHIP_MIRROR";
    case EMemSrc::SLOW:
        return "SLOW";
    case EMemSrc::SLOW_MIRROR:
        return "SLOW_MIRROR";
    case EMemSrc::FAST:
        return "FAST";
    case EMemSrc::CIA:
        return "CIA";
    case EMemSrc::CIA_MIRROR:
        return "CIA_MIRROR";
    case EMemSrc::RTC:
        return "RTC";
    case EMemSrc::CUSTOM:
        return "CUSTOM";
    case EMemSrc::CUSTOM_MIRROR:
        return "CUSTOM_MIRROR";
    case EMemSrc::AUTOCONF:
        return "AUTOCONF";
    case EMemSrc::ZOR:
        return "ZOR";
    case EMemSrc::ROM:
        return "ROM";
    case EMemSrc::ROM_MIRROR:
        return "ROM_MIRROR";
    case EMemSrc::WOM:
        return "WOM";
    case EMemSrc::EXT:
        return "EXT";
    default:
        return "???";
    }
}


const char* IVm::EMemSrc::to_desc(EMemSrc value)
{
    switch (value)
    {
    case EMemSrc::NONE:
        return "Unmapped";
    case EMemSrc::CHIP:
        return "Chip RAM";
    case EMemSrc::CHIP_MIRROR:
        return "Chip RAM mirror";
    case EMemSrc::SLOW:
        return "Slow RAM";
    case EMemSrc::SLOW_MIRROR:
        return "Slow RAM mirror";
    case EMemSrc::FAST:
        return "Fast RAM";
    case EMemSrc::CIA:
        return "CIA";
    case EMemSrc::CIA_MIRROR:
        return "CIA mirror";
    case EMemSrc::RTC:
        return "Real-time clock";
    case EMemSrc::CUSTOM:
        return "Custom chips";
    case EMemSrc::CUSTOM_MIRROR:
        return "Custom chips mirror";
    case EMemSrc::AUTOCONF:
        return "Auto config";
    case EMemSrc::ZOR:
        return "Zorro boards";
    case EMemSrc::ROM:
        return "Kickstart ROM";
    case EMemSrc::ROM_MIRROR:
        return "Kickstart ROM mirror";
    case EMemSrc::WOM:
        return "Write-only memory";
    case EMemSrc::EXT:
        return "Extension ROM";
    default:
        return "???";
    }
}
