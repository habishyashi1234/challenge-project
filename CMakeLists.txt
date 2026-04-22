cmake_minimum_required(VERSION 3.21)

project(challenge
    VERSION 1.0.0
    LANGUAGES CXX
)

# ── Language standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Symbol visibility ──────────────────────────────────────────────────────────
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# ── Output directories ─────────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ── Options ────────────────────────────────────────────────────────────────────
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan (GCC/Clang only)" OFF)

# ── Sanitizer helper ───────────────────────────────────────────────────────────
if(ENABLE_SANITIZERS)
    include(cmake/Sanitizers.cmake)
endif()

# ── Dependencies via Conan ─────────────────────────────────────────────────────
find_package(Boost REQUIRED COMPONENTS log log_setup)
find_package(GTest REQUIRED)

# ── Subdirectories ─────────────────────────────────────────────────────────────
add_subdirectory(plugin)
add_subdirectory(plugin_segfault)
add_subdirectory(app)

# ── Tests ──────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)

# ── Install dependency handling ───────────────────────────────────────────
include(cmake/InstallDeps.cmake)