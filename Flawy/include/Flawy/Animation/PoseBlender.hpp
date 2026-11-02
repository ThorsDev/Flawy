#pragma once

#include <cstdint>
#include <memory>
#include <span>

namespace Flawy::Animation
{
    class LocalPose;
    class Skeleton;

    struct BlendLayer
    {
        const LocalPose* pPose = nullptr;
        float fWeight = 0.0f;
    };

    class PoseBlender final
    {
    public:
        PoseBlender(const Skeleton& oSkeleton, uint32_t uMaxLayers);
        ~PoseBlender();

        PoseBlender(const PoseBlender&) = delete;
        PoseBlender& operator=(const PoseBlender&) = delete;
        PoseBlender(PoseBlender&&) = delete;
        PoseBlender& operator=(PoseBlender&&) = delete;

        void Blend(std::span<const BlendLayer> spanLayers, LocalPose& oOutput);
        [[nodiscard]] uint32_t GetMaxLayerCount() const noexcept;
        [[nodiscard]] const Skeleton& GetSkeleton() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
