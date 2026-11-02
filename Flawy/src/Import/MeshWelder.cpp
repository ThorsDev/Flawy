#include "Import/MeshWelder.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Flawy::Import
{
    namespace
    {
        template <typename TElement>
        void AppendBytes(std::string& sKey, const std::vector<TElement>& vStream, size_t uVertex)
        {
            if (vStream.empty())
            {
                return;
            }
            const size_t uOffset = sKey.size();
            sKey.resize(uOffset + sizeof(TElement));
            std::memcpy(sKey.data() + uOffset, &vStream[uVertex], sizeof(TElement));
        }

        template <typename TElement>
        void CopyVertex(std::vector<TElement>& vTarget, const std::vector<TElement>& vSource, size_t uVertex)
        {
            if (!vSource.empty())
            {
                vTarget.push_back(vSource[uVertex]);
            }
        }
    }

    void MeshWelder::Weld(Assets::MeshData& oMesh)
    {
        const size_t uVertexCount = oMesh.vPositions.size();
        Assets::MeshData oWelded;
        oWelded.sName = std::move(oMesh.sName);
        oWelded.vSections = std::move(oMesh.vSections);

        std::unordered_map<std::string, uint32_t> mapVertices;
        mapVertices.reserve(uVertexCount);
        std::vector<uint32_t> vRemap(uVertexCount, 0);
        std::string sKey;
        for (size_t uVertex = 0; uVertex < uVertexCount; ++uVertex)
        {
            sKey.clear();
            AppendBytes(sKey, oMesh.vPositions, uVertex);
            AppendBytes(sKey, oMesh.vNormals, uVertex);
            AppendBytes(sKey, oMesh.vTangents, uVertex);
            AppendBytes(sKey, oMesh.vTexCoords0, uVertex);
            AppendBytes(sKey, oMesh.vColorsAbgr, uVertex);
            AppendBytes(sKey, oMesh.vJointIndices, uVertex);
            AppendBytes(sKey, oMesh.vJointWeights, uVertex);

            const auto [itVertex, bInserted] = mapVertices.try_emplace(sKey, static_cast<uint32_t>(oWelded.vPositions.size()));
            if (bInserted)
            {
                CopyVertex(oWelded.vPositions, oMesh.vPositions, uVertex);
                CopyVertex(oWelded.vNormals, oMesh.vNormals, uVertex);
                CopyVertex(oWelded.vTangents, oMesh.vTangents, uVertex);
                CopyVertex(oWelded.vTexCoords0, oMesh.vTexCoords0, uVertex);
                CopyVertex(oWelded.vColorsAbgr, oMesh.vColorsAbgr, uVertex);
                CopyVertex(oWelded.vJointIndices, oMesh.vJointIndices, uVertex);
                CopyVertex(oWelded.vJointWeights, oMesh.vJointWeights, uVertex);
            }
            vRemap[uVertex] = itVertex->second;
        }

        oWelded.vIndices.reserve(oMesh.vIndices.size());
        for (const uint32_t uIndex : oMesh.vIndices)
        {
            oWelded.vIndices.push_back(uIndex < uVertexCount ? vRemap[uIndex] : uIndex);
        }
        oMesh = std::move(oWelded);
    }
}
