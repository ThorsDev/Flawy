file(GLOB_RECURSE FLAWY_ENGINE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE FLAWY_ENGINE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

add_library(Flawy STATIC ${FLAWY_ENGINE_HEADERS} ${FLAWY_ENGINE_SOURCES})
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
        miniaudio
        fastgltf::fastgltf
        angelscript
        angelscript_addons
        Jolt
        Tracy::TracyClient
)

flawy_configure_target(Flawy)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${FLAWY_ENGINE_HEADERS} ${FLAWY_ENGINE_SOURCES})
set_target_properties(Flawy PROPERTIES FOLDER "Engine")
