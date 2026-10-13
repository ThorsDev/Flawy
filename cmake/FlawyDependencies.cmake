include_guard(GLOBAL)

set(FLAWY_VENDOR_DIR "${PROJECT_SOURCE_DIR}/vendor" CACHE PATH "Directory containing the Flawy third-party dependencies")

set(FLAWY_REQUIRED_VENDORS SDL bgfx miniaudio simdjson fastgltf entt angelscript JoltPhysics glm imgui tracy spdlog slang)

foreach(vendor IN LISTS FLAWY_REQUIRED_VENDORS)
    if(NOT EXISTS "${FLAWY_VENDOR_DIR}/${vendor}")
        message(FATAL_ERROR
            "Missing dependency '${vendor}' in ${FLAWY_VENDOR_DIR}.\n"
            "Run scripts/install.ps1 (Windows) or scripts/install.sh (Linux/macOS) first.")
    endif()
endforeach()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_FOLDER "Vendor")

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_INSTALL OFF)
add_subdirectory("${FLAWY_VENDOR_DIR}/SDL" "${CMAKE_BINARY_DIR}/vendor/SDL" EXCLUDE_FROM_ALL SYSTEM)

set(BGFX_BUILD_EXAMPLES OFF)
set(BGFX_BUILD_TESTS OFF)
set(BGFX_INSTALL OFF)
set(BGFX_CUSTOM_TARGETS OFF)
set(BGFX_BUILD_TOOLS ON)
set(BGFX_BUILD_TOOLS_BIN2C OFF)
set(BGFX_BUILD_TOOLS_SHADER ON)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF)
set(BGFX_BUILD_TOOLS_TEXTURE OFF)
add_subdirectory("${FLAWY_VENDOR_DIR}/bgfx" "${CMAKE_BINARY_DIR}/vendor/bgfx" EXCLUDE_FROM_ALL SYSTEM)

add_library(miniaudio STATIC "${FLAWY_VENDOR_DIR}/miniaudio/miniaudio.c")
target_include_directories(miniaudio SYSTEM PUBLIC "${FLAWY_VENDOR_DIR}/miniaudio")
if(UNIX AND NOT APPLE)
    target_link_libraries(miniaudio PUBLIC ${CMAKE_DL_LIBS} pthread m)
endif()
set_target_properties(miniaudio PROPERTIES EXCLUDE_FROM_ALL ON)

set(SIMDJSON_DEVELOPER_MODE OFF)
set(SIMDJSON_ENABLE_THREADS ON)
add_subdirectory("${FLAWY_VENDOR_DIR}/simdjson" "${CMAKE_BINARY_DIR}/vendor/simdjson" EXCLUDE_FROM_ALL SYSTEM)

set(FASTGLTF_ENABLE_INSTALL OFF)
set(FASTGLTF_COMPILE_AS_CPP20 ON)
add_subdirectory("${FLAWY_VENDOR_DIR}/fastgltf" "${CMAKE_BINARY_DIR}/vendor/fastgltf" EXCLUDE_FROM_ALL SYSTEM)

add_subdirectory("${FLAWY_VENDOR_DIR}/entt" "${CMAKE_BINARY_DIR}/vendor/entt" EXCLUDE_FROM_ALL SYSTEM)

set(AS_DISABLE_INSTALL ON)
add_subdirectory("${FLAWY_VENDOR_DIR}/angelscript/sdk/angelscript/projects/cmake" "${CMAKE_BINARY_DIR}/vendor/angelscript" EXCLUDE_FROM_ALL SYSTEM)

set(FLAWY_ANGELSCRIPT_ADDON_DIR "${FLAWY_VENDOR_DIR}/angelscript/sdk/add_on")
add_library(angelscript_addons STATIC
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scriptarray/scriptarray.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scriptbuilder/scriptbuilder.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scriptdictionary/scriptdictionary.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scripthandle/scripthandle.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scripthelper/scripthelper.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scriptmath/scriptmath.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scriptstdstring/scriptstdstring.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/scriptstdstring/scriptstdstring_utils.cpp"
    "${FLAWY_ANGELSCRIPT_ADDON_DIR}/weakref/weakref.cpp"
)
target_include_directories(angelscript_addons SYSTEM PUBLIC "${FLAWY_ANGELSCRIPT_ADDON_DIR}")
target_link_libraries(angelscript_addons PUBLIC angelscript)
set_target_properties(angelscript_addons PROPERTIES EXCLUDE_FROM_ALL ON)

