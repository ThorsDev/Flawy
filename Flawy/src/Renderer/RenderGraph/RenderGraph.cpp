#include "Flawy/Renderer/RenderGraph/RenderGraph.hpp"

#include "Flawy/Renderer/RenderDevice.hpp"
#include "Flawy/Renderer/RenderGraph/IRenderPass.hpp"
#include "Renderer/Bgfx/BgfxResourceImpl.hpp"
#include "Renderer/RenderGraph/RenderGraphImpl.hpp"

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <format>
#include <limits>
#include <stdexcept>

namespace Flawy::Renderer
{
    namespace
    {
        uint16_t ToClearFlags(const RenderPassClear& oClear) noexcept
        {
            uint16_t uFlags = BGFX_CLEAR_NONE;
            if (oClear.bClearColor)
            {
                uFlags |= BGFX_CLEAR_COLOR;
            }
            if (oClear.bClearDepth)
            {
                uFlags |= BGFX_CLEAR_DEPTH;
            }
            if (oClear.bClearStencil)
            {
                uFlags |= BGFX_CLEAR_STENCIL;
            }
            return uFlags;
        }

        class ActivePassScope final
        {
        public:
            ActivePassScope(uint16_t& uActivePass, uint16_t uPassIndex) noexcept
                : m_uActivePass(uActivePass)
            {
                m_uActivePass = uPassIndex;
            }

            ~ActivePassScope()
            {
                m_uActivePass = RenderGraph::Impl::kNoPass;
            }

            ActivePassScope(const ActivePassScope&) = delete;
            ActivePassScope& operator=(const ActivePassScope&) = delete;
            ActivePassScope(ActivePassScope&&) = delete;
            ActivePassScope& operator=(ActivePassScope&&) = delete;

        private:
            uint16_t& m_uActivePass;
        };
    }

    std::string_view RenderGraph::Impl::PassRecord::GetName() const noexcept
    {
        return aName.data();
    }

    RenderGraph::Impl::Impl(RenderDevice& oRenderDevice) noexcept
        : oDevice(oRenderDevice)
        , oBuilder(*this)
        , oContext(*this)
    {
    }

    void RenderGraph::Impl::RequireFrameOpen(std::string_view svOperation) const
    {
        if (!bFrameOpen)
        {
            throw std::logic_error(std::format("RenderGraph::{} called outside of BeginFrame / Execute", svOperation));
        }
    }

    void RenderGraph::Impl::RequireNoActivePass(std::string_view svOperation) const
    {
        if (uActiveSetupPass != kNoPass)
        {
            throw std::logic_error(std::format("RenderGraph::{} called from inside IRenderPass::Setup of pass '{}'", svOperation, vPasses[uActiveSetupPass].GetName()));
        }
        if (uActiveExecutePass != kNoPass)
        {
            throw std::logic_error(std::format("RenderGraph::{} called from inside IRenderPass::Execute of pass '{}'", svOperation, vPasses[uActiveExecutePass].GetName()));
        }
    }

    uint16_t RenderGraph::Impl::RequireSetupPass(std::string_view svOperation) const
    {
        if (uActiveSetupPass == kNoPass)
        {
            throw std::logic_error(std::format("RenderPassBuilder::{} called outside of IRenderPass::Setup; the builder must not be retained", svOperation));
        }
        return uActiveSetupPass;
    }

    uint16_t RenderGraph::Impl::RequireExecutePass(std::string_view svOperation) const
    {
        if (uActiveExecutePass == kNoPass)
        {
            throw std::logic_error(std::format("RenderPassContext::{} called outside of IRenderPass::Execute; the context must not be retained", svOperation));
        }
        return uActiveExecutePass;
    }

    void RenderGraph::Impl::RequireValidResource(RenderGraphResource oResource) const
    {
        if (!oResource.IsValid())
        {
            throw std::invalid_argument("Render graph resource handle is invalid");
        }
        if (oResource != oBackbuffer && oResource.uGeneration != uFrameSerial)
        {
            throw std::logic_error(std::format("Render graph resource handle {} belongs to frame {} but the current frame is {}; only the backbuffer handle stays valid across frames",
                                               oResource.uIndex,
                                               oResource.uGeneration,
                                               uFrameSerial));
        }
        if (oResource.uIndex >= vResources.size())
        {
            throw std::invalid_argument(std::format("Render graph resource handle {} does not exist in the current frame", oResource.uIndex));
        }
    }

    void RenderGraph::Impl::RequireReadable(RenderGraphResource oResource, const PassRecord& oPass) const
    {
        RequireValidResource(oResource);
        if (oResource == oBackbuffer)
        {
            throw std::logic_error(std::format("Pass '{}' cannot read the backbuffer as a texture", oPass.GetName()));
        }
    }

