#include "Import/GltfSceneGraph.hpp"

#include "Import/ImportMath.hpp"

#include <glm/ext/quaternion_float.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <variant>

namespace Flawy::Import
{
    namespace
    {
        glm::mat4 ToMatrix(const fastgltf::math::fmat4x4& matSource) noexcept
        {
            glm::mat4 matResult(1.0f);
            for (glm::length_t iColumn = 0; iColumn < 4; ++iColumn)
            {
                for (glm::length_t iRow = 0; iRow < 4; ++iRow)
                {
                    matResult[iColumn][iRow] = matSource.col(static_cast<size_t>(iColumn))[static_cast<size_t>(iRow)];
                }
            }
            return matResult;
        }
    }

    GltfSceneGraph::GltfSceneGraph(const fastgltf::Asset& oAsset)
        : m_oAsset(oAsset)
    {
        BuildHierarchy();
        CollectSceneNodes();
    }

    std::optional<size_t> GltfSceneGraph::GetParent(size_t uNode) const
    {
        return m_vParents.at(uNode);
    }

    const glm::mat4& GltfSceneGraph::GetWorldMatrix(size_t uNode) const
    {
        return m_vWorld.at(uNode);
    }

    glm::mat4 GltfSceneGraph::GetParentWorldMatrix(size_t uNode) const
    {
        const std::optional<size_t> optParent = GetParent(uNode);
        return optParent ? m_vWorld[*optParent] : glm::mat4(1.0f);
    }

    std::span<const size_t> GltfSceneGraph::GetSceneNodes() const noexcept
    {
        return m_vSceneNodes;
    }

    Animation::JointTransform GltfSceneGraph::GetLocalTransform(const fastgltf::Node& oNode)
    {
        if (const auto* pTrs = std::get_if<fastgltf::TRS>(&oNode.transform))
        {
            Animation::JointTransform oTransform;
            oTransform.vecTranslation = {pTrs->translation.x(), pTrs->translation.y(), pTrs->translation.z()};
            oTransform.quatRotation = glm::quat(pTrs->rotation.w(), pTrs->rotation.x(), pTrs->rotation.y(), pTrs->rotation.z());
            oTransform.vecScale = {pTrs->scale.x(), pTrs->scale.y(), pTrs->scale.z()};
            return oTransform;
        }
        return ImportMath::DecomposeTransform(ToMatrix(std::get<fastgltf::math::fmat4x4>(oNode.transform)));
    }

    glm::mat4 GltfSceneGraph::GetLocalMatrix(const fastgltf::Node& oNode)
    {
        if (const auto* pMatrix = std::get_if<fastgltf::math::fmat4x4>(&oNode.transform))
        {
            return ToMatrix(*pMatrix);
        }
        return ImportMath::ComposeTransform(GetLocalTransform(oNode));
    }

    void GltfSceneGraph::BuildHierarchy()
    {
        const size_t uNodeCount = m_oAsset.nodes.size();
        m_vParents.assign(uNodeCount, std::nullopt);
        for (size_t uNode = 0; uNode < uNodeCount; ++uNode)
        {
            for (const size_t uChild : m_oAsset.nodes[uNode].children)
            {
                if (uChild >= uNodeCount || m_vParents[uChild])
                {
                    throw std::runtime_error(std::format("node {} has an invalid or second parent", uChild));
                }
                m_vParents[uChild] = uNode;
            }
        }

        m_vWorld.assign(uNodeCount, glm::mat4(1.0f));
        std::vector<bool> vVisited(uNodeCount, false);
        std::vector<size_t> vPending;
        for (size_t uNode = 0; uNode < uNodeCount; ++uNode)
        {
            if (!m_vParents[uNode])
            {
                vPending.push_back(uNode);
            }
        }
        while (!vPending.empty())
        {
            const size_t uNode = vPending.back();
            vPending.pop_back();
            vVisited[uNode] = true;
            m_vWorld[uNode] = GetParentWorldMatrix(uNode) * GetLocalMatrix(m_oAsset.nodes[uNode]);
            for (const size_t uChild : m_oAsset.nodes[uNode].children)
            {
                vPending.push_back(uChild);
            }
        }
        if (std::ranges::find(vVisited, false) != vVisited.end())
        {
            throw std::runtime_error("the node hierarchy contains a cycle");
        }
    }

    void GltfSceneGraph::CollectSceneNodes()
    {
        std::vector<size_t> vRoots;
        if (!m_oAsset.scenes.empty())
        {
            const size_t uScene = m_oAsset.defaultScene.value_or(0);
            if (uScene >= m_oAsset.scenes.size())
            {
                throw std::runtime_error(std::format("default scene {} does not exist", uScene));
            }
            for (const size_t uNode : m_oAsset.scenes[uScene].nodeIndices)
            {
                if (uNode >= m_oAsset.nodes.size())
                {
                    throw std::runtime_error(std::format("scene references missing node {}", uNode));
                }
                vRoots.push_back(uNode);
            }
        }
        else
        {
            for (size_t uNode = 0; uNode < m_oAsset.nodes.size(); ++uNode)
            {
                if (!m_vParents[uNode])
                {
                    vRoots.push_back(uNode);
                }
            }
        }

        std::vector<size_t> vPending(vRoots.rbegin(), vRoots.rend());
        while (!vPending.empty())
        {
            const size_t uNode = vPending.back();
            vPending.pop_back();
            m_vSceneNodes.push_back(uNode);
            const auto& vChildren = m_oAsset.nodes[uNode].children;
            for (auto itChild = vChildren.rbegin(); itChild != vChildren.rend(); ++itChild)
            {
                vPending.push_back(*itChild);
            }
        }
    }
}
