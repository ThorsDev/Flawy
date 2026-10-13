cmake_minimum_required(VERSION 3.25)

project(Flawy
    VERSION 0.1.0
    DESCRIPTION "Flawy multiplatform game engine"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

option(FLAWY_BUILD_SANDBOX "Build the Flawy sandbox application" ${PROJECT_IS_TOP_LEVEL})
option(FLAWY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors in Flawy targets" ON)
option(FLAWY_ENABLE_PROFILING "Enable Tracy profiling instrumentation" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FlawyCompilerOptions)
include(FlawyDependencies)

add_subdirectory(Flawy)

if(FLAWY_BUILD_SANDBOX)
    add_subdirectory(Flawy-Sandbox)
endif()
