#pragma once

#include <bgfx/bgfx.h>

#include <cstdarg>
#include <cstdint>

namespace Flawy::Renderer
{
    class BgfxCallbackAdapter final : public bgfx::CallbackI
    {
    public:
        void fatal(const char* szFilePath, uint16_t uLine, bgfx::Fatal::Enum eCode, const char* szMessage) override;
        void traceVargs(const char* szFilePath, uint16_t uLine, const char* szFormat, va_list oArguments) override;
        void profilerBegin(const char* szName, uint32_t uAbgr, const char* szFilePath, uint16_t uLine) override;
        void profilerBeginLiteral(const char* szName, uint32_t uAbgr, const char* szFilePath, uint16_t uLine) override;
        void profilerEnd() override;
        uint32_t cacheReadSize(uint64_t uId) override;
        bool cacheRead(uint64_t uId, void* pData, uint32_t uSize) override;
        void cacheWrite(uint64_t uId, const void* pData, uint32_t uSize) override;
        void screenShot(const char* szFilePath,
                        uint32_t uWidth,
                        uint32_t uHeight,
                        uint32_t uPitch,
                        bgfx::TextureFormat::Enum eFormat,
                        const void* pData,
                        uint32_t uSize,
                        bool bYFlip) override;
        void captureBegin(uint32_t uWidth, uint32_t uHeight, uint32_t uPitch, bgfx::TextureFormat::Enum eFormat, bool bYFlip) override;
        void captureEnd() override;
        void captureFrame(const void* pData, uint32_t uSize) override;
    };
}
