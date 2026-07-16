#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// A simplified configuration struct to bridge UAE settings to vAmiga
struct VAmigaExtConfig {
    int cpu_model;
    const char* hd_paths[4];
    int hd_types[4];       // 0 = HDF, 1 = DIR
    const char* hd_volnames[4];
    int num_hds;
    int chip_ram_kb;       // Chip RAM size in KB (0 = use vAmiga default)
    int slow_ram_kb;       // Slow (bogo) RAM size in KB
    int fast_ram_kb;       // Fast RAM size in KB
};

// This function is implemented in quaesar (e.g. uae_server_thread.cpp)
// and called by vAmiga thread to retrieve the configuration parsed by WinUAE.
void qsr_bridge_get_vamiga_config(struct VAmigaExtConfig* out_config);

#ifdef __cplusplus
}
#endif