    RenderGraphResource RenderGraph::Impl::AddResource(const ResourceRecord& oRecord)
    {
        if (vResources.size() >= RenderGraphResource::kInvalidIndex)
        {
            throw std::length_error("Render graph resource limit reached");
        }
        vResources.push_back(oRecord);
        return {static_cast<uint16_t>(vResources.size() - 1), uFrameSerial};
    }

    RenderGraphResource RenderGraph::Impl::CreateTransient(const RenderTargetDescriptor& oDescriptor)
    {
        const ResourceRecord& oBackbufferRecord = vResources[oBackbuffer.uIndex];
        ResourceRecord oRecord;
        oRecord.eKind = ResourceKind::Transient;
        oRecord.oDescriptor = oDescriptor;
        const bool bWidthFromBackbuffer = oRecord.oDescriptor.uWidth == 0;
        const bool bHeightFromBackbuffer = oRecord.oDescriptor.uHeight == 0;
        if (bWidthFromBackbuffer != bHeightFromBackbuffer)
        {
            throw std::invalid_argument(std::format("Transient target '{}' has size {}x{}; use 0x0 for the backbuffer size or give both dimensions",
                                                    oDescriptor.sDebugName,
                                                    oDescriptor.uWidth,
                                                    oDescriptor.uHeight));
        }
        if (bWidthFromBackbuffer)
        {
            oRecord.oDescriptor.uWidth = oBackbufferRecord.uWidth;
            oRecord.oDescriptor.uHeight = oBackbufferRecord.uHeight;
        }
        oRecord.uWidth = oRecord.oDescriptor.uWidth;
        oRecord.uHeight = oRecord.oDescriptor.uHeight;
        oRecord.bHasDepth = oRecord.oDescriptor.optDepthFormat.has_value();
        oRecord.bHasStencil = oRecord.bHasDepth && TextureFormatTraits::HasStencil(*oRecord.oDescriptor.optDepthFormat);
        return AddResource(oRecord);
    }

    std::string RenderGraph::Impl::DescribeResource(RenderGraphResource oResource) const
    {
        const ResourceRecord& oRecord = vResources[oResource.uIndex];
        switch (oRecord.eKind)
        {
        case ResourceKind::Backbuffer:
            return "Backbuffer";
        case ResourceKind::Imported:
            return std::format("imported target '{}'", oRecord.pTarget->GetDescriptor().sDebugName);
        case ResourceKind::Transient:
            return std::format("transient target '{}'", oRecord.oDescriptor.sDebugName);
        }
        return std::format("resource {}", oResource.uIndex);
    }

    bool RenderGraph::Impl::IsObserved(RenderGraphResource oResource) const noexcept
    {
        return vResources[oResource.uIndex].eKind != ResourceKind::Transient;
    }

    bool RenderGraph::Impl::LoadsPreviousContent(const PassRecord& oPass) const noexcept
    {
        const ResourceRecord& oTarget = vResources[oPass.oWrite.uIndex];
        return !oPass.oClear.bClearColor || (oTarget.bHasDepth && !oPass.oClear.bClearDepth) || (oTarget.bHasStencil && !oPass.oClear.bClearStencil);
    }

    bool RenderGraph::Impl::IsDeclaredRead(const PassRecord& oPass, RenderGraphResource oResource) const noexcept
    {
        for (uint32_t uRead = oPass.uReadBegin; uRead < oPass.uReadBegin + oPass.uReadCount; ++uRead)
        {
            if (vReads[uRead] == oResource)
            {
                return true;
            }
        }
        return false;
    }

    void RenderGraph::Impl::ValidateReads() const
    {
        for (size_t uPass = 0; uPass < vPasses.size(); ++uPass)
        {
            const PassRecord& oPass = vPasses[uPass];
            for (uint32_t uRead = oPass.uReadBegin; uRead < oPass.uReadBegin + oPass.uReadCount; ++uRead)
            {
                const RenderGraphResource oResource = vReads[uRead];
                if (oResource == oPass.oWrite)
                {
                    throw std::logic_error(std::format("Pass '{}' reads {} while writing it", oPass.GetName(), DescribeResource(oResource)));
                }
                if (vResources[oResource.uIndex].eKind == ResourceKind::Imported)
                {
                    continue;
                }

                bool bWritten = false;
                for (size_t uWriter = 0; uWriter < uPass && !bWritten; ++uWriter)
                {
                    bWritten = vPasses[uWriter].oWrite == oResource;
                }
                if (!bWritten)
                {
                    throw std::logic_error(std::format("Pass '{}' reads {} before it is written", oPass.GetName(), DescribeResource(oResource)));
                }
            }
        }
    }

