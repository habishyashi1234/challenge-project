add_executable(plugin_tests
    test_plugin.cpp
    test_plugin_segfault.cpp
)

target_include_directories(plugin_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/plugin/include
        ${CMAKE_SOURCE_DIR}/plugin_segfault/include
)

target_link_libraries(plugin_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

if(UNIX AND NOT APPLE)
    target_link_libraries(plugin_tests PRIVATE dl)
endif()

# ── CTest registration ─────────────────────────────────────────────────────────
add_test(NAME plugin_tests COMMAND plugin_tests)

if(WIN32)
    set(_plugin_file     "plugin.dll")
    set(_segfault_file        "plugin_segfault.dll")
else()
    set(_plugin_file     "libplugin.so")
    set(_segfault_file        "libplugin_segfault.so")
endif()

set_tests_properties(plugin_tests PROPERTIES
    ENVIRONMENT
        "PLUGIN_PATH=${CMAKE_BINARY_DIR}/bin/${_plugin_file};PLUGIN_SEGFAULT_PATH=${CMAKE_BINARY_DIR}/bin/${_segfault_file};SEGFAULT_LOG_PATH=${CMAKE_BINARY_DIR}/crash_report.txt"
)