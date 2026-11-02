#include "Import/SkeletonAssembler.hpp"

#include "Import/ImportMath.hpp"

#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr uint32_t kUnassigned = std::numeric_limits<uint32_t>::max();
        constexpr std::string_view kFallbackJointName = "Joint";
    }

    SkeletonAssembler::SkeletonAssembler(std::string sSkeletonName, std::vector<SkeletonSourceJoint> vSourceJoints, std::vector<std::string>& vWarnings)
        : m_vSourceJoints(std::move(vSourceJoints))
    {
        m_oDescriptor.sName = std::move(sSkeletonName);
        m_vPrefixes.reserve(m_vSourceJoints.size());
        for (const SkeletonSourceJoint& oJoint : m_vSourceJoints)
        {
            if (oJoint.optParent && *oJoint.optParent >= m_vSourceJoints.size())
            {
                throw std::invalid_argument(std::format("Skeleton '{}': joint '{}' references parent {} outside the joint list",
                                                        m_oDescriptor.sName,
                                                        oJoint.sName,
                                                        *oJoint.optParent));
            }
            Animation::JointTransform oPrefix = ImportMath::DecomposeTransform(oJoint.matPrefix);
            if (!ImportMath::HasUniformScale(oPrefix.vecScale))
            {
                vWarnings.push_back(std::format("Skeleton '{}': joint '{}' inherits a non-uniform scale from non-joint ancestors; its transforms are approximated",
                                                m_oDescriptor.sName,
                                                oJoint.sName));
            }
            m_vPrefixes.push_back(oPrefix);
        }

        AssignUniqueNames();
        BuildDescriptorOrder();
    }

    const Animation::SkeletonDescriptor& SkeletonAssembler::GetDescriptor() const noexcept
    {
        return m_oDescriptor;
    }

    uint32_t SkeletonAssembler::GetJointCount() const noexcept
    {
        return static_cast<uint32_t>(m_vSourceJoints.size());
    }

    uint32_t SkeletonAssembler::GetDescriptorIndex(uint32_t uSourceJoint) const
    {
        RequireSourceJoint(uSourceJoint);
        return m_vSourceToDescriptor[uSourceJoint];
    }

    const std::string& SkeletonAssembler::GetJointName(uint32_t uSourceJoint) const
    {
        RequireSourceJoint(uSourceJoint);
        return m_vJointNames[uSourceJoint];
    }

    glm::vec3 SkeletonAssembler::ToJointTranslation(uint32_t uSourceJoint, const glm::vec3& vecTranslation) const
    {
        RequireSourceJoint(uSourceJoint);
        const Animation::JointTransform& oPrefix = m_vPrefixes[uSourceJoint];
        return oPrefix.vecTranslation + oPrefix.quatRotation * (oPrefix.vecScale * vecTranslation);
    }

    glm::quat SkeletonAssembler::ToJointRotation(uint32_t uSourceJoint, const glm::quat& quatRotation) const
    {
        RequireSourceJoint(uSourceJoint);
        return glm::normalize(m_vPrefixes[uSourceJoint].quatRotation * quatRotation);
    }

    glm::vec3 SkeletonAssembler::ToJointScale(uint32_t uSourceJoint, const glm::vec3& vecScale) const
    {
        RequireSourceJoint(uSourceJoint);
        return m_vPrefixes[uSourceJoint].vecScale * vecScale;
    }

    void SkeletonAssembler::AssignUniqueNames()
    {
        std::unordered_set<std::string> setUsed;
        m_vJointNames.reserve(m_vSourceJoints.size());
        for (size_t uJoint = 0; uJoint < m_vSourceJoints.size(); ++uJoint)
        {
            const std::string& sSource = m_vSourceJoints[uJoint].sName;
            const std::string sBase = sSource.empty() ? std::format("{}{}", kFallbackJointName, uJoint) : sSource;
            std::string sName = sBase;
            for (uint32_t uSuffix = 2; setUsed.contains(sName); ++uSuffix)
            {
                sName = std::format("{}_{}", sBase, uSuffix);
            }
            setUsed.insert(sName);
            m_vJointNames.push_back(std::move(sName));
        }
    }

    void SkeletonAssembler::BuildDescriptorOrder()
    {
        std::vector<std::vector<uint32_t>> vChildren(m_vSourceJoints.size());
        std::vector<uint32_t> vRoots;
        for (uint32_t uJoint = 0; uJoint < m_vSourceJoints.size(); ++uJoint)
        {
            const std::optional<uint32_t>& optParent = m_vSourceJoints[uJoint].optParent;
            if (optParent)
            {
                vChildren[*optParent].push_back(uJoint);
            }
            else
            {
                vRoots.push_back(uJoint);
            }
        }

        m_vSourceToDescriptor.assign(m_vSourceJoints.size(), kUnassigned);
        for (const uint32_t uRoot : vRoots)
        {
            AppendSubtree(uRoot, std::nullopt, vChildren);
        }

        const auto itUnreached = std::ranges::find(m_vSourceToDescriptor, kUnassigned);
        if (itUnreached != m_vSourceToDescriptor.end())
        {
            const size_t uJoint = static_cast<size_t>(itUnreached - m_vSourceToDescriptor.begin());
            throw std::invalid_argument(std::format("Skeleton '{}': joint '{}' is part of a parent cycle", m_oDescriptor.sName, m_vJointNames[uJoint]));
        }
    }

    void SkeletonAssembler::AppendSubtree(uint32_t uSourceJoint, std::optional<uint32_t> optParentDescriptor, const std::vector<std::vector<uint32_t>>& vChildren)
    {
        std::vector<std::pair<uint32_t, std::optional<uint32_t>>> vPending = {{uSourceJoint, optParentDescriptor}};
        while (!vPending.empty())
        {
            const auto [uJoint, optParent] = vPending.back();
            vPending.pop_back();

            const SkeletonSourceJoint& oSource = m_vSourceJoints[uJoint];
            const int32_t iParent = optParent ? static_cast<int32_t>(*optParent) : Animation::SkeletonDescriptor::kNoParent;
            const uint32_t uDescriptor = m_oDescriptor.AddJoint(m_vJointNames[uJoint], iParent, ToJointTransform(uJoint, oSource.oLocalTransform));
            m_oDescriptor.vJoints[uDescriptor].optInverseBindMatrix = oSource.optInverseBindMatrix;
            m_vSourceToDescriptor[uJoint] = uDescriptor;

            const std::vector<uint32_t>& vJointChildren = vChildren[uJoint];
            for (auto itChild = vJointChildren.rbegin(); itChild != vJointChildren.rend(); ++itChild)
            {
                vPending.emplace_back(*itChild, uDescriptor);
            }
        }
    }

    Animation::JointTransform SkeletonAssembler::ToJointTransform(uint32_t uSourceJoint, const Animation::JointTransform& oLocal) const
    {
        Animation::JointTransform oTransform;
        oTransform.vecTranslation = ToJointTranslation(uSourceJoint, oLocal.vecTranslation);
        oTransform.quatRotation = ToJointRotation(uSourceJoint, oLocal.quatRotation);
        oTransform.vecScale = ToJointScale(uSourceJoint, oLocal.vecScale);
        return oTransform;
    }

    void SkeletonAssembler::RequireSourceJoint(uint32_t uSourceJoint) const
    {
        if (uSourceJoint >= m_vSourceJoints.size())
        {
            throw std::out_of_range(std::format("Skeleton '{}': source joint {} is out of range ({} joints)", m_oDescriptor.sName, uSourceJoint, m_vSourceJoints.size()));
        }
    }
}