    void RenderGraph::Impl::CullPasses()
    {
        for (PassRecord& oPass : vPasses)
        {
            oPass.bLive = oPass.bAlwaysExecute || IsObserved(oPass.oWrite);
        }

        for (size_t uPass = vPasses.size(); uPass-- > 0;)
        {
            const PassRecord& oPass = vPasses[uPass];
            if (!oPass.bLive)
            {
                continue;
            }
            for (uint32_t uRead = oPass.uReadBegin; uRead < oPass.uReadBegin + oPass.uReadCount; ++uRead)
            {
                MarkNearestEarlierWriterLive(uPass, vReads[uRead]);
            }
            if (LoadsPreviousContent(oPass))
            {
                MarkNearestEarlierWriterLive(uPass, oPass.oWrite);
            }
        }
    }

    void RenderGraph::Impl::MarkNearestEarlierWriterLive(size_t uPassIndex, RenderGraphResource oResource)
    {
        for (size_t uWriter = uPassIndex; uWriter-- > 0;)
        {
            if (vPasses[uWriter].oWrite == oResource)
            {
                vPasses[uWriter].bLive = true;
                return;
            }
        }
    }

    void RenderGraph::Impl::AcquireTransientTargets()
    {
        for (const PassRecord& oPass : vPasses)
        {
            ResourceRecord& oTarget = vResources[oPass.oWrite.uIndex];
            if (oPass.bLive && oTarget.eKind == ResourceKind::Transient && oTarget.pTarget == nullptr)
            {
                oTarget.pTarget = &oPool.Acquire(oTarget.oDescriptor);
            }
        }
    }

    uint16_t RenderGraph::Impl::AssignViewIds()
    {
        const uint32_t uMaxViews = bgfx::getCaps()->limits.maxViews;
        uint32_t uNextViewId = 0;
        for (PassRecord& oPass : vPasses)
        {
            if (!oPass.bLive)
            {
                continue;
            }
            if (uNextViewId + 1 >= uMaxViews)
            {
                throw std::length_error(std::format("Render graph needs more than {} bgfx views", uMaxViews - 1));
            }
            oPass.uViewId = static_cast<uint16_t>(uNextViewId++);
        }
        return static_cast<uint16_t>(uNextViewId);
    }

    void RenderGraph::Impl::PrepareView(const PassRecord& oPass) const
    {
        const ResourceRecord& oTarget = vResources[oPass.oWrite.uIndex];
        const bgfx::FrameBufferHandle hFrameBuffer = oTarget.pTarget != nullptr ? oTarget.pTarget->GetImpl().hFrameBuffer : bgfx::FrameBufferHandle{bgfx::kInvalidHandle};
        const glm::mat4 matIdentity(1.0f);
        const uint16_t uViewId = oPass.uViewId;

        bgfx::setViewName(uViewId, oPass.aName.data());
        bgfx::setViewMode(uViewId, bgfx::ViewMode::Sequential);
        bgfx::setViewFrameBuffer(uViewId, hFrameBuffer);
        bgfx::setViewRect(uViewId, 0, 0, static_cast<uint16_t>(oTarget.uWidth), static_cast<uint16_t>(oTarget.uHeight));
        bgfx::setViewClear(uViewId, ToClearFlags(oPass.oClear), oPass.oClear.uColorRgba, oPass.oClear.fDepth, oPass.oClear.uStencil);
        bgfx::setViewTransform(uViewId, glm::value_ptr(matIdentity), glm::value_ptr(matIdentity));
        bgfx::touch(uViewId);
    }

