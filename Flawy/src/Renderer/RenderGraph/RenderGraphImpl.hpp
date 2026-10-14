#pragma once

#include "Flawy/Renderer/RenderGraph/RenderGraph.hpp"
#include "Flawy/Renderer/RenderGraph/RenderGraphResource.hpp"
#include "Flawy/Renderer/RenderGraph/RenderPassBuilder.hpp"
#include "Flawy/Renderer/RenderGraph/RenderPassClear.hpp"
#include "Flawy/Renderer/RenderGraph/RenderPassContext.hpp"
#include "Flawy/Renderer/RenderTarget.hpp"
#include "Renderer/RenderGraph/RenderTargetPool.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    struct RenderGraph::Impl
    {
        static constexpr size_t kMaxPassNameLength = 31;
        static constexpr uint16_t kNoPass = 0xffffu;
        static constexpr uint32_t kIdleClearColorRgba = 0x000000ffu;

        enum class ResourceKind : uint8_t
        {
            Backbuffer,
            Imported,
            Transient
        };

        struct ResourceRecord
        {
            ResourceKind eKind = ResourceKind::Backbuffer;
            RenderTargetDescriptor oDescriptor;
            RenderTarget* pTarget = nullptr;
            uint32_t uWidth = 0;
            uint32_t uHeight = 0;
            bool bHasDepth = false;
            bool bHasStencil = false;
        };

        struct PassRecord
        {
            IRenderPass* pPass = nullptr;
            std::array<char, kMaxPassNameLength + 1> aName{};
            RenderGraphResource oWrite;
            RenderPassClear oClear;
            uint32_t uReadBegin = 0;
            uint32_t uReadCount = 0;
            bool bAlwaysExecute = false;
            bool bLive = false;
            uint16_t uViewId = 0;

            [[nodiscard]] std::string_view GetName() const noexcept;
        };

        explicit Impl(RenderDevice& oRenderDevice) noexcept;

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
        ~Impl() = default;

        void RequireFrameOpen(std::string_view svOperation) const;
        void RequireNoActivePass(std::string_view svOperation) const;
        [[nodiscard]] uint16_t RequireSetupPass(std::string_view svOperation) const;
        [[nodiscard]] uint16_t RequireExecutePass(std::string_view svOperation) const;
        void RequireValidResource(RenderGraphResource oResource) const;
        void RequireReadable(RenderGraphResource oResource, const PassRecord& oPass) const;

        [[nodiscard]] RenderGraphResource AddResource(const ResourceRecord& oRecord);
        [[nodiscard]] RenderGraphResource CreateTransient(const RenderTargetDescriptor& oDescriptor);
        [[nodiscard]] std::string DescribeResource(RenderGraphResource oResource) const;
        [[nodiscard]] bool IsObserved(RenderGraphResource oResource) const noexcept;
        [[nodiscard]] bool LoadsPreviousContent(const PassRecord& oPass) const noexcept;
        [[nodiscard]] bool IsDeclaredRead(const PassRecord& oPass, RenderGraphResource oResource) const noexcept;

        void ValidateReads() const;
        void CullPasses();
        void MarkNearestEarlierWriterLive(size_t uPassIndex, RenderGraphResource oResource);
        void AcquireTransientTargets();
        [[nodiscard]] uint16_t AssignViewIds();
        void PrepareView(const PassRecord& oPass) const;
        void ClearIdleBackbuffer(uint16_t uViewId) const;
        void ExecutePasses(uint16_t uIdleViewId);

        RenderDevice& oDevice;
        RenderTargetPool oPool;
        std::vector<PassRecord> vPasses;
        std::vector<RenderGraphResource> vReads;
        std::vector<ResourceRecord> vResources;
        RenderGraphResource oBackbuffer;
        uint32_t uFrameSerial = RenderGraphResource::kPersistentGeneration;
        bool bFrameOpen = false;
        uint16_t uActiveSetupPass = kNoPass;
        uint16_t uActiveExecutePass = kNoPass;
        RenderPassBuilder oBuilder;
        RenderPassContext oContext;
    };
}
