#include "Flawy/Assets/MeshDataSerializer.hpp"

#include "Flawy/Assets/AssetFile.hpp"
#include "Flawy/Assets/StaticMeshAsset.hpp"
#include "Flawy/Core/BinaryFormatError.hpp"
#include "Flawy/Core/BinaryReader.hpp"
#include "Flawy/Core/BinaryWriter.hpp"
#include "Flawy/Core/FileBytes.hpp"

#include <format>
#include <stdexcept>

namespace Flawy::Assets
{
    namespace
    {
        enum class MeshStreamFlag : uint8_t
        {
            Normals = 1u << 0,
            Tangents = 1u << 1,
            TexCoords0 = 1u << 2,
            Colors = 1u << 3,
            Skinning = 1u << 4
        };

        constexpr uint8_t kKnownFlags = 0x1fu;

        constexpr uint8_t ToMask(MeshStreamFlag eFlag) noexcept
        {
            return static_cast<uint8_t>(eFlag);
        }

        class MeshPayloadWriter final
        {
        public:
            explicit MeshPayloadWriter(Core::BinaryWriter& oWriter) noexcept
                : m_oWriter(oWriter)
            {
            }

            void Write(const MeshData& oMesh)
            {
                m_oWriter.WriteString(oMesh.sName);
                m_oWriter.WriteU32(oMesh.GetVertexCount());
                m_oWriter.WriteU8(BuildFlags(oMesh));
                WriteVectors(oMesh.vPositions);
                WriteVectors(oMesh.vNormals);
                WriteVectors(oMesh.vTangents);
                WriteVectors(oMesh.vTexCoords0);
                for (const uint32_t uColor : oMesh.vColorsAbgr)
                {
                    m_oWriter.WriteU32(uColor);
                }
                for (const glm::u8vec4& vecJoints : oMesh.vJointIndices)
                {
                    m_oWriter.WriteU8(vecJoints.x);
                    m_oWriter.WriteU8(vecJoints.y);
                    m_oWriter.WriteU8(vecJoints.z);
                    m_oWriter.WriteU8(vecJoints.w);
                }
                WriteVectors(oMesh.vJointWeights);
                m_oWriter.WriteU32Array(oMesh.vIndices);
                m_oWriter.WriteU32(static_cast<uint32_t>(oMesh.vSections.size()));
                for (const MeshSection& oSection : oMesh.vSections)
                {
                    m_oWriter.WriteString(oSection.sMaterialName);
                    m_oWriter.WriteU32(oSection.uFirstIndex);
                    m_oWriter.WriteU32(oSection.uIndexCount);
                }
            }

        private:
            [[nodiscard]] static uint8_t BuildFlags(const MeshData& oMesh) noexcept
            {
                uint8_t uFlags = 0;
                AddFlag(uFlags, !oMesh.vNormals.empty(), MeshStreamFlag::Normals);
                AddFlag(uFlags, oMesh.HasTangents(), MeshStreamFlag::Tangents);
                AddFlag(uFlags, !oMesh.vTexCoords0.empty(), MeshStreamFlag::TexCoords0);
                AddFlag(uFlags, !oMesh.vColorsAbgr.empty(), MeshStreamFlag::Colors);
                AddFlag(uFlags, oMesh.HasSkinning(), MeshStreamFlag::Skinning);
                return uFlags;
            }

            static void AddFlag(uint8_t& uFlags, bool bPresent, MeshStreamFlag eFlag) noexcept
            {
                if (bPresent)
                {
                    uFlags = static_cast<uint8_t>(uFlags | ToMask(eFlag));
                }
            }

            template <typename TVector>
            void WriteVectors(const std::vector<TVector>& vVectors)
            {
                for (const TVector& vecValue : vVectors)
                {
                    for (typename TVector::length_type iComponent = 0; iComponent < TVector::length(); ++iComponent)
                    {
                        m_oWriter.WriteF32(vecValue[iComponent]);
                    }
                }
            }

            Core::BinaryWriter& m_oWriter;
        };

        class MeshPayloadReader final
        {
        public:
            explicit MeshPayloadReader(Core::BinaryReader& oReader) noexcept
                : m_oReader(oReader)
            {
            }

            [[nodiscard]] MeshData Read()
            {
                MeshData oMesh;
                oMesh.sName = m_oReader.ReadString(MeshData::kMaxNameLength);

                const size_t uVertexCountOffset = m_oReader.GetOffset();
                const uint32_t uVertexCount = m_oReader.ReadU32();
                if (uVertexCount == 0 || uVertexCount > MeshData::kMaxVertexCount)
                {
                    throw Core::BinaryFormatError(std::format("vertex count {} is outside [1, {}]", uVertexCount, MeshData::kMaxVertexCount), uVertexCountOffset);
                }

                const size_t uFlagsOffset = m_oReader.GetOffset();
                const uint8_t uFlags = m_oReader.ReadU8();
                if ((uFlags & ~kKnownFlags) != 0)
                {
                    throw Core::BinaryFormatError(std::format("unknown mesh stream flags 0x{:02x}", uFlags), uFlagsOffset);
                }

                ReadVectors(oMesh.vPositions, uVertexCount);
                ReadVectors(oMesh.vNormals, HasFlag(uFlags, MeshStreamFlag::Normals) ? uVertexCount : 0);
                ReadVectors(oMesh.vTangents, HasFlag(uFlags, MeshStreamFlag::Tangents) ? uVertexCount : 0);
                ReadVectors(oMesh.vTexCoords0, HasFlag(uFlags, MeshStreamFlag::TexCoords0) ? uVertexCount : 0);
                ReadColors(oMesh.vColorsAbgr, HasFlag(uFlags, MeshStreamFlag::Colors) ? uVertexCount : 0);
                const uint32_t uSkinnedCount = HasFlag(uFlags, MeshStreamFlag::Skinning) ? uVertexCount : 0;
                ReadJointIndices(oMesh.vJointIndices, uSkinnedCount);
                ReadVectors(oMesh.vJointWeights, uSkinnedCount);
                m_oReader.ReadU32Array(oMesh.vIndices, MeshData::kMaxIndexCount);
                ReadSections(oMesh.vSections);
                return oMesh;
            }

