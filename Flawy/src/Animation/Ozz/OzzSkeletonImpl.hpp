#pragma once

#include "Flawy/Animation/JointTransform.hpp"
#include "Flawy/Animation/Skeleton.hpp"
#include "Flawy/Core/StringHash.hpp"

#include <glm/mat4x4.hpp>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Flawy::Animation
{
    struct Skeleton::Impl
    {
        std::string sName;
        ozz::unique_ptr<ozz::animation::Skeleton> upSkeleton;
        std::vector<std::string> vJointNames;
        std::vector<int32_t> vParentIndices;
        std::vector<JointTransform> vLocalRestPoses;
        std::vector<glm::mat4> vModelRestPoses;
        std::vector<glm::mat4> vInverseBindMatrices;
        std::unordered_map<std::string, uint32_t, Core::TransparentStringHash, std::equal_to<>> mapJoints;
    };
}
