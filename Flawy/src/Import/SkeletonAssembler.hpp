#pragma once

#include "Flawy/Animation/JointTransform.hpp"
#include "Flawy/Animation/SkeletonDescriptor.hpp"

#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Flawy::Import
{
    struct SkeletonSourceJoint
    {
        std::string sName;
        std::optional<uint32_t> optParent;
        glm::mat4 matPrefix{1.0f};
        Animation::JointTransform oLocalTransform;
        std::optional<glm::mat4> optInverseBindMatrix;
    };

    class SkeletonAssembler final
    {
    public:
        SkeletonAssembler(std::string sSkeletonName, std::vector<SkeletonSourceJoint> vSourceJoints, std::vector<std::string>& vWarnings);

        [[nodiscard]] const Animation::SkeletonDescriptor& GetDescriptor() const noexcept;
        [[nodiscard]] uint32_t GetJointCount() const noexcept;
        [[nodiscard]] uint32_t GetDescriptorIndex(uint32_t uSourceJoint) const;
        [[nodiscard]] const std::string& GetJointName(uint32_t uSourceJoint) const;
        [[nodiscard]] glm::vec3 ToJointTranslation(uint32_t uSourceJoint, const glm::vec3& vecTranslation) const;
        [[nodiscard]] glm::quat ToJointRotation(uint32_t uSourceJoint, const glm::quat& quatRotation) const;
        [[nodiscard]] glm::vec3 ToJointScale(uint32_t uSourceJoint, const glm::vec3& vecScale) const;

    private:
        void AssignUniqueNames();
        void BuildDescriptorOrder();
        void AppendSubtree(uint32_t uSourceJoint, std::optional<uint32_t> optParentDescriptor, const std::vector<std::vector<uint32_t>>& vChildren);
        [[nodiscard]] Animation::JointTransform ToJointTransform(uint32_t uSourceJoint, const Animation::JointTransform& oLocal) const;
        void RequireSourceJoint(uint32_t uSourceJoint) const;

        std::vector<SkeletonSourceJoint> m_vSourceJoints;
        std::vector<Animation::JointTransform> m_vPrefixes;
        std::vector<std::string> m_vJointNames;
        std::vector<uint32_t> m_vSourceToDescriptor;
        Animation::SkeletonDescriptor m_oDescriptor;
    };
}
