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

On Ubuntu 22.04 the system CMake is typically 3.22. Install a newer version
from the Kitware APT repository:

```bash
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
  | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ jammy main'
sudo apt-get update && sudo apt-get install cmake
```

Alternatively `pip install cmake` installs the latest stable release directly.

### Stuck on CMake 3.21 or 3.22?

If upgrading is not an option you can bypass the preset system entirely.
After `conan install` finishes it prints the exact fallback command for your
platform — look for the line starting with `(cmake<3.23)`. On Linux it looks
like this:

```bash
# After conan install, use the toolchain file directly instead of --preset
cmake build/Release \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build/Release
ctest --test-dir build/Release --output-on-failure
```

The build output goes to the same `build/Release/` directory so everything
else — install, linting, sanitizers — works identically, just without the
`--preset` shorthand.

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

cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

### Linux — Clang

```bash
conan install . --build=missing -s build_type=Release \
    --profile:host=profiles/linux-clang \
    --profile:build=profiles/linux-clang

cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

### Windows — MSVC

Open a **Developer Command Prompt for VS 2022** (or run `vcvars64.bat`) so
that `cl.exe` and `ninja.exe` are on the path, then:

```bat
conan install . --build=missing -s build_type=Release ^
    --profile:host=profiles/windows-msvc ^
    --profile:build=profiles/windows-msvc

cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

The first `conan install` downloads and compiles Boost from source, which
takes 10–20 minutes. Subsequent runs are instant because Conan caches compiled
binaries in `~/.conan2/p/` (Linux) or `%USERPROFILE%\.conan2\p\` (Windows).

### Available presets

`CMakePresets.json` ships two named presets that you can use directly after
`conan install`:

| Preset | Build type | Use case |
|--------|-----------|---------|
| `release` | Release | Normal builds and CI |
| `debug` | Debug | Local debugging |

```bash
# Debug build — same workflow, just swap the preset name
conan install . --build=missing -s build_type=Debug \
    --profile:host=profiles/linux-gcc \
    --profile:build=profiles/linux-gcc

cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

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

`install` is a built-in CMake target — it is created automatically whenever
the project has any `install()` rule. No extra CMake code is needed to enable
it. Two equivalent ways exist to trigger it:

```bash
# Modern way — cmake --install reads cmake_install.cmake from the build dir
# and runs all install() rules. Prefix controls where files go.
cmake --install build/Release --prefix ./install        # Linux
cmake --install build --prefix ./install                 # Windows
```

```bash
# Older way — invokes the build system's install target
# Requires DESTDIR or CMAKE_INSTALL_PREFIX set at configure time
cmake --build --preset release --target install
```

Both commands run exactly the same install rules from the CMakeLists files.
The difference is syntax and how the prefix is specified — `cmake --install`
accepts `--prefix` directly on the command line, while `cmake --build --target
install` uses whatever `CMAKE_INSTALL_PREFIX` was set to at configure time, or
`DESTDIR` as a prepend override.

For local use `cmake --install` is simpler and cleaner. In CI the package job
uses `cmake --build --preset release --target install` with `DESTDIR` because
the preset resolves the build directory automatically across platforms, avoiding
separate Linux/Windows steps. An equally valid and arguably clearer CI approach
is to use platform-specific steps:

```yaml
- name: Install (Linux)
  if: runner.os == 'Linux'
  run: cmake --install build/Release --prefix ${{ github.workspace }}/install

- name: Install (Windows)
  if: runner.os == 'Windows'
  run: cmake --install build --prefix ${{ github.workspace }}/install
```

This makes the platform difference explicit and avoids the `DESTDIR` confusion
entirely.

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

### How the Boost libraries are bundled

`cmake/InstallDeps.cmake` uses CMake's `file(GET_RUNTIME_DEPENDENCIES)` which
reads the actual ELF import tables (Linux) or PE import tables (Windows) of
the installed binaries at install time. It collects every shared library the
binaries depend on, then copies them into the install prefix.

The destination is chosen without hardcoding any directory name:

```cmake
if(WIN32)
    set(_dep_dest "${CMAKE_INSTALL_BINDIR}")  # bin/ — where Windows looks for DLLs
else()
    set(_dep_dest "${CMAKE_INSTALL_LIBDIR}")  # lib/ — standard on Linux
endif()
```

System libraries are excluded by regex patterns so they are never copied —
the install tree only bundles what is not guaranteed to be on the target machine:

```cmake
POST_EXCLUDE_REGEXES
    "/lib/x86_64-linux-gnu/"   # Linux system libs
    "/lib64/"
    "/usr/lib/"
    "[Ww]indows"               # Windows system directory
    "api-ms-win"               # Windows API sets
    "VCRUNTIME"                # MSVC runtime
    "MSVCP"
    "ucrtbase"
    "kernel32"
    "user32"
```

This approach works on both platforms without any path hardcoding. On Linux
`GET_RUNTIME_DEPENDENCIES` follows `.so` symlinks via `FOLLOW_SYMLINK_CHAIN`
so versioned libraries like `libboost_log.so.1.84.0` are correctly included.

---

## Linting

Both tools need a configure step first so that `compile_commands.json` exists.

### clang-format

```bash
# Check for violations — used in CI, exits non-zero on any violation
cmake --build --preset release --target format-check

# Fix violations in place — use this locally before committing
cmake --build --preset release --target format-fix
```

### clang-tidy

Runs automatically during compilation when enabled:

```bash
cmake --preset release -DENABLE_CLANG_TIDY=ON
cmake --build --preset release
```

The `.clang-tidy` config enables `cppcoreguidelines-*`, `modernize-*`,
`performance-*`, and `readability-*` checks with `WarningsAsErrors=*` so any
violation stops the build immediately.

---

## Sanitizers

AddressSanitizer and UndefinedBehaviorSanitizer work on Linux with both GCC
and Clang. MSVC supports ASan only — UBSan is not available on Windows.

```bash
cmake --preset release -DENABLE_SANITIZERS=ON
cmake --build --preset release
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --preset release --output-on-failure
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

To trigger a release, push a version tag:

```bash
git tag v1.2.0
git push origin v1.2.0
```

The release job runs only after Build, Lint, and Package all pass. It then
creates a GitHub Release with the platform archives attached as downloadable
assets.

---

## Known limitations

**`InstallDeps.cmake` scans the installed executables and plugins** —
`file(GET_RUNTIME_DEPENDENCIES)` reads the ELF/PE import tables of the
installed `challenge`, `libplugin.so`, and `libplugin_segfault.so` to find
their runtime dependencies. This works correctly for the current dependency
graph but the list of binaries to scan is explicit. If you add a new plugin
that depends on additional shared libraries, add its installed path to the
`EXECUTABLES` list in `InstallDeps.cmake`.

**Clang version pinned to 18** — the `profiles/linux-clang` profile pins
`compiler.version=18` to match the default Clang on `ubuntu-latest` at the
time of writing. If GitHub upgrades the runner image to a newer Ubuntu,
update both `profiles/linux-clang` and the `apt-get install clang-18` step
in `ci.yml` to match.

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
├── CMakePresets.json               release + debug presets (conan-release generated)
└── conanfile.py                    Boost 1.84.0 (shared) + GTest 1.14.0 (static)
```