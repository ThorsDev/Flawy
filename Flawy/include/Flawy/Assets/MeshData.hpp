#pragma once

#include <glm/ext/vector_uint4_sized.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Assets
{
    struct MeshSection
    {
        std::string sMaterialName;
        uint32_t uFirstIndex = 0;
        uint32_t uIndexCount = 0;
    };

    struct MeshData
    {
        static constexpr uint32_t kFormatVersion = 1;
        static constexpr uint32_t kMaxVertexCount = 16u * 1024u * 1024u;
        static constexpr uint32_t kMaxIndexCount = 16u * 1024u * 1024u;
        static constexpr uint32_t kMaxNameLength = 4096;
        static constexpr uint32_t kMaxSectionCount = 65536;
        static constexpr std::string_view kDefaultSectionName = "Default";

        std::string sName;
        std::vector<glm::vec3> vPositions;
        std::vector<glm::vec3> vNormals;
        std::vector<glm::vec4> vTangents;
        std::vector<glm::vec2> vTexCoords0;
        std::vector<uint32_t> vColorsAbgr;
        std::vector<glm::u8vec4> vJointIndices;
        std::vector<glm::vec4> vJointWeights;
        std::vector<uint32_t> vIndices;
        std::vector<MeshSection> vSections;

        [[nodiscard]] uint32_t GetVertexCount() const noexcept;
        [[nodiscard]] bool HasTangents() const noexcept;
        [[nodiscard]] bool HasSkinning() const noexcept;
        void Validate() const;
    };
}
