#pragma once

#include "Flawy/Renderer/RenderTarget.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace Flawy::Renderer
{
    class RenderTargetPool final
    {
    public:
        static constexpr uint32_t kUnusedFrameLimit = 4;

        [[nodiscard]] RenderTarget& Acquire(const RenderTargetDescriptor& oResolved);
        void EndFrame();

    private:
        struct Entry
        {
            std::unique_ptr<RenderTarget> upTarget;
            bool bInUse = false;
            uint32_t uUnusedFrames = 0;
        };

        std::vector<Entry> m_vEntries;
    };
}
