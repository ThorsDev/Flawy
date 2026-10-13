#include "Renderer/Bgfx/BgfxCallbackAdapter.hpp"

#include "Flawy/Core/Logger.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr size_t kTraceBufferSize = 1024;

        std::string_view TrimTrailingNewlines(std::string_view svText) noexcept
        {
            while (!svText.empty() && (svText.back() == '\n' || svText.back() == '\r'))
            {
                svText.remove_suffix(1);
            }
            return svText;
        }
    }

    void BgfxCallbackAdapter::fatal(const char* szFilePath, uint16_t uLine, bgfx::Fatal::Enum eCode, const char* szMessage)
    {
        if (Core::Logger::IsInitialized())
        {
            auto& oLogger = Core::Logger::Get();
            oLogger.critical("bgfx fatal error {} at {}:{}: {}", static_cast<int>(eCode), szFilePath, uLine, szMessage);
            oLogger.flush();
        }
        std::abort();
    }

    void BgfxCallbackAdapter::traceVargs(const char* szFilePath, uint16_t uLine, const char* szFormat, va_list oArguments)
    {
        if (!Core::Logger::IsInitialized() || !Core::Logger::Get().should_log(spdlog::level::trace))
        {
            return;
        }

        std::array<char, kTraceBufferSize> aBuffer{};
        const int iWritten = std::vsnprintf(aBuffer.data(), aBuffer.size(), szFormat, oArguments);
        if (iWritten < 0)
        {
            return;
        }

        const std::string_view svMessage = TrimTrailingNewlines(aBuffer.data());
        Core::Logger::Get().trace("bgfx {}:{}: {}", szFilePath, uLine, svMessage);
    }

    void BgfxCallbackAdapter::profilerBegin(const char*, uint32_t, const char*, uint16_t)
    {
    }

    void BgfxCallbackAdapter::profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t)
    {
    }

    void BgfxCallbackAdapter::profilerEnd()
    {
    }

    uint32_t BgfxCallbackAdapter::cacheReadSize(uint64_t)
    {
        return 0;
    }

    bool BgfxCallbackAdapter::cacheRead(uint64_t, void*, uint32_t)
    {
        return false;
    }

    void BgfxCallbackAdapter::cacheWrite(uint64_t, const void*, uint32_t)
    {
    }

    void BgfxCallbackAdapter::screenShot(const char*, uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, const void*, uint32_t, bool)
    {
    }

    void BgfxCallbackAdapter::captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool)
    {
    }

    void BgfxCallbackAdapter::captureEnd()
    {
    }

    void BgfxCallbackAdapter::captureFrame(const void*, uint32_t)
    {
    }
}
