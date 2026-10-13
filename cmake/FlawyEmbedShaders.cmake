cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED FLAWY_SHADER_DIR OR NOT DEFINED FLAWY_SHADER_OUTPUT)
    message(FATAL_ERROR "FlawyEmbedShaders.cmake requires FLAWY_SHADER_DIR and FLAWY_SHADER_OUTPUT")
endif()

file(GLOB_RECURSE FLAWY_SHADER_FILES RELATIVE "${FLAWY_SHADER_DIR}" "${FLAWY_SHADER_DIR}/*.slang")
list(SORT FLAWY_SHADER_FILES)

set(FLAWY_SHADER_ARRAYS "")
set(FLAWY_SHADER_ENTRIES "")
set(FLAWY_SHADER_INDEX 0)
string(REPEAT "0x[0-9a-f][0-9a-f]," 16 FLAWY_SHADER_LINE_PATTERN)

foreach(FLAWY_SHADER_FILE IN LISTS FLAWY_SHADER_FILES)
    file(READ "${FLAWY_SHADER_DIR}/${FLAWY_SHADER_FILE}" FLAWY_SHADER_HEX HEX)
    string(LENGTH "${FLAWY_SHADER_HEX}" FLAWY_SHADER_HEX_LENGTH)
    math(EXPR FLAWY_SHADER_SIZE "${FLAWY_SHADER_HEX_LENGTH} / 2")
    if(FLAWY_SHADER_SIZE EQUAL 0)
        message(FATAL_ERROR "Embedded shader is empty: ${FLAWY_SHADER_FILE}")
    endif()

    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FLAWY_SHADER_BYTES "${FLAWY_SHADER_HEX}")
    string(REGEX REPLACE "(${FLAWY_SHADER_LINE_PATTERN})" "\\1\n            " FLAWY_SHADER_BYTES "${FLAWY_SHADER_BYTES}")
    string(REGEX REPLACE "\n            $" "" FLAWY_SHADER_BYTES "${FLAWY_SHADER_BYTES}")

    string(APPEND FLAWY_SHADER_ARRAYS
        "        constexpr unsigned char kShaderData${FLAWY_SHADER_INDEX}[] = {\n"
        "            ${FLAWY_SHADER_BYTES}\n"
        "        };\n\n")
    string(APPEND FLAWY_SHADER_ENTRIES
        "            EmbeddedShaderEntry{\"${FLAWY_SHADER_FILE}\", kShaderData${FLAWY_SHADER_INDEX}, ${FLAWY_SHADER_SIZE}},\n")

    math(EXPR FLAWY_SHADER_INDEX "${FLAWY_SHADER_INDEX} + 1")
endforeach()

set(FLAWY_SHADER_SOURCE
"#include \"Renderer/Shader/EmbeddedShaderTable.hpp\"

#include <array>

namespace Flawy::Renderer
{
    namespace
    {
${FLAWY_SHADER_ARRAYS}        constexpr std::array<EmbeddedShaderEntry, ${FLAWY_SHADER_INDEX}> kEntries = {
${FLAWY_SHADER_ENTRIES}        };
    }

    std::span<const EmbeddedShaderEntry> EmbeddedShaderTable::GetEntries() noexcept
    {
        return kEntries;
    }
}
")

file(CONFIGURE OUTPUT "${FLAWY_SHADER_OUTPUT}" CONTENT "${FLAWY_SHADER_SOURCE}" @ONLY)
