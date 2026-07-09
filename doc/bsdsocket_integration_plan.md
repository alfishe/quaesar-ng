# Re-enabling BSDSocket Library Injection in Quaesar-NG

## Background
The `bsdsocket.library` emulation allows an Amiga OS guest to seamlessly utilize the host machine's networking stack for Internet access. WinUAE implements this heavily through Windows Winsock in `od-win32/bsdsock.cpp`.

Because Quaesar-NG focuses heavily on POSIX environments (macOS/Linux), the Windows-specific implementation cannot be used. Currently, the core machine-independent `bsdsocket.cpp` is excluded from the CMake build, `#define BSDSOCKET` is commented out, and the required startup symbols are stubbed out with `UNIMPLEMENTED()` traps in `dummy.cpp`.

## The Solution: POSIX Backend
The `fs-uae` emulator (located in `/Volumes/TB4-4Tb/Projects/emulators/fs-uae`) contains a fully POSIX-compliant implementation in `od-fs/bsdsocket_posix.cpp` which maps Amiga socket requests to standard UNIX network APIs (`sys/socket.h`, `netinet/in.h`, etc.). 

## Implementation Plan

### 1. Procurement of POSIX backend
- **Source:** Copy `/Volumes/TB4-4Tb/Projects/emulators/fs-uae/od-fs/bsdsocket_posix.cpp` to `/Volumes/TB4-4Tb/Projects/emulators/github/quaesar-ng/libs/uae_lib/bsdsocket_posix.cpp`.
- **Review:** Validate that `fs-uae`'s threading (e.g., `uae_sem_t`) and UAE environment definitions map seamlessly onto Quaesar-NG's UAE core definitions.

### 2. CMake and Preprocessor Integration
- **`CMakeLists.txt`:** Add `bsdsocket.cpp` and the new `bsdsocket_posix.cpp` to the `UAE_SOURCE_LIST`.
- **`sysconfig.h`:** Uncomment `#define BSDSOCKET` to globally enable the compilation of `bsdsocket.library` emulation hooks across the UAE core.

### 3. Removing Traps
- **`dummy.cpp`:** Erase the `UNIMPLEMENTED()` stubs for `bsdlib_install()`, `bsdlib_startup()`, and `bsdsock_fake_int_handler()`. The compilation of `bsdsocket.cpp` will now correctly provide these symbols.

### 4. Configuration and Usability
- The library injection is triggered internally by `currprefs.socket_emu`.
- The `cfgfile.cpp` parser already supports setting this via the `bsdsocket_emu` parameter.
- **Usage:** Users will launch the emulator with `-s bsdsocket_emu=true`. By default, this value is `false`, ensuring that networking is safely disabled unless explicitly requested by the user.
