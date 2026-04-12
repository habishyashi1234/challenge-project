add_executable(challenge
    src/main.cpp
)

target_include_directories(challenge
    PRIVATE
        ${CMAKE_SOURCE_DIR}/plugin/include
)

if(UNIX AND NOT APPLE)
    target_link_libraries(challenge PRIVATE dl)
    target_link_options(challenge PRIVATE -Wl,--disable-new-dtags)
endif()

set_target_properties(challenge PROPERTIES
    BUILD_RPATH "${CONAN_RUNTIME_LIB_DIRS}"
    INSTALL_RPATH "$ORIGIN/../lib"
)

# ── Install rules ──────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS challenge
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)