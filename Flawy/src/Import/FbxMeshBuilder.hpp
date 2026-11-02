#pragma once

#include "Flawy/Assets/MeshData.hpp"

#include <ufbx.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Flawy::Import
{
    class FbxSkinBinding;

    class FbxMeshBuilder final
    {
    public:
        FbxMeshBuilder(const FbxSkinBinding& oSkinBinding, std::vector<std::string>& vWarnings);

        [[nodiscard]] std::optional<Assets::MeshData> Build(const ufbx_node& oNode);

        [[nodiscard]] size_t GetCornerVertexCount() const noexcept;

    private:
        void AppendFace(const ufbx_mesh& oSource, const ufbx_face& oFace, std::vector<uint32_t>& vTriangleCorners);
        void AppendInfluences(uint32_t uVertex);
        [[nodiscard]] static std::string GetMaterialName(const ufbx_node& oNode, const ufbx_mesh& oSource, uint32_t uMaterial);

        const FbxSkinBinding& m_oSkinBinding;
        std::vector<std::string>& m_vWarnings;
        Assets::MeshData m_oMesh;
        const ufbx_skin_deformer* m_pSkin = nullptr;
        bool m_bHasTangents = false;
        bool m_bSkinRepresentable = true;
        size_t m_uCornerVertexCount = 0;
    };
}
