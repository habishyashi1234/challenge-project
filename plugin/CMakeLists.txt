add_library(plugin SHARED
    src/plugin.cpp
)

target_compile_definitions(plugin PRIVATE PLUGIN_BUILDING)

target_include_directories(plugin
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(plugin
    PRIVATE
        Boost::log
        Boost::log_setup
        Boost::filesystem
)

if(ENABLE_SANITIZERS)
    target_link_libraries(plugin PRIVATE challenge::sanitizers)
endif()

set_target_properties(plugin PROPERTIES
    OUTPUT_NAME "plugin"
    BUILD_RPATH  "$ORIGIN"
    INSTALL_RPATH "$ORIGIN/../lib"
)

include(GNUInstallDirs)

install(TARGETS plugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)