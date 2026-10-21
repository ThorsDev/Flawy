#pragma once

#include "Flawy/Renderer/LightEnvironment.hpp"
#include "Flawy/Renderer/ViewParameters.hpp"
#include "Renderer/Material/MaterialShaderNames.hpp"

#include <bgfx/bgfx.h>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>

namespace Flawy::Renderer
{
    class ViewUniformSet final
    {
    public:
        static constexpr uint16_t kViewParamsCount = MaterialShaderNames::kViewParamsCount;
        static constexpr uint16_t kLightParamsCount = MaterialShaderNames::kLightParamsCount;
        static constexpr uint16_t kMaxPointLights = MaterialShaderNames::kMaxPointLights;

        ViewUniformSet();
        ~ViewUniformSet();

        ViewUniformSet(const ViewUniformSet&) = delete;
        ViewUniformSet& operator=(const ViewUniformSet&) = delete;
        ViewUniformSet(ViewUniformSet&&) = delete;
        ViewUniformSet& operator=(ViewUniformSet&&) = delete;

        void SetView(const ViewParameters& oView, uint32_t uWidth, uint32_t uHeight) noexcept;
        void SetLights(const LightEnvironment& oLights);
        void Reset() noexcept;
        void Apply() const noexcept;

    private:
        bgfx::UniformHandle m_hViewParams = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_hLightParams = BGFX_INVALID_HANDLE;
        std::array<glm::vec4, kViewParamsCount> m_aViewParams{};
        std::array<glm::vec4, kLightParamsCount> m_aLightParams{};
    };
}
