#include "quaesar.h"
#include <SDL.h>
#include <amDebugger/debugger.h>
#include <qd/App/appPartsMgr.h>
#include <qd/App/appliction.h>
#include <qd/Thread/thread.h>
#include <stdarg.h>
#include <stdio.h>

// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "external/cli11/CLI11.hpp"
#include "parse_options.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on

#ifdef WIN32
#define SDL_MAIN_NEEDED
#include <SDL_main.h>
#else
// WTF SDL2MAIN_LIBRARY - I can't to build it on MacOs
#define SDL_main main
#endif  // WIN32


amD::QuasarApp* app = nullptr;

extern void real_main(int argc, TCHAR** argv);
extern void keyboard_settrans();

namespace amD {
extern void quae_parse_cmdline(int argc, TCHAR** argv);
};  //namespace amD


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: Move this somewhere else

bool ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    // Compare the end of the string with the suffix
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}


static std::vector<std::string> uaeCliArgVals;

int uae_thread_main_func(void*) {
    std::vector<const char*> argv;
    argv.push_back("quasar.exe");
    argv.reserve(uaeCliArgVals.size() * 2 + 1);
    for (const std::string& s : uaeCliArgVals) {
        argv.push_back("-s");
        argv.push_back(s.c_str());
    }
    amD::quae_parse_cmdline((int)argv.size(), const_cast<char**>(&argv[0]));

    ::real_main(0, nullptr);
    return 0;
}


// Quaesar main
int SDL_main(int argc, char* argv[]) {
    app = new amD::QuasarApp();
    qd::CreateApplicationParams prm;
    app->onCreate(prm);

    ::syncbase = 1000000;

    Options options;
    CLI::App cliApp{"Quaesar"};
    cliApp.allow_extras();
    cliApp.add_option("input", options.input, "Executable or image file (adf, dms)");     // ->check(CLI::ExistingFile);
    cliApp.add_option("-k,--kickstart", options.kickstart, "Path to the kickstart ROM");  // ->check(CLI::ExistingFile);
    cliApp.add_option("--serial_port", options.serial_port, "Serial port path");
    cliApp.add_option("-s", uaeCliArgVals,
                      "key followed by the original WinUAE commands. Example:\n"
                      "   quaesar.exe -k c:\\Amiga\\KICK13.rom -s filesystem=rw,dh0:c:\\Amiga\\hd0");

    cliApp.parse(argc, argv);

    keyboard_settrans();
    default_prefs(&currprefs, true, 0);
    fixup_prefs(&currprefs, true);

    if (!options.input.empty()) {
        // TODO: cleanup
        if (ends_with(options.input.c_str(), ".exe") || !ends_with(options.input.c_str(), ".adf")) {
            if (FILE* check_file = fopen(options.input.c_str(), "rb")) {
                fclose(check_file);
                Adf::create_for_exefile(options.input.c_str());
                strcpy(currprefs.floppyslots[0].df, "dummy.adf");
            } else {
                SDL_Log("can't open input file:'%s'", options.input.c_str());
            }
        } else {
            strcpy(currprefs.floppyslots[0].df, options.input.c_str());
        }
    }

    if (!options.serial_port.empty()) {
        currprefs.use_serial = 1;
        strcpy(currprefs.sername, options.serial_port.c_str());
    }

    // Most compatible mode
    currprefs.cpu_cycle_exact = 1;
    currprefs.cpu_memory_cycle_exact = 1;
    currprefs.blitter_cycle_exact = 1;
    currprefs.floppy_speed = 100;
    //    currprefs.turbo_emulation = 1; // it disables sound
    currprefs.sound_stereo_separation = 0;
    currprefs.uaeboard = 1;
    currprefs.win32_filesystem_mangle_reserved_names = true;  // required for FS
    currprefs.filesys_custom_uaefsdb = false;                 // hack to not implement 'custom_fsdb_*' funcs now

    strcpy(currprefs.romfile, options.kickstart.c_str());

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    atexit(&SDL_Quit);

    // start UAE in separate thread
    SDL_Thread* uae_thread_handler;
    uae_thread_handler = SDL_CreateThread(&uae_thread_main_func, "UAE emulator", nullptr);

    // wait UAE initialization
    amD::onUaeInitialized = new qd::ThreadEvent(true);
    amD::onUaeInitialized->wait();

    // quaesar main loop
    ::app->initialize();
    ::app->doMainLoop();

    // quit
    SDL_Log("Waiting UAE thread over ...");
    app->m_pDebugger->execConsoleCmd("q");

    // wait UAE done
    SDL_WaitThread(uae_thread_handler, nullptr);
    SAFE_DELETE(amD::onUaeInitialized);

    ::app->destroy();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
}
