#include "Flawy/Renderer/Material/MaterialInstance.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Renderer/Material/MaterialSystem.hpp"
#include "Flawy/Renderer/Texture.hpp"
#include "Renderer/Material/MaterialInstanceImpl.hpp"
#include "Renderer/Material/MaterialShaderNames.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint32_t kInvalidParameterIndex = 0xffffffffu;

        template <typename TParameter> MaterialParameterId FindByName(const std::vector<TParameter>& vParameters, std::string_view svName) noexcept
        {
            const auto itParameter = std::ranges::find(vParameters, svName, &TParameter::sName);
            if (itParameter == vParameters.end())
            {
                return {};
            }
            return {static_cast<uint32_t>(std::distance(vParameters.begin(), itParameter))};
        }

        template <typename TParameter> const TParameter& RequireParameter(const std::vector<TParameter>& vParameters, MaterialParameterId oId, std::string_view svKind)
        {
            if (!oId.IsValid() || oId.uIndex >= vParameters.size())
            {
                throw std::invalid_argument(std::format("Invalid material {} parameter id", svKind));
            }
            return vParameters[oId.uIndex];
        }

        std::string_view ToTypeName(MaterialValueType eType) noexcept
        {
            switch (eType)
            {
            case MaterialValueType::Float1:
                return "Float1";
            case MaterialValueType::Float2:
                return "Float2";
            case MaterialValueType::Float3:
                return "Float3";
            case MaterialValueType::Float4:
                return "Float4";
            }
            return "Unknown";
        }

        void WarnOnColorSpaceMismatch(const Material& oMaterial, const MaterialTextureParameter& oParameter, const Texture& oTexture)
        {
            if (oTexture.IsSrgb() == oParameter.bSrgb || !Core::Logger::IsInitialized())
            {
                return;
            }
            Core::Logger::Get().warn("Material '{}' texture '{}' expects {} data but the bound texture is {}",
                                     oMaterial.GetDebugName(),
                                     oParameter.sName,
                                     oParameter.bSrgb ? "sRGB" : "linear",
                                     oTexture.IsSrgb() ? "sRGB" : "linear");
        }

        void WriteComponents(std::vector<glm::vec4>& vSlots, const MaterialValueParameter& oParameter, const glm::vec4& vecValue) noexcept
        {
            glm::vec4& vecSlot = vSlots[oParameter.uSlot];
            for (uint8_t uOffset = 0; uOffset < oParameter.GetComponentCount(); ++uOffset)
            {
                vecSlot[oParameter.uComponent + uOffset] = vecValue[uOffset];
            }
        }
    }

    bool MaterialParameterId::IsValid() const noexcept
    {
        return uIndex != kInvalidParameterIndex;
    }

    MaterialInstance::Impl::Impl(Material& oMaterial)
        : pMaterial(&oMaterial)
    {
        const MaterialParameterLayout& oLayout = oMaterial.GetParameterLayout();
        vSlots.assign(oLayout.uVec4SlotCount, glm::vec4(0.0f));
        for (const MaterialValueParameter& oParameter : oLayout.vValues)
        {
            WriteComponents(vSlots, oParameter, oParameter.vecDefault);
        }
        vTextures.resize(oLayout.vTextures.size());
        vTextureSamplerNames.reserve(oLayout.vTextures.size());
        for (const MaterialTextureParameter& oTexture : oLayout.vTextures)
        {
            vTextureSamplerNames.push_back(MaterialShaderNames::GetTextureSamplerName(oTexture.uStage));
        }
    }

    const ShaderParameters& MaterialInstance::Impl::AcquireParameters(const MaterialPermutation& oPermutation) const
    {
        if (vPermutationParameters.size() <= oPermutation.uIndex)
        {
            vPermutationParameters.resize(oPermutation.uIndex + 1u);
            vSynced.resize(oPermutation.uIndex + 1u, false);
        }

        std::unique_ptr<ShaderParameters>& upParameters = vPermutationParameters[oPermutation.uIndex];
        if (!upParameters)
        {
            upParameters = std::make_unique<ShaderParameters>(*oPermutation.pProgram);
            vSynced[oPermutation.uIndex] = false;
        }
        if (!vSynced[oPermutation.uIndex])
        {
            Synchronize(*upParameters);
            vSynced[oPermutation.uIndex] = true;
        }
        return *upParameters;
    }

    void MaterialInstance::Impl::MarkDirty() noexcept
    {
        std::fill(vSynced.begin(), vSynced.end(), false);
    }

    void MaterialInstance::Impl::Synchronize(ShaderParameters& oParameters) const
    {
        const ShaderParameterId oSlotsId = oParameters.Find(MaterialShaderNames::kMaterialParamsUniform);
        if (oSlotsId.IsValid() && !vSlots.empty())
        {
            oParameters.SetVec4Array(oSlotsId, vSlots);
        }

        const Texture& oWhite = pMaterial->GetSystem().GetWhiteTexture();
        for (size_t uIndex = 0; uIndex < vTextureSamplerNames.size(); ++uIndex)
        {
            const ShaderParameterId oTextureId = oParameters.Find(vTextureSamplerNames[uIndex]);
            if (!oTextureId.IsValid())
            {
                continue;
            }

            const TextureBinding& oBinding = vTextures[uIndex];
            const Texture& oTexture = oBinding.pTexture != nullptr ? *oBinding.pTexture : oWhite;
            if (oBinding.optSampler)
            {
                oParameters.SetTexture(oTextureId, oTexture, *oBinding.optSampler);
            }
            else
            {
                oParameters.SetTexture(oTextureId, oTexture);
            }
        }
    }

    MaterialInstance::MaterialInstance(Material& oMaterial)
        : m_upImpl(std::make_unique<Impl>(oMaterial))
    {
    }

    MaterialInstance::~MaterialInstance() = default;

    MaterialParameterId MaterialInstance::FindValue(std::string_view svName) const noexcept
    {
        return FindByName(m_upImpl->pMaterial->GetParameterLayout().vValues, svName);
    }

    MaterialParameterId MaterialInstance::RequireValue(std::string_view svName) const
    {
        const MaterialParameterId oId = FindValue(svName);
        if (!oId.IsValid())
        {
            throw std::invalid_argument(std::format("Material '{}' has no value parameter named '{}'", m_upImpl->pMaterial->GetDebugName(), svName));
        }
        return oId;
    }

    MaterialParameterId MaterialInstance::FindTexture(std::string_view svName) const noexcept
    {
        return FindByName(m_upImpl->pMaterial->GetParameterLayout().vTextures, svName);
    }

    MaterialParameterId MaterialInstance::RequireTexture(std::string_view svName) const
    {
        const MaterialParameterId oId = FindTexture(svName);
        if (!oId.IsValid())
        {
            throw std::invalid_argument(std::format("Material '{}' has no texture parameter named '{}'", m_upImpl->pMaterial->GetDebugName(), svName));
        }
        return oId;
    }

    void MaterialInstance::SetFloat(MaterialParameterId oId, float fValue)
    {
        const MaterialValueParameter& oParameter = RequireParameter(m_upImpl->pMaterial->GetParameterLayout().vValues, oId, "value");
        if (oParameter.eType != MaterialValueType::Float1)
        {
            throw std::invalid_argument(std::format("Material parameter '{}' is {} and cannot be set with SetFloat", oParameter.sName, ToTypeName(oParameter.eType)));
        }
        WriteComponents(m_upImpl->vSlots, oParameter, glm::vec4(fValue));
        m_upImpl->MarkDirty();
    }

    void MaterialInstance::SetVector(MaterialParameterId oId, const glm::vec4& vecValue)
    {
        const MaterialValueParameter& oParameter = RequireParameter(m_upImpl->pMaterial->GetParameterLayout().vValues, oId, "value");
        if (oParameter.eType == MaterialValueType::Float1)
        {
            throw std::invalid_argument(std::format("Material parameter '{}' is Float1 and must be set with SetFloat", oParameter.sName));
        }
        WriteComponents(m_upImpl->vSlots, oParameter, vecValue);
        m_upImpl->MarkDirty();
    }

    void MaterialInstance::SetTexture(MaterialParameterId oId, const Texture& oTexture)
    {
        const MaterialTextureParameter& oParameter = RequireParameter(m_upImpl->pMaterial->GetParameterLayout().vTextures, oId, "texture");
        WarnOnColorSpaceMismatch(*m_upImpl->pMaterial, oParameter, oTexture);
        m_upImpl->vTextures[oId.uIndex] = {&oTexture, std::nullopt};
        m_upImpl->MarkDirty();
    }

    void MaterialInstance::SetTexture(MaterialParameterId oId, const Texture& oTexture, const SamplerState& oSampler)
    {
        const MaterialTextureParameter& oParameter = RequireParameter(m_upImpl->pMaterial->GetParameterLayout().vTextures, oId, "texture");
        WarnOnColorSpaceMismatch(*m_upImpl->pMaterial, oParameter, oTexture);
        m_upImpl->vTextures[oId.uIndex] = {&oTexture, oSampler};
        m_upImpl->MarkDirty();
    }

    void MaterialInstance::ClearTexture(MaterialParameterId oId)
    {
        static_cast<void>(RequireParameter(m_upImpl->pMaterial->GetParameterLayout().vTextures, oId, "texture"));
        m_upImpl->vTextures[oId.uIndex] = {};
        m_upImpl->MarkDirty();
    }

    void MaterialInstance::SetStaticSwitch(std::string_view svName, bool bValue)
    {
        Material& oMaterial = *m_upImpl->pMaterial;
        if (oMaterial.GetGraph() == nullptr)
        {
            throw std::logic_error(std::format("Material '{}' is not built from a material graph; static switch '{}' cannot be set", oMaterial.GetDebugName(), svName));
        }

        std::vector<MaterialStaticSwitchValue> vSwitches = m_upImpl->vStaticSwitches;
        const auto itSwitch = std::ranges::find(vSwitches, svName, &MaterialStaticSwitchValue::sName);
        if (itSwitch != vSwitches.end())
        {
            itSwitch->bValue = bValue;
        }
        else
        {
            vSwitches.push_back({std::string(svName), bValue});
        }

        const uint32_t uSet = oMaterial.ResolveStaticSwitchSet(vSwitches);
        m_upImpl->vStaticSwitches = std::move(vSwitches);
        m_upImpl->uStaticSwitchSet = uSet;
        m_upImpl->MarkDirty();
    }

    Material& MaterialInstance::GetMaterial() const noexcept
    {
        return *m_upImpl->pMaterial;
    }

    const MaterialInstance::Impl& MaterialInstance::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
