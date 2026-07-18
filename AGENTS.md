# Pre-Commit Checklist for AI Agents

This document describes what must be verified before committing changes to pass GitHub workflow validations.

## Quick Reference

| Check | Command | When Required |
|-------|---------|---------------|
| Format | `clang-format -i <files>` | Any C/C++ file change |
| Build (Windows) | `cmake -B build && cmake --build build` | Any src/ change |
| Build (Linux) | `cmake -B build -G Ninja && ninja -C build` | Any src/ change |
| Build (macOS) | `cmake -B build -G Ninja && ninja -C build` | Any src/ change |

## 1. Code Formatting (All Platforms)

**Workflow:** `format_check.yml`  
**Triggers on:** Changes to `src/**`, `.clang-format`, `scripts/ci/check-format.sh`

Before committing any C/C++ changes, run clang-format version 18:

```bash
# Format specific files
clang-format -i src/path/to/file.cpp

# Format all changed files
find src -type f \( -name '*.cpp' -o -name '*.h' \) -exec clang-format -i {} +

# Preview what the CI will check (compare against a base commit)
scripts/ci/check-format.sh --changed HEAD~1
```

**Common issues:**
- Include alignment comments must match clang-format's spacing
- Long function calls may be wrapped differently than hand-written
- The CI uses Ubuntu's clang-format-18; local versions may differ slightly

## 2. Windows Build

**Workflow:** `ci.yml` (windows job), `release.yml` (build-windows-msvc, build-windows-mingw)  
**Runners:** windows-2022, windows-2025

### MSVC Build
```powershell
# Using bundled CMake
bin\win\cmake\bin\cmake.exe -B build
msbuild build\quaesar.sln /p:Configuration=Debug /p:Platform="x64"

# Or with system CMake + Ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### MinGW Build (via MSYS2)
```bash
# In MSYS2 MINGW64 shell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Common issues:**
- MSVC treats warnings as errors (`/WX`); fix all warnings
- CRT deprecation warnings are suppressed via `_CRT_SECURE_NO_WARNINGS`
- Ensure submodules are initialized: `git submodule update --init --recursive`

## 3. Linux Build

**Workflow:** `ci.yml` (linux_ubuntu, linux_clang), `release.yml` (build-linux)  
**Compilers:** GCC 11/12/13, Clang 12-15  
**Runners:** ubuntu-22.04, ubuntu-24.04

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt-get install -y libsdl2-dev cmake ninja-build libgtk-3-dev

# Build with Ninja
cmake -B build -G Ninja
ninja -C build

# Or with Make
cmake -B build
make -C build -j$(nproc)
```

**Build variants tested by CI:**
- GTK file dialogs: `-DNFD_PORTAL=OFF` (requires `libgtk-3-dev`)
- Portal file dialogs: `-DNFD_PORTAL=ON` (requires `libdbus-1-dev`)

**Common issues:**
- Different GCC/Clang versions may produce different warnings
- Ensure code compiles warning-free on both GCC and Clang

## 4. macOS Build

**Workflow:** `ci.yml` (macos), `release.yml` (build-macos-arm64)  
**Runner:** macos-14 (Apple Silicon)

```bash
# Install dependencies
brew install sdl2 cmake ninja

# Build
cmake -B build -G Ninja -DCMAKE_OSX_ARCHITECTURES=arm64
ninja -C build
```

**Common issues:**
- ARM64-only build; x86_64 not tested in CI
- Ensure no macOS-specific deprecation warnings

## 5. Workflow Triggers

### format_check.yml
Only runs when these paths change:
- `src/**`
- `.clang-format`
- `scripts/ci/check-format.sh`

### release.yml
Skipped when only these paths change:
- `.github/**`
- `**.md`
- `**.py`
- `docs/**`

### ci.yml
Runs on pushes/PRs to `main` and `debugger-start` branches.

## 6. Pre-Commit Summary

1. **Format code:** `clang-format -i <changed-files>`
2. **Build locally** on at least one platform
3. **Check for warnings** — CI treats them as errors on some configs
4. **Verify submodules** are up to date if dependencies changed
5. **Don't commit** `.github/`, docs-only, or Python-only changes expecting a build — those are filtered out
