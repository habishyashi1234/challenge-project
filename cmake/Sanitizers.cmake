# Creates an interface target challenge::sanitizers that any target
# can link against to opt into sanitizer instrumentation.
# Usage: target_link_libraries(my_target PRIVATE challenge::sanitizers)

function(enable_sanitizers)
    add_library(challenge_sanitizers INTERFACE)
    add_library(challenge::sanitizers ALIAS challenge_sanitizers)

    if(MSVC)
        # MSVC supports ASan only — no UBSan or TSan
        message(STATUS "Sanitizers: enabling MSVC AddressSanitizer")
        target_compile_options(challenge_sanitizers INTERFACE
            /fsanitize=address
            /Zi
        )
        # MSVC linker does not accept /fsanitize — compile only
        target_link_options(challenge_sanitizers INTERFACE
            /INCREMENTAL:NO
        )

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Sanitizers: enabling ASan + UBSan")
        target_compile_options(challenge_sanitizers INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(challenge_sanitizers INTERFACE
            -fsanitize=address,undefined
        )

    else()
        message(WARNING "Sanitizers: unsupported compiler ${CMAKE_CXX_COMPILER_ID} — skipping")
    endif()
endfunction()