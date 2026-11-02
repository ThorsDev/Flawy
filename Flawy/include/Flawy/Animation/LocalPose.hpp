#pragma once

#include <memory>

namespace Flawy::Animation
{
    class Skeleton;

    class LocalPose final
    {
    public:
        struct Impl;

        explicit LocalPose(const Skeleton& oSkeleton);
        ~LocalPose();

        LocalPose(const LocalPose&) = delete;
        LocalPose& operator=(const LocalPose&) = delete;
        LocalPose(LocalPose&&) = delete;
        LocalPose& operator=(LocalPose&&) = delete;

        void ResetToRestPose() noexcept;
        [[nodiscard]] const Skeleton& GetSkeleton() const noexcept;
        [[nodiscard]] Impl& GetImpl() noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
