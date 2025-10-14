#pragma once
#include "qd/stl/forwardDecl.h"


struct DbgConfig {
    SINGLETON_DECLARATION(DbgConfig);

    bool showVHPopsLines = false;

};
inline static DbgConfig* g_dbg_cfg = &DbgConfig::get();
