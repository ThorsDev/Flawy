#include "Renderer/Bgfx/BgfxRendererBackendMapping.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::array kDefaultPreference = {bgfx::RendererType::Direct3D11, bgfx::RendererType::Vulkan};

        class SupportedRendererList final
        {
        public:
            SupportedRendererList()
                : m_uCount(bgfx::getSupportedRenderers(static_cast<uint8_t>(bgfx::RendererType::Count), m_aRendererTypes.data()))
            {
            }

            [[nodiscard]] bool Contains(bgfx::RendererType::Enum eRendererType) const noexcept
            {
                const std::span<const bgfx::RendererType::Enum> spanSupported(m_aRendererTypes.data(), m_uCount);
                return std::ranges::find(spanSupported, eRendererType) != spanSupported.end();
            }

        private:
            std::array<bgfx::RendererType::Enum, bgfx::RendererType::Count> m_aRendererTypes{};
            uint8_t m_uCount = 0;
        };

        bgfx::RendererType::Enum ToRendererType(RendererBackend eBackend)
        {
            switch (eBackend)
            {
            case RendererBackend::Direct3D11:
                return bgfx::RendererType::Direct3D11;
            case RendererBackend::Vulkan:
                return bgfx::RendererType::Vulkan;
            case RendererBackend::Default:
                break;
            }
            throw std::invalid_argument("RendererBackend::Default has no direct bgfx renderer type");
        }
    }

    bgfx::RendererType::Enum BgfxRendererBackendMapping::ResolveRendererType(RendererBackend eBackend)
    {
        const SupportedRendererList oSupported;

        if (eBackend != RendererBackend::Default)
        {
            const bgfx::RendererType::Enum eRendererType = ToRendererType(eBackend);
            if (!oSupported.Contains(eRendererType))
            {
                throw std::runtime_error("Renderer backend is not supported on this platform: " + std::string(bgfx::getRendererName(eRendererType)));
            }
            return eRendererType;
        }

        for (const bgfx::RendererType::Enum eRendererType : kDefaultPreference)
        {
            if (oSupported.Contains(eRendererType))
            {
                return eRendererType;
            }
        }

        throw std::runtime_error("None of the default renderer backends (Direct3D 11, Vulkan) is supported on this platform");
    }

    RendererBackend BgfxRendererBackendMapping::ToRendererBackend(bgfx::RendererType::Enum eRendererType)
    {
        switch (eRendererType)
        {
        case bgfx::RendererType::Direct3D11:
            return RendererBackend::Direct3D11;
        case bgfx::RendererType::Vulkan:
            return RendererBackend::Vulkan;
        default:
            break;
        }
        throw std::runtime_error("Unsupported active bgfx renderer: " + std::string(bgfx::getRendererName(eRendererType)));
    }
}
