#include "Renderer/RenderGraph/RenderTargetPool.hpp"

#include "Flawy/Core/Logger.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        std::string DescribeTarget(const RenderTargetDescriptor& oDescriptor)
        {
            std::string sDescription = std::format("'{}' {}x{} {}",
                                                   oDescriptor.sDebugName,
                                                   oDescriptor.uWidth,
                                                   oDescriptor.uHeight,
                                                   TextureFormatTraits::GetName(oDescriptor.eColorFormat));
            if (oDescriptor.optDepthFormat)
            {
                sDescription.append(std::format(" + {}", TextureFormatTraits::GetName(*oDescriptor.optDepthFormat)));
            }
            return sDescription;
        }

        void LogTargetEvent(std::string_view svEvent, const RenderTargetDescriptor& oDescriptor)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().info("{} transient render target {}", svEvent, DescribeTarget(oDescriptor));
            }
        }
    }

    RenderTarget& RenderTargetPool::Acquire(const RenderTargetDescriptor& oResolved)
    {
        for (Entry& oEntry : m_vEntries)
        {
            if (!oEntry.bInUse && oEntry.upTarget->GetDescriptor() == oResolved)
            {
                oEntry.bInUse = true;
                oEntry.uUnusedFrames = 0;
                return *oEntry.upTarget;
            }
        }

        Entry& oEntry = m_vEntries.emplace_back();
        try
        {
            oEntry.upTarget = std::make_unique<RenderTarget>(oResolved);
        }
        catch (...)
        {
            m_vEntries.pop_back();
            throw;
        }
        oEntry.bInUse = true;
        LogTargetEvent("Created", oResolved);
        return *oEntry.upTarget;
    }

    void RenderTargetPool::EndFrame()
    {
        for (Entry& oEntry : m_vEntries)
        {
            if (oEntry.bInUse)
            {
                oEntry.bInUse = false;
                continue;
            }
            ++oEntry.uUnusedFrames;
        }

        std::erase_if(m_vEntries, [](const Entry& oEntry) {
            const bool bExpired = oEntry.uUnusedFrames > kUnusedFrameLimit;
            if (bExpired)
            {
                LogTargetEvent("Released", oEntry.upTarget->GetDescriptor());
            }
            return bExpired;
        });
    }
}
