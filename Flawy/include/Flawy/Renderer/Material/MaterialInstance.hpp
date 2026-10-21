#pragma once

#include <glm/vec4.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

namespace Flawy::Renderer
{
    class Material;
    class Texture;
    struct SamplerState;

    struct MaterialParameterId
    {
        uint32_t uIndex = 0xffffffffu;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    class MaterialInstance final
    {
    public:
        struct Impl;

        explicit MaterialInstance(Material& oMaterial);
        ~MaterialInstance();

        MaterialInstance(const MaterialInstance&) = delete;
        MaterialInstance& operator=(const MaterialInstance&) = delete;
        MaterialInstance(MaterialInstance&&) = delete;
        MaterialInstance& operator=(MaterialInstance&&) = delete;

        [[nodiscard]] MaterialParameterId FindValue(std::string_view svName) const noexcept;
        [[nodiscard]] MaterialParameterId RequireValue(std::string_view svName) const;
        [[nodiscard]] MaterialParameterId FindTexture(std::string_view svName) const noexcept;
        [[nodiscard]] MaterialParameterId RequireTexture(std::string_view svName) const;

        void SetFloat(MaterialParameterId oId, float fValue);
        void SetVector(MaterialParameterId oId, const glm::vec4& vecValue);
        void SetTexture(MaterialParameterId oId, const Texture& oTexture);
        void SetTexture(MaterialParameterId oId, const Texture& oTexture, const SamplerState& oSampler);
        void ClearTexture(MaterialParameterId oId);
        void SetStaticSwitch(std::string_view svName, bool bValue);

        [[nodiscard]] Material& GetMaterial() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
