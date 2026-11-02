#pragma once

#include "Flawy/Animation/JointTransform.hpp"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Flawy::Animation
{
    struct SkeletonDescriptor;

    class Skeleton final
    {
    public:
        struct Impl;

        explicit Skeleton(const SkeletonDescriptor& oDescriptor);
        ~Skeleton();

        Skeleton(const Skeleton&) = delete;
        Skeleton& operator=(const Skeleton&) = delete;
        Skeleton(Skeleton&&) = delete;
        Skeleton& operator=(Skeleton&&) = delete;

        [[nodiscard]] const std::string& GetName() const noexcept;
        [[nodiscard]] uint32_t GetJointCount() const noexcept;
        [[nodiscard]] const std::string& GetJointName(uint32_t uJoint) const;
        [[nodiscard]] int32_t GetJointParentIndex(uint32_t uJoint) const;
        [[nodiscard]] std::optional<uint32_t> FindJoint(std::string_view svName) const noexcept;
        [[nodiscard]] const JointTransform& GetLocalRestPose(uint32_t uJoint) const;
        [[nodiscard]] std::span<const glm::mat4> GetModelRestPose() const noexcept;
        [[nodiscard]] std::span<const glm::mat4> GetInverseBindMatrices() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        void RequireJoint(uint32_t uJoint) const;

        std::unique_ptr<Impl> m_upImpl;
    };
}
