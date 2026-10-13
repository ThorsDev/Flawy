include_guard(GLOBAL)

function(flawy_configure_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus /MP)
        if(FLAWY_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor)
        if(FLAWY_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if(WIN32)
        target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()

    get_target_property(FLAWY_TARGET_TYPE ${target} TYPE)
    if(FLAWY_TARGET_TYPE STREQUAL "EXECUTABLE")
        flawy_copy_runtime_dependencies(${target})
    endif()
endfunction()

function(flawy_copy_runtime_dependencies target)
    if(NOT WIN32)
        return()
    endif()

    set(FLAWY_RUNTIME_DLLS "$<TARGET_RUNTIME_DLLS:${target}>")
    set(FLAWY_COPY_COMMAND "${CMAKE_COMMAND};-E;copy_if_different;${FLAWY_RUNTIME_DLLS};$<TARGET_FILE_DIR:${target}>")
    set(FLAWY_NOOP_COMMAND "${CMAKE_COMMAND};-E;true")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "$<IF:$<BOOL:${FLAWY_RUNTIME_DLLS}>,${FLAWY_COPY_COMMAND},${FLAWY_NOOP_COMMAND}>"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()
