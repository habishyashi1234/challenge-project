add_executable(plugin_tests
    test_plugin.cpp
)

target_include_directories(plugin_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/plugin/include
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
else()
    set(_plugin_file     "libplugin.so")
endif()

set_tests_properties(plugin_tests PROPERTIES
    ENVIRONMENT
        "PLUGIN_PATH=${CMAKE_BINARY_DIR}/bin/${_plugin_file};"
)