#pragma once

#include <glm/mat4x4.hpp>

#include <memory>
#include <span>

namespace Flawy::Animation
{
    class LocalPose;
    class Skeleton;

    class ModelPose final
    {
    public:
        explicit ModelPose(const Skeleton& oSkeleton);
        ~ModelPose();

        ModelPose(const ModelPose&) = delete;
        ModelPose& operator=(const ModelPose&) = delete;
        ModelPose(ModelPose&&) = delete;
        ModelPose& operator=(ModelPose&&) = delete;

        void Update(const LocalPose& oLocalPose);
        [[nodiscard]] std::span<const glm::mat4> GetJointMatrices() const noexcept;
        [[nodiscard]] std::span<const glm::mat4> GetSkinningMatrices() const noexcept;
        [[nodiscard]] const Skeleton& GetSkeleton() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
