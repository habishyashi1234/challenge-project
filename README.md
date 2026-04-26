# Challenge — Runtime Plugin System

A C++20 project demonstrating a runtime plugin architecture built with modern
CMake and Conan 2. The host executable discovers and loads shared libraries at
runtime via `dlopen` (Linux) and `LoadLibrary` (Windows) — there is no
link-time dependency between the host and its plugins. The project ships a
full GitHub Actions pipeline covering Linux (GCC and Clang) and Windows
(MSVC), static analysis, sanitizer runs, and automated release packaging.

---

## What gets built

**`challenge`** — the host executable. It loads both plugins at startup by
calling `dlopen`/`LoadLibrary`, resolves their exported symbols with
`dlsym`/`GetProcAddress`, and invokes them through C-compatible function
pointers. The host has no link-time knowledge of either plugin — they can be
swapped or extended without recompiling anything.

**`libplugin.so` / `plugin.dll`** — the demo plugin. Uses Boost.Log to write
a message on initialisation and exposes three symbols: `plugin_init`,
`plugin_get_name`, and `plugin_add`. All symbols use `extern "C"` linkage with
the `PLUGIN_API` visibility attribute so `dlsym` finds them by plain C name
across compilers and optimisation levels.

**`libplugin_segfault.so` / `plugin_segfault.dll`** — the crash detection
plugin. Installs a `SIGSEGV`/`SIGBUS` signal handler on Linux (using
`Boost::stacktrace_backtrace`) or a structured exception handler on Windows
(using `Boost::stacktrace_windbg`) that writes a full call-stack backtrace to
a file before letting the process terminate normally. The host always loads
this plugin first so the handler is active before any other code runs.

The crash report is written to `crash_report.txt` next to the executable by
default. Override it with the `SEGFAULT_LOG_PATH` environment variable:

```bash
SEGFAULT_LOG_PATH=/tmp/myapp_crash.txt ./build/Release/bin/challenge
```

---

## Prerequisites

| Tool | Version needed | How to get it |
|------|---------------|---------------|
| CMake | **≥ 3.23** | see note below |
| Conan | 2.x | `pip install conan` |
| Ninja | any | `pip install ninja` or system package |
| GCC | 11+ | system package or toolchain |
| Clang | 14+ | system package |
| MSVC | 2022 (19.3x) | Visual Studio or Build Tools |

### Why 3.23 and not 3.21

`CMakeLists.txt` declares `cmake_minimum_required(VERSION 3.21)` because the
build logic itself needs nothing beyond 3.21. The mismatch comes from Conan 2.
When `conan install` runs, `CMakeToolchain` generates `ConanPresets.json` using
preset schema `"version": 4`. CMake only understands schema version 4 from
3.23 onwards — on 3.21 or 3.22 the command `cmake --preset conan-release`
fails immediately with a preset schema version error.

If upgrading CMake is not an option and you are stuck on 3.21 or 3.22, you
can bypass the preset system entirely by passing the toolchain file directly.
After running `conan install`, Conan prints the exact fallback command for
your platform:

**Linux:**
```bash
cmake build/Release \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build/Release
ctest --test-dir build/Release --output-on-failure
```

The toolchain path and build directory come from Conan's `cmake_layout` — 
`build/Release` on Linux and `build` on Windows with the Ninja generator.

On Ubuntu 22.04 the system CMake is typically 3.22. Install a newer version
from the Kitware APT repository:

```bash
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
  | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ jammy main'
sudo apt-get update && sudo apt-get install cmake
```

Alternatively `pip install cmake` installs the latest stable release directly.

---

## Conan profiles

Compiler settings live in the `profiles/` directory and are committed to the
repository. This avoids `conan profile detect` which reads the current machine
and produces different results in different environments — not reliable for
reproducible CI.

```
profiles/
├── linux-gcc       GCC 13, C++20, libstdc++11
├── linux-clang     Clang 18, C++20, libstdc++11
└── windows-msvc    MSVC 194 (VS 2022), C++20, dynamic CRT
```

Always pass both `--profile:host` and `--profile:build` to Conan. In a normal
build both point to the same file. In cross-compilation the build profile
describes the machine running the compiler and the host profile describes the
target device — Conan keeps the package IDs for each context independent so
you never accidentally mix binaries built for different architectures.

---

## Building

### Linux — GCC

```bash
conan install . --build=missing -s build_type=Release \
    --profile:host=profiles/linux-gcc \
    --profile:build=profiles/linux-gcc

cmake --preset conan-release
cmake --build --preset conan-release
ctest --preset conan-release --output-on-failure
```

### Linux — Clang

```bash
conan install . --build=missing -s build_type=Release \
    --profile:host=profiles/linux-clang \
    --profile:build=profiles/linux-clang

cmake --preset conan-release
cmake --build --preset conan-release
ctest --preset conan-release --output-on-failure
```

### Windows — MSVC

Open a **Developer Command Prompt for VS 2022** (or run `vcvars64.bat`) so
that `cl.exe` and `ninja.exe` are on the path, then:

```bat
conan install . --build=missing -s build_type=Release ^
    --profile:host=profiles/windows-msvc ^
    --profile:build=profiles/windows-msvc

cmake --preset conan-release
cmake --build --preset conan-release
ctest --preset conan-release --output-on-failure
```

