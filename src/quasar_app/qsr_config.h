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

    // Sound engine selection: "native" keeps the emulator core's default
    // audio path; "pwm" enables the PWM-style engine (CIC/boxcar resampling
    // in the core + punch/room post-processing).
    std::string soundEngine = "native";
    // Punch transient enhancement (PWM engine only)
    bool soundPunch = true;
    // Room simulation for headphone listening (PWM engine only):
    // "off", "-15db", "-14db", "-13db", "-12db", "-9db"
    std::string soundRoom = "off";

    bool isPwmSoundEngine() const {
        return soundEngine == "pwm" || soundEngine == "PWM";
    }
};
inline static CfgQsrStartup& g_cfg_startup = CfgQsrStartup::get();
