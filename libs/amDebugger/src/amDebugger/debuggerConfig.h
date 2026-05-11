#pragma once


struct DbgConfig {
    QD_SINGLETON_DECLARE(DbgConfig);

    bool showVHPopsLines = false;

};
inline static DbgConfig* g_dbg_cfg = &DbgConfig::get();
