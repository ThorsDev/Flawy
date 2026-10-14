file(GLOB_RECURSE FLAWY_ENGINE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE FLAWY_ENGINE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE FLAWY_ENGINE_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang")

set(FLAWY_EMBEDDED_SHADER_TABLE "${CMAKE_CURRENT_BINARY_DIR}/Generated/EmbeddedShaderTable.cpp")
set(FLAWY_EMBED_SHADERS_SCRIPT "${PROJECT_SOURCE_DIR}/cmake/FlawyEmbedShaders.cmake")

add_custom_command(
    OUTPUT "${FLAWY_EMBEDDED_SHADER_TABLE}"
    COMMAND "${CMAKE_COMMAND}"
        "-DFLAWY_SHADER_DIR=${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "-DFLAWY_SHADER_OUTPUT=${FLAWY_EMBEDDED_SHADER_TABLE}"
        -P "${FLAWY_EMBED_SHADERS_SCRIPT}"
    DEPENDS ${FLAWY_ENGINE_SHADERS} "${FLAWY_EMBED_SHADERS_SCRIPT}"
    COMMENT "Embedding Flawy default shaders"
    VERBATIM
)

add_library(Flawy STATIC ${FLAWY_ENGINE_HEADERS} ${FLAWY_ENGINE_SOURCES} ${FLAWY_ENGINE_SHADERS} "${FLAWY_EMBEDDED_SHADER_TABLE}")
add_library(Flawy::Flawy ALIAS Flawy)

target_include_directories(Flawy
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(Flawy PUBLIC cxx_std_20)
target_compile_definitions(Flawy PRIVATE FLAWY_VERSION="${PROJECT_VERSION}")

target_link_libraries(Flawy
    PUBLIC
        glm::glm
        EnTT::EnTT
        spdlog::spdlog
        imgui
    PRIVATE
        SDL3::SDL3-static
        bgfx
        bimg
        bx
        miniaudio
        fastgltf::fastgltf
        angelscript
        angelscript_addons
        Jolt
        Tracy::TracyClient
        Slang::slang
)

flawy_configure_target(Flawy)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${FLAWY_ENGINE_HEADERS} ${FLAWY_ENGINE_SOURCES} ${FLAWY_ENGINE_SHADERS})
source_group("Generated" FILES "${FLAWY_EMBEDDED_SHADER_TABLE}")
set_target_properties(Flawy PROPERTIES FOLDER "Engine")