set(OVERRIDE_CXX_FLAGS OFF)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
set(ENABLE_ALL_WARNINGS OFF)
set(ENABLE_INSTALL OFF)
set(CPP_RTTI_ENABLED ON)
set(CPP_EXCEPTIONS_ENABLED ON)
set(INTERPROCEDURAL_OPTIMIZATION OFF)
set(JPH_USE_DX12 OFF)
set(JPH_USE_VK OFF)
set(JPH_USE_MTL OFF)
set(JPH_USE_CPU_COMPUTE OFF)
add_subdirectory("${FLAWY_VENDOR_DIR}/JoltPhysics/Build" "${CMAKE_BINARY_DIR}/vendor/JoltPhysics" EXCLUDE_FROM_ALL SYSTEM)

set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory("${FLAWY_VENDOR_DIR}/glm" "${CMAKE_BINARY_DIR}/vendor/glm" EXCLUDE_FROM_ALL SYSTEM)

set(FLAWY_IMGUI_DIR "${FLAWY_VENDOR_DIR}/imgui")
add_library(imgui STATIC
    "${FLAWY_IMGUI_DIR}/imgui.cpp"
    "${FLAWY_IMGUI_DIR}/imgui_demo.cpp"
    "${FLAWY_IMGUI_DIR}/imgui_draw.cpp"
    "${FLAWY_IMGUI_DIR}/imgui_tables.cpp"
    "${FLAWY_IMGUI_DIR}/imgui_widgets.cpp"
    "${FLAWY_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp"
)
target_include_directories(imgui SYSTEM PUBLIC "${FLAWY_IMGUI_DIR}" "${FLAWY_IMGUI_DIR}/backends")
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)
set_target_properties(imgui PROPERTIES EXCLUDE_FROM_ALL ON)

set(TRACY_ENABLE ${FLAWY_ENABLE_PROFILING})
set(TRACY_ON_DEMAND ON)
add_subdirectory("${FLAWY_VENDOR_DIR}/tracy" "${CMAKE_BINARY_DIR}/vendor/tracy" EXCLUDE_FROM_ALL SYSTEM)

set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_INSTALL OFF)
add_subdirectory("${FLAWY_VENDOR_DIR}/spdlog" "${CMAKE_BINARY_DIR}/vendor/spdlog" EXCLUDE_FROM_ALL SYSTEM)

set(FLAWY_SLANG_DIR "${FLAWY_VENDOR_DIR}/slang")
add_library(Slang::slang SHARED IMPORTED GLOBAL)
add_library(Slang::slang-compiler SHARED IMPORTED GLOBAL)
if(WIN32)
    set_target_properties(Slang::slang PROPERTIES
        IMPORTED_LOCATION "${FLAWY_SLANG_DIR}/bin/slang.dll"
        IMPORTED_IMPLIB "${FLAWY_SLANG_DIR}/lib/slang.lib"
    )
    set_target_properties(Slang::slang-compiler PROPERTIES
        IMPORTED_LOCATION "${FLAWY_SLANG_DIR}/bin/slang-compiler.dll"
        IMPORTED_IMPLIB "${FLAWY_SLANG_DIR}/lib/slang-compiler.lib"
    )
elseif(APPLE)
    set_target_properties(Slang::slang PROPERTIES IMPORTED_LOCATION "${FLAWY_SLANG_DIR}/lib/libslang.dylib")
    set_target_properties(Slang::slang-compiler PROPERTIES IMPORTED_LOCATION "${FLAWY_SLANG_DIR}/lib/libslang-compiler.dylib")
else()
    set_target_properties(Slang::slang PROPERTIES IMPORTED_LOCATION "${FLAWY_SLANG_DIR}/lib/libslang.so")
    set_target_properties(Slang::slang-compiler PROPERTIES IMPORTED_LOCATION "${FLAWY_SLANG_DIR}/lib/libslang-compiler.so")
endif()
set_target_properties(Slang::slang PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FLAWY_SLANG_DIR}/include"
    INTERFACE_COMPILE_DEFINITIONS SLANG_DYNAMIC
)
target_link_libraries(Slang::slang INTERFACE Slang::slang-compiler)

unset(CMAKE_FOLDER)