        private:
            [[nodiscard]] static bool HasFlag(uint8_t uFlags, MeshStreamFlag eFlag) noexcept
            {
                return (uFlags & ToMask(eFlag)) != 0;
            }

            template <typename TVector>
            void ReadVectors(std::vector<TVector>& vOut, uint32_t uCount)
            {
                const auto uComponents = static_cast<size_t>(TVector::length());
                m_oReader.RequireElements(uCount, uComponents * sizeof(float));
                vOut.resize(uCount);
                for (TVector& vecValue : vOut)
                {
                    for (typename TVector::length_type iComponent = 0; iComponent < TVector::length(); ++iComponent)
                    {
                        vecValue[iComponent] = m_oReader.ReadF32();
                    }
                }
            }

            void ReadColors(std::vector<uint32_t>& vOut, uint32_t uCount)
            {
                m_oReader.RequireElements(uCount, sizeof(uint32_t));
                vOut.resize(uCount);
                for (uint32_t& uColor : vOut)
                {
                    uColor = m_oReader.ReadU32();
                }
            }

            void ReadJointIndices(std::vector<glm::u8vec4>& vOut, uint32_t uCount)
            {
                m_oReader.RequireElements(uCount, 4);
                vOut.resize(uCount);
                for (glm::u8vec4& vecJoints : vOut)
                {
                    vecJoints.x = m_oReader.ReadU8();
                    vecJoints.y = m_oReader.ReadU8();
                    vecJoints.z = m_oReader.ReadU8();
                    vecJoints.w = m_oReader.ReadU8();
                }
            }

            void ReadSections(std::vector<MeshSection>& vOut)
            {
                const size_t uCountOffset = m_oReader.GetOffset();
                const uint32_t uCount = m_oReader.ReadU32();
                if (uCount > MeshData::kMaxSectionCount)
                {
                    throw Core::BinaryFormatError(std::format("section count {} exceeds the maximum of {}", uCount, MeshData::kMaxSectionCount), uCountOffset);
                }
                m_oReader.RequireElements(uCount, 12);
                vOut.resize(uCount);
                for (MeshSection& oSection : vOut)
                {
                    oSection.sMaterialName = m_oReader.ReadString(MeshData::kMaxNameLength);
                    oSection.uFirstIndex = m_oReader.ReadU32();
                    oSection.uIndexCount = m_oReader.ReadU32();
                }
            }

            Core::BinaryReader& m_oReader;
        };
    }

    std::vector<std::byte> MeshDataSerializer::Encode(const MeshData& oMesh)
    {
        oMesh.Validate();
        Core::BinaryWriter oWriter;
        MeshPayloadWriter(oWriter).Write(oMesh);
        return AssetFile::Encode(StaticMeshAsset::kTypeId, MeshData::kFormatVersion, oWriter.GetBytes());
    }

    MeshData MeshDataSerializer::Decode(std::span<const std::byte> spanFile)
    {
        const AssetFile::Contents oContents = AssetFile::Decode(spanFile, StaticMeshAsset::kTypeId);
        if (oContents.uFormatVersion != MeshData::kFormatVersion)
        {
            throw Core::BinaryFormatError(std::format("unsupported mesh format version {}, expected {}", oContents.uFormatVersion, MeshData::kFormatVersion),
                                          AssetFile::kFormatVersionOffset);
        }

        Core::BinaryReader oReader(oContents.spanPayload, AssetFile::kPayloadOffset);
        MeshData oMesh = MeshPayloadReader(oReader).Read();
        if (!oReader.IsAtEnd())
        {
            throw Core::BinaryFormatError(std::format("{} unexpected trailing byte(s) in the mesh payload", oReader.GetRemaining()), oReader.GetOffset());
        }

        try
        {
            oMesh.Validate();
        }
        catch (const std::invalid_argument& oError)
        {
            throw Core::BinaryFormatError(std::format("invalid mesh data: {}", oError.what()), AssetFile::kPayloadOffset);
        }
        return oMesh;
    }

    void MeshDataSerializer::Save(const std::filesystem::path& oPath, const MeshData& oMesh)
    {
        const std::vector<std::byte> vBytes = Encode(oMesh);
        Core::FileBytes::WriteAtomically(oPath, vBytes);
    }
}
