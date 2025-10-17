#pragma once
#include <amDebugger/config.h>
#include <qd/stl/vector.h>
#include "string"
#include "vector"


//------------------------------------------------------------------------
struct CfgQsrStartup : public CfgBase {
    CFG_DECLARE(CfgQsrStartup);
    // kickstart file such as kick.rom, kick31.rom, etc.
    std::string kickRomPath;

    // input file such as .adf, .dms, executable, etc.
    std::string input;
    // serial port path ('/tmp/virtual-serial-port')
    std::string serialPort;

    std::vector<std::string> uaeExtArgs;
};
inline static CfgQsrStartup& g_cfg_startup = CfgQsrStartup::get();
