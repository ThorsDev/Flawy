file(GLOB_RECURSE FLAWY_SANDBOX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

file(GLOB FLAWY_SANDBOX_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang")

add_executable(Flawy-Sandbox ${FLAWY_SANDBOX_SOURCES} ${FLAWY_SANDBOX_SHADERS})

target_link_libraries(Flawy-Sandbox PRIVATE Flawy::Flawy)
target_compile_definitions(Flawy-Sandbox PRIVATE FLAWY_SANDBOX_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

flawy_configure_target(Flawy-Sandbox)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${FLAWY_SANDBOX_SOURCES} ${FLAWY_SANDBOX_SHADERS})
set_target_properties(Flawy-Sandbox PROPERTIES
    FOLDER "Applications"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Flawy-Sandbox)
endif()
