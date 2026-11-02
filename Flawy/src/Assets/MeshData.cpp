#include "Flawy/Assets/MeshData.hpp"

#include <cmath>
#include <format>
#include <stdexcept>

namespace Flawy::Assets
{
    namespace
    {
        template <typename TVector>
        bool IsFinite(const TVector& vecValue) noexcept
        {
            for (typename TVector::length_type iComponent = 0; iComponent < TVector::length(); ++iComponent)
            {
                if (!std::isfinite(vecValue[iComponent]))
                {
                    return false;
                }
            }
            return true;
        }

        class MeshDataValidator final
        {
        public:
            explicit MeshDataValidator(const MeshData& oMesh) noexcept
                : m_oMesh(oMesh)
            {
            }

            void Validate() const
            {
                ValidateName();
                ValidateVertices();
                ValidateIndices();
                ValidateSections();
            }

        private:
            [[noreturn]] void Fail(std::string_view svReason) const
            {
                throw std::invalid_argument(std::format("Mesh '{}': {}", m_oMesh.sName, svReason));
            }

            void ValidateName() const
            {
                if (m_oMesh.sName.size() > MeshData::kMaxNameLength)
                {
                    Fail(std::format("name is longer than {} bytes", MeshData::kMaxNameLength));
                }
            }

            template <typename TElement>
            void ValidateOptionalStream(const std::vector<TElement>& vStream, std::string_view svStream) const
            {
                if (!vStream.empty() && vStream.size() != m_oMesh.vPositions.size())
                {
                    Fail(std::format("{} stream has {} element(s) but the mesh has {} vertices", svStream, vStream.size(), m_oMesh.vPositions.size()));
                }
            }

            template <typename TVector>
            void ValidateFinite(const std::vector<TVector>& vStream, std::string_view svStream) const
            {
                for (size_t uIndex = 0; uIndex < vStream.size(); ++uIndex)
                {
                    if (!IsFinite(vStream[uIndex]))
                    {
                        Fail(std::format("{} of vertex {} is not finite", svStream, uIndex));
                    }
                }
            }

            void ValidateVertices() const
            {
                if (m_oMesh.vPositions.empty())
                {
                    Fail("mesh has no vertices");
                }
                if (m_oMesh.vPositions.size() > MeshData::kMaxVertexCount)
                {
                    Fail(std::format("mesh has {} vertices, more than the maximum of {}", m_oMesh.vPositions.size(), MeshData::kMaxVertexCount));
                }

                ValidateOptionalStream(m_oMesh.vNormals, "normal");
                ValidateOptionalStream(m_oMesh.vTangents, "tangent");
                ValidateOptionalStream(m_oMesh.vTexCoords0, "texcoord0");
                ValidateOptionalStream(m_oMesh.vColorsAbgr, "color");
                ValidateOptionalStream(m_oMesh.vJointIndices, "joint index");
                ValidateOptionalStream(m_oMesh.vJointWeights, "joint weight");
                if (m_oMesh.vJointIndices.empty() != m_oMesh.vJointWeights.empty())
                {
                    Fail("joint index and joint weight streams must both be present or both be absent");
                }

                ValidateFinite(m_oMesh.vPositions, "position");
                ValidateFinite(m_oMesh.vNormals, "normal");
                ValidateFinite(m_oMesh.vTangents, "tangent");
                ValidateFinite(m_oMesh.vTexCoords0, "texcoord0");
                ValidateFinite(m_oMesh.vJointWeights, "joint weight");
                for (size_t uIndex = 0; uIndex < m_oMesh.vJointWeights.size(); ++uIndex)
                {
                    const glm::vec4& vecWeights = m_oMesh.vJointWeights[uIndex];
                    if (vecWeights.x < 0.0f || vecWeights.y < 0.0f || vecWeights.z < 0.0f || vecWeights.w < 0.0f)
                    {
                        Fail(std::format("joint weight of vertex {} is negative", uIndex));
                    }
                }
            }

            void ValidateIndices() const
            {
                const std::vector<uint32_t>& vIndices = m_oMesh.vIndices;
                if (vIndices.empty())
                {
                    Fail("mesh has no indices");
                }
                if (vIndices.size() > MeshData::kMaxIndexCount)
                {
                    Fail(std::format("mesh has {} indices, more than the maximum of {}", vIndices.size(), MeshData::kMaxIndexCount));
                }
                if (vIndices.size() % 3 != 0)
                {
                    Fail(std::format("index count {} is not a multiple of 3", vIndices.size()));
                }

                const uint32_t uVertexCount = m_oMesh.GetVertexCount();
                for (size_t uIndex = 0; uIndex < vIndices.size(); ++uIndex)
                {
                    if (vIndices[uIndex] >= uVertexCount)
                    {
                        Fail(std::format("index {} references vertex {} but the mesh has {} vertices", uIndex, vIndices[uIndex], uVertexCount));
                    }
                }
            }

            void ValidateSections() const
            {
                const std::vector<MeshSection>& vSections = m_oMesh.vSections;
                if (vSections.empty())
                {
                    return;
                }
                if (vSections.size() > MeshData::kMaxSectionCount)
                {
                    Fail(std::format("mesh has {} sections, more than the maximum of {}", vSections.size(), MeshData::kMaxSectionCount));
                }

                uint64_t uExpectedFirst = 0;
                for (size_t uSection = 0; uSection < vSections.size(); ++uSection)
                {
                    const MeshSection& oSection = vSections[uSection];
                    if (oSection.sMaterialName.size() > MeshData::kMaxNameLength)
                    {
                        Fail(std::format("section {} material name is longer than {} bytes", uSection, MeshData::kMaxNameLength));
                    }
                    if (oSection.uFirstIndex != uExpectedFirst)
                    {
                        Fail(std::format("section {} starts at index {} but must start at {}", uSection, oSection.uFirstIndex, uExpectedFirst));
                    }
                    if (oSection.uIndexCount == 0 || oSection.uIndexCount % 3 != 0)
                    {
                        Fail(std::format("section {} index count {} must be a positive multiple of 3", uSection, oSection.uIndexCount));
                    }
                    uExpectedFirst += oSection.uIndexCount;
                }

                if (uExpectedFirst != m_oMesh.vIndices.size())
                {
                    Fail(std::format("sections cover {} indices but the mesh has {}", uExpectedFirst, m_oMesh.vIndices.size()));
                }
            }

            const MeshData& m_oMesh;
        };
    }

    uint32_t MeshData::GetVertexCount() const noexcept
    {
        return static_cast<uint32_t>(vPositions.size());
    }

    bool MeshData::HasTangents() const noexcept
    {
        return !vTangents.empty();
    }

    bool MeshData::HasSkinning() const noexcept
    {
        return !vJointIndices.empty() && !vJointWeights.empty();
    }

    void MeshData::Validate() const
    {
        MeshDataValidator(*this).Validate();
    }
}
