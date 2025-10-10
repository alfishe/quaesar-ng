#include <SDL.h>
#include <nfd.h>
#include <qd/app/appPartsMgr.h>
#include <qd/app/application.h>
#include <qd/thread/thread.h>
#include <cstdarg>
#include <cstdio>
#include "amDebugger/debuggerWndApp.h"
#include "cli11/CLI11.hpp"
#include "qsr_application.h"
#include "qsr_config.h"
#include "quaesar.h"


#ifdef WIN32
#define SDL_MAIN_NEEDED
#include <SDL_main.h>
#else
// WTF SDL2MAIN_LIBRARY - I can't to build it on MacOs
#define SDL_main main
#endif  // WIN32


// Quaesar main
int SDL_main(int argc, char* argv[]) {
    // read options from CLI

    CLI::App cliApp{"Quaesar"};
    cliApp.allow_extras();
    cliApp.add_option("input", g_cfg_startup->input,
                      "Executable or image file (adf, dms)");  // ->check(CLI::ExistingFile);
    cliApp.add_option("-k,--kickstart", g_cfg_startup->kickRomPath,
                      "Path to the kickstart ROM");  // ->check(CLI::ExistingFile);
    cliApp.add_option("--serial_port", g_cfg_startup->serialPort, "Serial port path");
    cliApp.add_option("-s", g_cfg_startup->uaeExtArgs,
                      "key followed by the original WinUAE commands. Example:\n"
                      "   quaesar.exe -k c:\\Amiga\\KICK13.rom -s filesystem=rw,dh0:c:\\Amiga\\hd0");
    cliApp.parse(argc, argv);

    // initialize SDL
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
