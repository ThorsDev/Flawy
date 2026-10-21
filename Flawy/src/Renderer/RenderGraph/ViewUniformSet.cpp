#include "Renderer/RenderGraph/ViewUniformSet.hpp"

#include <glm/geometric.hpp>

#include <format>
#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        static_assert(LightEnvironment::kMaxPointLights == ViewUniformSet::kMaxPointLights, "LightEnvironment and View.slang must agree on the point light count");
        static_assert(4 + 2 * ViewUniformSet::kMaxPointLights == ViewUniformSet::kLightParamsCount, "u_lightParams layout must hold every point light");

        constexpr uint32_t kDirectionalSlot = 0;
        constexpr uint32_t kDirectionalRadianceSlot = 1;
        constexpr uint32_t kSkyAmbientSlot = 2;
        constexpr uint32_t kGroundAmbientSlot = 3;
        constexpr uint32_t kFirstPointLightSlot = 4;
        constexpr float kUnusedPointLightRadius = 1.0f;
        constexpr float kMinDirectionLength = 1.0e-6f;

        bgfx::UniformHandle CreateVec4Uniform(std::string_view svName, uint16_t uCount)
        {
            const std::string sName(svName);
            const bgfx::UniformHandle hUniform = bgfx::createUniform(sName.c_str(), bgfx::UniformType::Vec4, uCount);
            if (!bgfx::isValid(hUniform))
            {
                throw std::runtime_error(std::format("Failed to create engine uniform '{}'", svName));
            }
            return hUniform;
        }
    }

    ViewUniformSet::ViewUniformSet()
        : m_hViewParams(CreateVec4Uniform(MaterialShaderNames::kViewParamsUniform, kViewParamsCount))
    {
        try
        {
            m_hLightParams = CreateVec4Uniform(MaterialShaderNames::kLightParamsUniform, kLightParamsCount);
        }
        catch (...)
        {
            bgfx::destroy(m_hViewParams);
            throw;
        }
        Reset();
    }

    ViewUniformSet::~ViewUniformSet()
    {
        bgfx::destroy(m_hLightParams);
        bgfx::destroy(m_hViewParams);
    }

    void ViewUniformSet::SetView(const ViewParameters& oView, uint32_t uWidth, uint32_t uHeight) noexcept
    {
        const float fWidth = static_cast<float>(uWidth);
        const float fHeight = static_cast<float>(uHeight);
        m_aViewParams[0] = glm::vec4(oView.vecCameraPosition, oView.fTimeSeconds);
        m_aViewParams[1] = glm::vec4(fWidth, fHeight, uWidth != 0 ? 1.0f / fWidth : 0.0f, uHeight != 0 ? 1.0f / fHeight : 0.0f);
    }

    void ViewUniformSet::SetLights(const LightEnvironment& oLights)
    {
        if (oLights.vPointLights.size() > kMaxPointLights)
        {
            throw std::invalid_argument(std::format("LightEnvironment has {} point lights but at most {} are supported", oLights.vPointLights.size(), kMaxPointLights));
        }
        for (const PointLight& oLight : oLights.vPointLights)
        {
            if (!(oLight.fRadius > 0.0f))
            {
                throw std::invalid_argument("Point light radius must be greater than zero");
            }
        }

        const float fDirectionLength = oLights.optDirectional ? glm::length(oLights.optDirectional->vecDirection) : 1.0f;
        if (!(fDirectionLength >= kMinDirectionLength))
        {
            throw std::invalid_argument("Directional light direction must not be zero");
        }

        m_aLightParams.fill(glm::vec4(0.0f));
        if (oLights.optDirectional)
        {
            const DirectionalLight& oDirectional = *oLights.optDirectional;
            m_aLightParams[kDirectionalSlot] = glm::vec4(oDirectional.vecDirection / fDirectionLength, 1.0f);
            m_aLightParams[kDirectionalRadianceSlot] = glm::vec4(oDirectional.vecColor * oDirectional.fIntensity, 0.0f);
        }
        m_aLightParams[kSkyAmbientSlot] = glm::vec4(oLights.vecSkyAmbient, 0.0f);
        m_aLightParams[kGroundAmbientSlot] = glm::vec4(oLights.vecGroundAmbient, 0.0f);

        for (uint32_t uIndex = 0; uIndex < kMaxPointLights; ++uIndex)
        {
            const uint32_t uSlot = kFirstPointLightSlot + 2 * uIndex;
            if (uIndex < oLights.vPointLights.size())
            {
                const PointLight& oLight = oLights.vPointLights[uIndex];
                m_aLightParams[uSlot] = glm::vec4(oLight.vecPosition, oLight.fRadius);
                m_aLightParams[uSlot + 1] = glm::vec4(oLight.vecColor * oLight.fIntensity, 0.0f);
                continue;
            }
            m_aLightParams[uSlot] = glm::vec4(0.0f, 0.0f, 0.0f, kUnusedPointLightRadius);
        }
    }

    void ViewUniformSet::Reset() noexcept
    {
        m_aViewParams.fill(glm::vec4(0.0f));
        m_aLightParams.fill(glm::vec4(0.0f));
        for (uint32_t uIndex = 0; uIndex < kMaxPointLights; ++uIndex)
        {
            m_aLightParams[kFirstPointLightSlot + 2 * uIndex].w = kUnusedPointLightRadius;
        }
    }

    void ViewUniformSet::Apply() const noexcept
    {
        bgfx::setUniform(m_hViewParams, m_aViewParams.data(), kViewParamsCount);
        bgfx::setUniform(m_hLightParams, m_aLightParams.data(), kLightParamsCount);
    }
}
