# Quaesar /ˈkweɪ.zɑr/ [![CI](https://github.com/alfishe/quaesar-ng/actions/workflows/ci.yml/badge.svg)](https://github.com/alfishe/quaesar-ng/actions/workflows/ci.yml) [![Download](https://img.shields.io/github/v/release/alfishe/quaesar-ng?label=Download&logo=github)](https://github.com/alfishe/quaesar-ng/releases)

<img src="https://raw.githubusercontent.com/theblacklotus/quaesar/main/bin/quaesar.png">

Quaesar is an emulator based on [WinUAE](https://github.com/tonioni/WinUAE), aimed primarily at demosceners and demo developers. First off, Quaesar does not intend to replace WinUAE; it should be viewed as an alternative within a very specific niche.

So, what sets Quaesar apart?

 * Fully cross-platform: Runs on Linux, Windows, and macOS, based on the latest WinUAE code, and runs full CI on GitHub for all platforms.
 * Focuses on specific Amiga platforms only: A500(+)/A600, A1200, A1230, A1260, which are the most popular platforms for demos. Features such as graphics card support have been or will be removed.
 * The primary target is A500 512/512, which is the default configuration with accurate emulation settings.
 *  Still possible to configure options such as memory and CPU from the command line if needed.

## 🔍 Status

Quaesar is actively developed. Current capabilities include:

* Full sound output with two engine modes (see below).
* Snapshot save/load (state files) via hotkeys, CLI, and drag-and-drop.
* Built-in debugger with a full UI (registers, disassembly, memory inspector, breakpoints).
* Dual backend engines: WinUAE (default) and vAmiga, selectable at launch.
* Hard-drive / host-directory mounting via WinUAE `-s` syntax.
* Drag-and-drop: drop `.adf`/`.dms`/`.img` files to swap floppies, or snapshot files to restore state.

## 🚀 Quick Start

```
quaesar demo.adf -k kick13.rom
```

Mount a host directory as a hard drive (Kickstart 3.1):

```
quaesar -k roms/kick31.rom -s quickstart=A1200,0 -s filesystem2=rw,DH0:Work:/path/to/dir,0
```

Boot from a snapshot:
```
quaesar savegame.uss -k roms/kick13.rom
```

## 🎛️ Command-Line Reference

| Option | Description |
| :--- | :--- |
| `input` (positional) | Floppy image (`.adf`, `.dms`), executable, or snapshot file (`.uss` / `.vasnap`). Snapshots are auto-detected by magic bytes. |
| `-k`, `--kickstart <path>` | Path to the Kickstart ROM. |
| `-s <key>=<value>` | Pass-through to the WinUAE configuration system. Repeatable. See [WinUAE config keys](https://github.com/tonioni/WinUAE/wiki). Common: `quickstart=A500,0`, `cpu_model=68000`, `chipmem_size=2`, `filesystem2=rw,DH0:/path,0`, `floppy_speed=800`. |
| `--engine <uae\|vamiga>` | Select emulation backend (default: `uae`). |
| `--serial_port <path>` | Map the Amiga serial port to a host file or device. |
| `--sound-engine <native\|pwm>` | `native` = core default audio path. `pwm` = CIC/boxcar resampling + punch/room post-processing (default: `native`). |
| `--sound-punch` / `--no-sound-punch` | Enable/disable punch transient enhancement (PWM engine only, default: on). |
| `--sound-room <mode>` | Room simulation for headphones (PWM engine only): `off`, `-15db`, `-14db`, `-13db`, `-12db`, `-9db` (default: `off`). |

## ⌨️ Hotkeys

| Key | Action |
| :--- | :--- |
| **F12** | Toggle on-screen config overlay (ImGui). |
| **Shift+F12** | Open the integrated debugger window. |
| **F5** | Load quicksave snapshot. |
| **Shift+F5** | Save quicksave snapshot. |
| **Ctrl+R** | Hard reset the Amiga. |
| **ESC** | Release mouse capture (or quit if configured). |
| **Click** | Capture mouse into emulator window. |

Quicksave files are stored in `data/snapshots/quicksave.uss`.

## 📦 Snapshot Files

* **`.uss`** — UAE engine state files (ASF format).
* **`.vasnap`** — vAmiga engine state files.

Snapshots can be loaded at startup (as the positional argument), via drag-and-drop onto the emulator window, or restored at runtime with **F5**. The engine type is auto-detected from file magic bytes.

## 🏗️ Build

Quaesar uses [CMake](https://cmake.org) to configure the build step. See each platform below for the required steps needed.

### Windows

Visual Studio (with the Windows SDK) needs to be installed and the Community edition works fine. Currently Visual Studio 2019 and 2022 is being tested, but older versions may work also.

1. Run or double click `scripts\open_vs_solution.cmd`
2. Build the solution and start it as with any other program. 

### Linux 

Linux version depends on CMake and SDL2. Each distro has various ways on installing, but this is how Ubuntu/Debian would do it. Ninja build here is optional, but recommended.

```
apt-get install libsdl2-dev cmake ninja-build
```

To build (using Ninja)

```
mkdir output && cd output && cmake .. -G Ninja && ninja
```

To build (using make)

```
mkdir output && cd output && cmake .. && make -j$(nproc)
```

### macOS

The steps for macOS are identical to Linux, except you usually use [Homebrew](https://brew.sh) to install packages. 

```
brew install sdl2 cmake ninja
```

