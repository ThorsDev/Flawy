#pragma once

#include <cstddef>
#include <span>

namespace Flawy::Renderer
{
    struct EmbeddedShaderEntry
    {
        const char* szPath = nullptr;
        const unsigned char* pData = nullptr;
        size_t uSize = 0;
    };

    class EmbeddedShaderTable final
    {
    public:
        EmbeddedShaderTable() = delete;

        [[nodiscard]] static std::span<const EmbeddedShaderEntry> GetEntries() noexcept;
    };
}
