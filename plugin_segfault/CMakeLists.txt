add_library(plugin_segfault SHARED
    src/plugin_segfault.cpp
)

target_compile_definitions(plugin_segfault PRIVATE PLUGIN_SEGFAULT_BUILDING)
target_compile_definitions(plugin_segfault PRIVATE BOOST_STACKTRACE_DYN_LINK)

target_include_directories(plugin_segfault
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_link_libraries(plugin_segfault PRIVATE Boost::stacktrace_windbg)
else()
    target_link_libraries(plugin_segfault PRIVATE Boost::stacktrace_backtrace)
endif()

set_target_properties(plugin_segfault PROPERTIES
    OUTPUT_NAME "plugin_segfault"
    BUILD_RPATH  "$ORIGIN"
    INSTALL_RPATH "$ORIGIN/../lib"
)

include(GNUInstallDirs)

install(TARGETS plugin_segfault
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)