The first `conan install` downloads and compiles Boost from source, which
takes 10–20 minutes. Subsequent runs are instant because Conan caches compiled
binaries in `~/.conan2/p/` (Linux) or `%USERPROFILE%\.conan2\p\` (Windows).

---

## Running from the build tree

All outputs land in `build/Release/bin/` so the executable finds the plugins
without any `LD_LIBRARY_PATH` or `PATH` changes:

```bash
./build/Release/bin/challenge        # Linux
.\build\Release\bin\challenge.exe    # Windows
```

Expected output:

```
Loading plugin from: .../build/Release/bin/libplugin_segfault.so
Plugin name : plugin_segfault
Loading plugin from: .../build/Release/bin/libplugin.so
Plugin name : challange_plugin
plugin_add(3, 4): 7
```

---

## Installing

The install target produces a fully self-contained directory tree. All Boost
shared libraries are copied alongside the project binaries so the installed
tree runs without the Conan cache or any ambient environment variables.

```bash
# Stage to ./install relative to the project root
DESTDIR=./install cmake --build --preset conan-release --target install
```

`DESTDIR` prepends to every install destination path — it is the standard Unix
staging mechanism used by Make, CMake, and most package tools.

### Linux install layout

```
install/
├── bin/
│   └── challenge
└── lib/
    ├── libplugin.so
    ├── libplugin_segfault.so
    ├── libboost_log.so.1.84.0
    ├── libboost_log_setup.so.1.84.0
    └── ...
```

### Windows install layout

```
install/
└── bin/
    ├── challenge.exe
    ├── plugin.dll
    ├── plugin_segfault.dll
    ├── boost_log-vc143-mt-x64-1_84.dll
    └── ...
```

Run from the install tree:

```bash
./install/bin/challenge       # Linux
.\install\bin\challenge.exe   # Windows
```

On Linux the binary carries an RPATH of `$ORIGIN/../lib` so it finds shared
libraries relative to its own location regardless of where the install tree is
placed.

---

## Linting

Both tools need a configure step first so that `compile_commands.json` exists.

### clang-format

```bash
# Check for violations — used in CI, exits non-zero on any violation
cmake --build --preset conan-release --target format-check

# Fix violations in place — use this locally before committing
cmake --build --preset conan-release --target format-fix
```

### clang-tidy

Runs automatically during compilation when enabled:

```bash
cmake --preset conan-release -DENABLE_CLANG_TIDY=ON
cmake --build --preset conan-release
```

The `.clang-tidy` config enables `cppcoreguidelines-*`, `modernize-*`,
`performance-*`, and `readability-*` checks with `WarningsAsErrors=*` so any
violation stops the build immediately.

---

## Sanitizers

AddressSanitizer and UndefinedBehaviorSanitizer work on Linux with both GCC
and Clang. MSVC supports ASan only — UBSan is not available on Windows.

```bash
cmake --preset conan-release -DENABLE_SANITIZERS=ON
cmake --build --preset conan-release
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --preset conan-release --output-on-failure
```

`detect_leaks=0` disables LeakSanitizer. Boost has intentional one-time
startup allocations that LSan reports as leaks — they are not bugs in this
project. ASan and UBSan still fully instrument all memory accesses and
undefined behaviour in project code.

The `ENABLE_SANITIZERS` option links an interface target `challenge::sanitizers`
to every project target including both plugins. This is necessary because the
plugins are loaded at runtime — if only the host binary is instrumented but the
plugins are not, ASan cannot detect errors that occur inside plugin code.

---

## CI pipeline

The pipeline runs on every push to `main`, on pull requests, and on `v*.*.*`
tags.

| Job | Platform | Description |
|-----|----------|-------------|
| Build & Test | ubuntu/GCC, ubuntu/Clang, windows/MSVC | Conan install, configure, build, ctest |
| Lint | ubuntu/GCC | clang-format check + clang-tidy, fails on any violation |
| ASan + UBSan | ubuntu/GCC, ubuntu/Clang | Full test suite under sanitizer instrumentation |
| Install & Package | same matrix as Build & Test | cmake install, archive as `.tar.gz` or `.zip`, upload as workflow artifact |
| Release | ubuntu | Only on `v*.*.*` tags — attaches all archives to a GitHub Release |

Conan binaries are cached between runs using `actions/cache` keyed on the
`conanfile.py` hash and the compiler. Boost is only compiled from source on
the first run for each compiler or when `conanfile.py` changes.

---

## Project layout

```
challenge-project/
├── app/
│   └── src/main.cpp                host — loads plugins via dlopen/LoadLibrary
├── plugin/
│   ├── include/plugin/plugin.h     C API and PLUGIN_API visibility macro
│   └── src/plugin.cpp              demo plugin using Boost.Log
├── plugin_segfault/
│   ├── include/plugin_segfault/plugin_segfault.h
│   └── src/plugin_segfault.cpp     signal handler + Boost.Stacktrace backtrace
├── tests/
│   ├── test_plugin.cpp             GTest suite for the demo plugin
│   └── test_plugin_segfault.cpp    GTest suite for the segfault plugin
├── cmake/
│   ├── InstallDeps.cmake           copies Conan shared libs into the install tree
│   └── Sanitizers.cmake            challenge::sanitizers interface target
├── profiles/
│   ├── linux-gcc                   Conan profile — GCC 13, Linux
│   ├── linux-clang                 Conan profile — Clang 18, Linux
│   └── windows-msvc                Conan profile — MSVC 194, Windows
├── .clang-format                   Google style, 4-space indent
├── .clang-tidy                     cppcoreguidelines + modernize + readability
├── CMakeLists.txt                  root build definition
├── CMakePresets.json               base + conan-release + conan-debug presets
└── conanfile.py                    Boost 1.84.0 (shared) + GTest 1.14.0 (static)
```