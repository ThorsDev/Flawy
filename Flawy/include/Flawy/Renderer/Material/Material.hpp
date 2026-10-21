#pragma once

#include "Flawy/Renderer/Material/MaterialCompiler.hpp"
#include "Flawy/Renderer/Material/MaterialDescriptor.hpp"
#include "Flawy/Renderer/Material/MaterialGraph.hpp"
#include "Flawy/Renderer/Material/MaterialParameterLayout.hpp"
#include "Flawy/Renderer/Material/MaterialPass.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class MaterialSystem;
    class ShaderProgram;

    struct MaterialPermutation
    {
        uint32_t uIndex = 0;
        const ShaderProgram* pProgram = nullptr;
    };

    class Material final
    {
    public:
        Material(MaterialSystem& oSystem, MaterialDescriptor oDescriptor);
        Material(MaterialSystem& oSystem, MaterialGraph oGraph);
        ~Material();

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;
        Material(Material&&) = delete;
        Material& operator=(Material&&) = delete;

        [[nodiscard]] const MaterialPermutation& CompilePermutation(const MaterialPermutationKey& oKey);
        [[nodiscard]] const MaterialParameterLayout& GetParameterLayout() const noexcept;
        [[nodiscard]] std::string_view GetDebugName() const noexcept;
        [[nodiscard]] MaterialSystem& GetSystem() const noexcept;
        [[nodiscard]] uint32_t GetPermutationCount() const noexcept;
        [[nodiscard]] const MaterialGraph* GetGraph() const noexcept;
        [[nodiscard]] uint32_t ResolveStaticSwitchSet(std::span<const MaterialStaticSwitchValue> spanValues);
        [[nodiscard]] std::span<const std::string> GetStaticSwitchNames() const noexcept;
        [[nodiscard]] uint32_t GetStaticSwitchSetCount() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
