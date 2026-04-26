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
    enable_sanitizers()
endif()


# ── Clang-tidy ─────────────────────────────────────────────────────────────────
option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis during build" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXE}"
            "--warnings-as-errors=*"
        )
        set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
            ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    else()
        message(WARNING "clang-tidy not found — skipping analysis")
    endif()
endif()

# ── Clang-format targets ───────────────────────────────────────────────────────
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_CXX_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/app/*.cpp"
        "${CMAKE_SOURCE_DIR}/plugin/*.cpp"
        "${CMAKE_SOURCE_DIR}/plugin/*.h"
        "${CMAKE_SOURCE_DIR}/plugin_segfault/*.cpp"
        "${CMAKE_SOURCE_DIR}/plugin_segfault/*.h"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_CXX_FILES}
        COMMENT "Checking clang-format violations"
        VERBATIM
    )
    add_custom_target(format-fix
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_CXX_FILES}
        COMMENT "Fixing clang-format violations"
        VERBATIM
    )
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