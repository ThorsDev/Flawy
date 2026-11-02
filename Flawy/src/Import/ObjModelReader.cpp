#include "Import/ObjModelReader.hpp"

#include "Import/ImportMath.hpp"
#include "Import/NormalGenerator.hpp"
#include "Import/PathText.hpp"

#include "Flawy/Core/FileBytes.hpp"

#include <fast_obj.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Flawy::Import
{
    namespace
    {
        constexpr std::array<std::string_view, 1> kExtensions = {".obj"};
        constexpr std::string_view kFormatName = "OBJ";
        constexpr uint32_t kMinimumFaceCorners = 3;

        class ObjFileSystem final
        {
        public:
            ObjFileSystem() = default;
            ObjFileSystem(const ObjFileSystem&) = delete;
            ObjFileSystem& operator=(const ObjFileSystem&) = delete;
            ObjFileSystem(ObjFileSystem&&) = delete;
            ObjFileSystem& operator=(ObjFileSystem&&) = delete;

            [[nodiscard]] static fastObjCallbacks GetCallbacks() noexcept
            {
                fastObjCallbacks oCallbacks{};
                oCallbacks.file_open = &ObjFileSystem::Open;
                oCallbacks.file_close = &ObjFileSystem::Close;
                oCallbacks.file_read = &ObjFileSystem::Read;
                oCallbacks.file_size = &ObjFileSystem::Size;
                return oCallbacks;
            }

        private:
            struct OpenFile
            {
                std::vector<std::byte> vBytes;
                size_t uPosition = 0;
            };

            static void* Open(const char* szPath, void* pUserData) noexcept
            {
                try
                {
                    auto& oFileSystem = *static_cast<ObjFileSystem*>(pUserData);
                    const std::string_view svPath(szPath);
                    const std::filesystem::path oPath(std::u8string(svPath.begin(), svPath.end()));
                    auto upFile = std::make_unique<OpenFile>();
                    upFile->vBytes = Core::FileBytes::Read(oPath);
                    OpenFile* pFile = upFile.get();
                    oFileSystem.m_vFiles.push_back(std::move(upFile));
                    return pFile;
                }
                catch (...)
                {
                    return nullptr;
                }
            }

            static void Close(void* pFile, void* pUserData) noexcept
            {
                auto& oFileSystem = *static_cast<ObjFileSystem*>(pUserData);
                std::erase_if(oFileSystem.m_vFiles, [pFile](const std::unique_ptr<OpenFile>& upFile) { return upFile.get() == pFile; });
            }

            static size_t Read(void* pFile, void* pDestination, size_t uBytes, void* pUserData) noexcept
            {
                static_cast<void>(pUserData);
                auto& oFile = *static_cast<OpenFile*>(pFile);
                const size_t uCount = std::min(uBytes, oFile.vBytes.size() - oFile.uPosition);
                if (uCount != 0)
                {
                    std::memcpy(pDestination, oFile.vBytes.data() + oFile.uPosition, uCount);
                }
                oFile.uPosition += uCount;
                return uCount;
            }

            static unsigned long Size(void* pFile, void* pUserData) noexcept
            {
                static_cast<void>(pUserData);
                const auto& oFile = *static_cast<const OpenFile*>(pFile);
                return static_cast<unsigned long>(std::min<size_t>(oFile.vBytes.size(), std::numeric_limits<unsigned long>::max()));
            }

            std::vector<std::unique_ptr<OpenFile>> m_vFiles;
        };

        struct FastObjMeshDeleter
        {
            void operator()(fastObjMesh* pMesh) const noexcept
            {
                fast_obj_destroy(pMesh);
            }
        };

        struct ObjVertexKey
        {
            uint32_t uPosition = 0;
            uint32_t uTexCoord = 0;
            uint32_t uNormal = 0;

            [[nodiscard]] bool operator==(const ObjVertexKey& oOther) const noexcept = default;
        };

        struct ObjVertexKeyHash
        {
            [[nodiscard]] size_t operator()(const ObjVertexKey& oKey) const noexcept
            {
                const uint64_t uHigh = (static_cast<uint64_t>(oKey.uPosition) << 32u) | oKey.uTexCoord;
                return std::hash<uint64_t>{}(uHigh) ^ (std::hash<uint32_t>{}(oKey.uNormal) * 0x9e3779b97f4a7c15ull);
            }
        };

        class ObjMeshBuilder final
        {
        public:
            ObjMeshBuilder(const fastObjMesh& oSource, std::string sName, std::vector<std::string>& vWarnings)
                : m_oSource(oSource)
                , m_vWarnings(vWarnings)
                , m_bHasTexCoords(oSource.texcoord_count > 1)
                , m_bHasColors(oSource.color_count > 1 && oSource.color_count == oSource.position_count)
            {
                m_oMesh.sName = std::move(sName);
            }

            [[nodiscard]] Assets::MeshData Build()
            {
                const uint32_t uMaterialCount = std::max(m_oSource.material_count, 1u);
                std::vector<std::vector<uint32_t>> vMaterialIndices(uMaterialCount);
                uint32_t uSkippedFaces = 0;
                uint32_t uFirstCorner = 0;
                for (uint32_t uFace = 0; uFace < m_oSource.face_count; ++uFace)
                {
                    const uint32_t uCorners = m_oSource.face_vertices[uFace];
                    const uint32_t uMaterial = m_oSource.material_count == 0 ? 0 : m_oSource.face_materials[uFace];
                    if (uMaterial >= uMaterialCount)
                    {
                        throw std::runtime_error(std::format("face {} references material {} of {}", uFace, uMaterial, uMaterialCount));
                    }
                    if (uCorners < kMinimumFaceCorners)
                    {
                        ++uSkippedFaces;
                    }
                    else
                    {
                        AppendFace(uFirstCorner, uCorners, vMaterialIndices[uMaterial]);
                    }
                    uFirstCorner += uCorners;
                }

                if (uSkippedFaces != 0)
                {
                    m_vWarnings.push_back(std::format("{} point or line face(s) were skipped", uSkippedFaces));
                }
                AppendSections(vMaterialIndices);
                if (!m_bAllNormals)
                {
                    NormalGenerator::GenerateMissing(m_oMesh, m_vHasNormal);
                }
                return std::move(m_oMesh);
            }

        private:
            void AppendFace(uint32_t uFirstCorner, uint32_t uCorners, std::vector<uint32_t>& vIndices)
            {
                if (static_cast<uint64_t>(uFirstCorner) + uCorners > m_oSource.index_count)
                {
                    throw std::runtime_error("face corners exceed the index list");
                }
                const uint32_t uAnchor = ResolveVertex(m_oSource.indices[uFirstCorner]);
                uint32_t uPrevious = ResolveVertex(m_oSource.indices[uFirstCorner + 1]);
                for (uint32_t uCorner = 2; uCorner < uCorners; ++uCorner)
                {
                    const uint32_t uCurrent = ResolveVertex(m_oSource.indices[uFirstCorner + uCorner]);
                    vIndices.insert(vIndices.end(), {uAnchor, uPrevious, uCurrent});
                    uPrevious = uCurrent;
                }
            }

            uint32_t ResolveVertex(const fastObjIndex& oIndex)
            {
                if (oIndex.p == 0 || oIndex.p >= m_oSource.position_count)
                {
                    throw std::runtime_error(std::format("face references position {} of {}", oIndex.p, m_oSource.position_count - 1));
                }
                if (oIndex.t >= m_oSource.texcoord_count || oIndex.n >= m_oSource.normal_count)
                {
                    throw std::runtime_error(std::format("face references texture coordinate {} or normal {} out of range", oIndex.t, oIndex.n));
                }

                const ObjVertexKey oKey{oIndex.p, oIndex.t, oIndex.n};
                const auto itExisting = m_mapVertices.find(oKey);
                if (itExisting != m_mapVertices.end())
                {
                    return itExisting->second;
                }

                const auto uVertex = static_cast<uint32_t>(m_oMesh.vPositions.size());
                m_oMesh.vPositions.emplace_back(ReadVector3(m_oSource.positions, oIndex.p));
                m_oMesh.vNormals.push_back(oIndex.n == 0 ? glm::vec3(0.0f, 1.0f, 0.0f) : ReadVector3(m_oSource.normals, oIndex.n));
                m_vHasNormal.push_back(oIndex.n != 0);
                m_bAllNormals = m_bAllNormals && oIndex.n != 0;
                if (m_bHasTexCoords)
                {
                    const float* pTexCoord = m_oSource.texcoords + static_cast<size_t>(oIndex.t) * 2;
                    m_oMesh.vTexCoords0.emplace_back(pTexCoord[0], 1.0f - pTexCoord[1]);
                }
                if (m_bHasColors)
                {
                    m_oMesh.vColorsAbgr.push_back(ImportMath::PackColorAbgr(glm::vec4(ReadVector3(m_oSource.colors, oIndex.p), 1.0f)));
                }
                m_mapVertices.emplace(oKey, uVertex);
                return uVertex;
            }

            void AppendSections(const std::vector<std::vector<uint32_t>>& vMaterialIndices)
            {
                for (uint32_t uMaterial = 0; uMaterial < vMaterialIndices.size(); ++uMaterial)
                {
                    const std::vector<uint32_t>& vIndices = vMaterialIndices[uMaterial];
                    if (vIndices.empty())
                    {
                        continue;
                    }

                    Assets::MeshSection oSection;
                    oSection.sMaterialName = GetMaterialName(uMaterial);
                    oSection.uFirstIndex = static_cast<uint32_t>(m_oMesh.vIndices.size());
                    oSection.uIndexCount = static_cast<uint32_t>(vIndices.size());
                    m_oMesh.vIndices.insert(m_oMesh.vIndices.end(), vIndices.begin(), vIndices.end());
                    m_oMesh.vSections.push_back(std::move(oSection));
                }
            }

            [[nodiscard]] std::string GetMaterialName(uint32_t uMaterial) const
            {
                if (m_oSource.material_count == 0 || m_oSource.materials[uMaterial].name == nullptr)
                {
                    return std::string(Assets::MeshData::kDefaultSectionName);
                }
                return std::string(m_oSource.materials[uMaterial].name);
            }

            [[nodiscard]] static glm::vec3 ReadVector3(const float* pValues, uint32_t uIndex) noexcept
            {
                const float* pValue = pValues + static_cast<size_t>(uIndex) * 3;
                return {pValue[0], pValue[1], pValue[2]};
            }

            const fastObjMesh& m_oSource;
            std::vector<std::string>& m_vWarnings;
            bool m_bHasTexCoords = false;
            bool m_bHasColors = false;
            bool m_bAllNormals = true;
            Assets::MeshData m_oMesh;
            std::vector<bool> m_vHasNormal;
            std::unordered_map<ObjVertexKey, uint32_t, ObjVertexKeyHash> m_mapVertices;
        };
    }

    std::string_view ObjModelReader::GetFormatName() const noexcept
    {
        return kFormatName;
    }

    std::span<const std::string_view> ObjModelReader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    ImportedScene ObjModelReader::Read(const std::filesystem::path& oPath, const Assets::AssetImportOptions& oOptions)
    {
        static_cast<void>(oOptions);
        const std::string sDisplayPath = PathText::ToUtf8(oPath);

        ObjFileSystem oFileSystem;
        const fastObjCallbacks oCallbacks = ObjFileSystem::GetCallbacks();
        const std::unique_ptr<fastObjMesh, FastObjMeshDeleter> upSource(fast_obj_read_with_callbacks(sDisplayPath.c_str(), &oCallbacks, &oFileSystem));
        if (!upSource)
        {
            throw std::runtime_error(std::format("OBJ file '{}' could not be read", sDisplayPath));
        }

        ImportedScene oScene;
        try
        {
            Assets::MeshData oMesh = ObjMeshBuilder(*upSource, PathText::GetStem(oPath), oScene.vWarnings).Build();
            if (oMesh.vIndices.empty())
            {
                oScene.vWarnings.push_back("the file contains no triangles; no mesh was imported");
            }
            else
            {
                oScene.vMeshes.push_back(std::move(oMesh));
            }
        }
        catch (const std::runtime_error& oError)
        {
            throw std::runtime_error(std::format("OBJ file '{}' is invalid: {}", sDisplayPath, oError.what()));
        }
        return oScene;
    }
}
