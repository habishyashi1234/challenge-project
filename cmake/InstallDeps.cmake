include(GNUInstallDirs)

if(WIN32)
    set(_lib_pattern "*.dll")
    set(_dest "${CMAKE_INSTALL_BINDIR}")
else()
    set(_lib_pattern "*.so*")
    set(_dest "${CMAKE_INSTALL_LIBDIR}")
endif()

# Conan 2 CMakeDeps sets boost_LIB_DIRS_<CONFIG> after find_package(Boost)
string(TOUPPER "${CMAKE_BUILD_TYPE}" _build_type_upper)
set(_boost_lib_dir "${boost_LIB_DIRS_${_build_type_upper}}")

if(_boost_lib_dir AND EXISTS "${_boost_lib_dir}")
    fget_filename_component(_boost_root "${_boost_lib_dir}" DIRECTORY)
    file(GLOB _boost_libs
        "${_boost_lib_dir}/${_lib_pattern}"
        "${_boost_root}/bin/${_lib_pattern}"
    )
    if(_boost_libs)
        install(FILES ${_boost_libs}
            DESTINATION ${_dest}
            COMPONENT Runtime
        )
    else()
        message(WARNING "No Boost libraries found in ${_boost_lib_dir}")
    endif()
else()
    message(WARNING "boost_LIB_DIRS_${_build_type_upper} not set or does not exist")
endif()