#pragma once

#include "Flawy/Animation/JointTransform.hpp"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Flawy::Animation
{
    struct JointDescriptor
    {
        static constexpr int32_t kNoParent = -1;

        std::string sName;
        int32_t iParentIndex = kNoParent;
        JointTransform oLocalRestPose;
        std::optional<glm::mat4> optInverseBindMatrix;
    };

    struct SkeletonDescriptor
    {
        static constexpr int32_t kNoParent = JointDescriptor::kNoParent;
        static constexpr uint32_t kMaxJoints = 1024;
        static constexpr uint32_t kMaxNameLength = 4096;

        std::string sName;
        std::vector<JointDescriptor> vJoints;

        uint32_t AddJoint(std::string sJointName, int32_t iParentIndex, const JointTransform& oLocalRestPose);
        void Validate() const;
    };
}
