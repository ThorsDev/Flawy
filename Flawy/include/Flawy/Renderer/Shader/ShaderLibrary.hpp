#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    class ShaderCache;
    class ShaderCompiler;
    class ShaderProgram;

    struct ShaderLibrarySpecification
    {
        std::vector<std::filesystem::path> vSearchRoots;
    };

    class ShaderLibrary final
    {
    public:
        ShaderLibrary(ShaderCompiler& oCompiler, ShaderCache& oCache, const ShaderLibrarySpecification& oSpecification = {});
        ~ShaderLibrary();

        ShaderLibrary(const ShaderLibrary&) = delete;
        ShaderLibrary& operator=(const ShaderLibrary&) = delete;
        ShaderLibrary(ShaderLibrary&&) = delete;
        ShaderLibrary& operator=(ShaderLibrary&&) = delete;

        [[nodiscard]] ShaderProgram& GetProgram(std::string_view svName);
        [[nodiscard]] uint32_t GetProgramCount() const noexcept;
        [[nodiscard]] std::span<const std::filesystem::path> GetSearchRoots() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
