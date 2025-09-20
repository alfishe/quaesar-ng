#pragma once
#include <amDebugger/config.h>


struct CfgQsrMain : public CfgBase {
    CFG_DECLARE(CfgQsrMain);
    bool quitByEsc = true;
};
inline static CfgQsrMain* g_cfg_main = CfgQsrMain::get();


//------------------------------------------------------------------------
struct CfgQsrStartup : public CfgBase {
    CFG_DECLARE(CfgQsrStartup);
    qd::string kickstart;
};
inline static CfgQsrStartup* g_cfg_startup = CfgQsrStartup::get();
