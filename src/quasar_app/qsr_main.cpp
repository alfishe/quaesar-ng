#include <SDL.h>
#include <nfd.h>
#include <qd/app/appPartsMgr.h>
#include <qd/app/application.h>
#include <qd/thread/thread.h>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include "amDebugger/debuggerWndApp.h"
#include "cli11/CLI11.hpp"
#include "crashhandler/crashhandler.h"
#include "qsr_application.h"
#include "qsr_config.h"
#include "qsr_main_wnd_client_app.h"  // g_cfg_vm_wnd (CfgQsrMain)
#include "qsr_operations.h"           // isSnapshotFile
#include "quaesar.h"
#include "vm_player_selector.h"  // VmPlayersSelector::isKnownCoreId


#ifdef WIN32
#define SDL_MAIN_NEEDED
#include <SDL_main.h>
#else
// WTF SDL2MAIN_LIBRARY - I can't to build it on MacOs
#define SDL_main main
#endif  // WIN32


// Quaesar main
int SDL_main(int argc, char* argv[]) {
    // Install crash handler as early as possible
    auto crashHandler = std::unique_ptr<CrashHandler>(CrashHandler::create());
    crashHandler->install();

    // read options from CLI

    CLI::App cliApp{"Quaesar"};
    cliApp.allow_extras();
    cliApp.add_option("input", g_cfg_startup.input,
                      "Executable or image file (adf, dms)");  // ->check(CLI::ExistingFile);
    cliApp.add_option("-k,--kickstart", g_cfg_startup.kickRomPath,
                      "Path to the kickstart ROM");  // ->check(CLI::ExistingFile);
    cliApp.add_option("--serial_port", g_cfg_startup.serialPort, "Serial port path");
    cliApp.add_option("-s", g_cfg_startup.uaeExtArgs,
                      "key followed by the original WinUAE commands. Example:\n"
                      "   quaesar.exe -k c:\\Amiga\\KICK13.rom -s filesystem=rw,dh0:c:\\Amiga\\hd0");

    std::string engineId;
    cliApp.add_option("--engine", engineId, "Emulation engine to use (uae, vamiga, ...)");

    cliApp.add_option("--sound-engine", g_cfg_startup.soundEngine,
                      "Sound engine: 'native' (default, core's own path) or 'pwm' "
                      "(CIC/boxcar resampling + punch enhancement)");
    cliApp.add_flag("--sound-punch,!--no-sound-punch", g_cfg_startup.soundPunch,
                    "Enable/disable punch transient enhancement (pwm sound engine only, default on)");
    cliApp.add_option("--sound-room", g_cfg_startup.soundRoom,
                      "Room simulation for headphones (pwm sound engine only): "
                      "off, -15db, -14db, -13db, -12db, -9db");
    try {
        cliApp.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return cliApp.exit(e);
    }

    // Detect snapshot files in the input positional argument.
    // If the input is a .uss or .vasnap file (by magic bytes or extension),
    // move it to snapshotPath so it's loaded after emulator init instead
    // of being treated as a disk image.
    if (!g_cfg_startup.input.empty()) {
        if (qsr::operations::isSnapshotFile(g_cfg_startup.input)) {
            g_cfg_startup.snapshotPath = g_cfg_startup.input;
            g_cfg_startup.input.clear();
            printf("Startup: detected snapshot file, will load after init\n");
        }
    }

    // Apply --engine selection with validation.
    // Default engine is already WinUae (set in CfgQsrMain); only override
    // if the user passed --engine and the id maps to a known engine.
    // Lookup is case-insensitive ("vAmiga", "VAMIGA", "vamiga" all work).
    if (!engineId.empty()) {
        qsr::EngineId engine = qsr::engineIdFromStr(engineId.c_str());
        if (engine == qsr::EngineId::Unknown) {
            SDL_Log("Unknown engine '%s', falling back to 'uae'", engineId.c_str());
            qsr::g_cfg_vm_wnd.engine = qsr::EngineId::WinUae;
        } else {
            qsr::g_cfg_vm_wnd.engine = engine;
        }
    }

    // Validate kickstart ROM exists before starting any subsystem.
    // Without a valid ROM both UAE and vAmiga engines are non-functional,
    // so there's no point proceeding to SDL init, window creation, etc.
    if (!g_cfg_startup.kickRomPath.empty()) {
        if (FILE* f = fopen(g_cfg_startup.kickRomPath.c_str(), "rb")) {
            fclose(f);
        } else {
            fprintf(stderr, "ERROR: Kickstart ROM not found: '%s'\n", g_cfg_startup.kickRomPath.c_str());
            return 1;
        }
    }

    // initialize SDL

    // Suppress SDL's built-in NSLog output — write_log() routes through
    // SDL_LogMessageV which triggers NSLog on macOS. NSLog is extremely
    // expensive (CoreFoundation + mutex + kdebug_trace syscall) and the
    // emulator generates thousands of log lines per second during boot,
    // burning a full CPU core. The qd::logConsole() path in write_log()
    // still works for internal debug output.
    // SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    atexit(&SDL_Quit);

    // create Quaesar app
    {
        g_pApp = new qsr::QuaesarApplication();
        qd::CreateApplicationParams prm;
        g_pApp->onConstruct(prm);
    }

    // initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        printf("NFD_Init failed: %s\n", NFD_GetError());
        return 0;
    }

    // app main loop
    ::g_pApp->initialize();
    ::g_pApp->doMainLoop();

    // destroy app
    ::g_pApp->destroy();
    NFD_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
}