    void RenderGraph::Impl::ClearIdleBackbuffer(uint16_t uViewId) const
    {
        const ResourceRecord& oTarget = vResources[oBackbuffer.uIndex];
        bgfx::setViewName(uViewId, "Idle");
        bgfx::setViewMode(uViewId, bgfx::ViewMode::Sequential);
        bgfx::setViewFrameBuffer(uViewId, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(uViewId, 0, 0, static_cast<uint16_t>(oTarget.uWidth), static_cast<uint16_t>(oTarget.uHeight));
        bgfx::setViewClear(uViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kIdleClearColorRgba, 1.0f, 0);
        bgfx::touch(uViewId);
    }

    void RenderGraph::Impl::ExecutePasses(uint16_t uIdleViewId)
    {
        bool bBackbufferWritten = false;
        for (uint16_t uPass = 0; uPass < vPasses.size(); ++uPass)
        {
            const PassRecord& oPass = vPasses[uPass];
            if (!oPass.bLive)
            {
                continue;
            }

            bBackbufferWritten = bBackbufferWritten || oPass.oWrite == oBackbuffer;
            PrepareView(oPass);
            const ActivePassScope oScope(uActiveExecutePass, uPass);
            oPass.pPass->Execute(oContext);
        }

        if (!bBackbufferWritten)
        {
            ClearIdleBackbuffer(uIdleViewId);
        }
    }

    RenderGraph::RenderGraph(RenderDevice& oDevice)
        : m_upImpl(std::make_unique<Impl>(oDevice))
    {
    }

    RenderGraph::~RenderGraph() = default;

    void RenderGraph::BeginFrame()
    {
        Impl& oImpl = *m_upImpl;
        oImpl.RequireNoActivePass("BeginFrame");

        ++oImpl.uFrameSerial;
        if (oImpl.uFrameSerial == RenderGraphResource::kPersistentGeneration)
        {
            ++oImpl.uFrameSerial;
        }

        oImpl.vPasses.clear();
        oImpl.vReads.clear();
        oImpl.vResources.clear();

        Impl::ResourceRecord oBackbufferRecord;
        oBackbufferRecord.eKind = Impl::ResourceKind::Backbuffer;
        oBackbufferRecord.uWidth = oImpl.oDevice.GetWidth();
        oBackbufferRecord.uHeight = oImpl.oDevice.GetHeight();
        oBackbufferRecord.bHasDepth = true;
        oImpl.vResources.push_back(oBackbufferRecord);
        oImpl.oBackbuffer = RenderGraphResource{0, RenderGraphResource::kPersistentGeneration};
        oImpl.bFrameOpen = true;
    }

    void RenderGraph::AddPass(IRenderPass& oPass)
    {
        Impl& oImpl = *m_upImpl;
        oImpl.RequireNoActivePass("AddPass");
        oImpl.RequireFrameOpen("AddPass");
        if (oImpl.vPasses.size() >= Impl::kNoPass)
        {
            throw std::length_error("Render graph pass limit reached");
        }

        const auto uPassIndex = static_cast<uint16_t>(oImpl.vPasses.size());
        const auto uReadBegin = static_cast<uint32_t>(oImpl.vReads.size());
        Impl::PassRecord& oRecord = oImpl.vPasses.emplace_back();
        oRecord.pPass = &oPass;
        oRecord.uReadBegin = uReadBegin;
        std::format_to_n(oRecord.aName.data(), Impl::kMaxPassNameLength, "Pass {}", uPassIndex);

        try
        {
            {
                const ActivePassScope oScope(oImpl.uActiveSetupPass, uPassIndex);
                oPass.Setup(oImpl.oBuilder);
            }
            if (!oImpl.vPasses[uPassIndex].oWrite.IsValid())
            {
                throw std::logic_error(std::format("Pass '{}' declares no target; call RenderPassBuilder::WriteTarget in Setup", oImpl.vPasses[uPassIndex].GetName()));
            }
        }
        catch (...)
        {
            oImpl.vReads.resize(uReadBegin);
            oImpl.vPasses.pop_back();
            throw;
        }
    }

    RenderGraphResource RenderGraph::GetBackbuffer() const noexcept
    {
        return RenderGraphResource{0, RenderGraphResource::kPersistentGeneration};
    }

    RenderGraphResource RenderGraph::ImportTarget(RenderTarget& oTarget)
    {
        Impl& oImpl = *m_upImpl;
        oImpl.RequireFrameOpen("ImportTarget");
        for (size_t uIndex = 0; uIndex < oImpl.vResources.size(); ++uIndex)
        {
            const Impl::ResourceRecord& oExisting = oImpl.vResources[uIndex];
            if (oExisting.eKind == Impl::ResourceKind::Imported && oExisting.pTarget == &oTarget)
            {
                return {static_cast<uint16_t>(uIndex), oImpl.uFrameSerial};
            }
        }

        Impl::ResourceRecord oRecord;
        oRecord.eKind = Impl::ResourceKind::Imported;
        oRecord.pTarget = &oTarget;
        oRecord.uWidth = oTarget.GetWidth();
        oRecord.uHeight = oTarget.GetHeight();
        oRecord.bHasDepth = oTarget.HasDepth();
        oRecord.bHasStencil = oRecord.bHasDepth && TextureFormatTraits::HasStencil(*oTarget.GetDescriptor().optDepthFormat);
        return oImpl.AddResource(oRecord);
    }

    RenderGraphResource RenderGraph::CreateTransientTarget(const RenderTargetDescriptor& oDescriptor)
    {
        Impl& oImpl = *m_upImpl;
        oImpl.RequireFrameOpen("CreateTransientTarget");
        return oImpl.CreateTransient(oDescriptor);
    }

    void RenderGraph::Execute()
    {
        Impl& oImpl = *m_upImpl;
        oImpl.RequireFrameOpen("Execute");
        oImpl.RequireNoActivePass("Execute");
        oImpl.bFrameOpen = false;

        try
        {
            oImpl.ValidateReads();
            oImpl.CullPasses();
            oImpl.AcquireTransientTargets();
            oImpl.ExecutePasses(oImpl.AssignViewIds());
        }
        catch (...)
        {
            oImpl.oPool.EndFrame();
            throw;
        }
        oImpl.oPool.EndFrame();
    }
}
