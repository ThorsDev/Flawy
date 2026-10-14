#include "Flawy/Renderer/ShaderParameters.hpp"

#include "Renderer/Bgfx/BgfxRenderStateMapping.hpp"
#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        uint32_t GetFloatsPerElement(ShaderUniformType eType) noexcept
        {
            switch (eType)
            {
            case ShaderUniformType::Vec4:
                return 4;
            case ShaderUniformType::Mat3:
                return 9;
            case ShaderUniformType::Mat4:
                return 16;
            case ShaderUniformType::Sampler:
                return 0;
            }
            return 0;
        }

        std::string_view ToTypeName(ShaderUniformType eType) noexcept
        {
            switch (eType)
            {
            case ShaderUniformType::Vec4:
                return "Vec4";
            case ShaderUniformType::Mat3:
                return "Mat3";
            case ShaderUniformType::Mat4:
                return "Mat4";
            case ShaderUniformType::Sampler:
                return "Sampler";
            }
            return "Unknown";
        }
    }

    bool ShaderParameterId::IsValid() const noexcept
    {
        return uIndex != ShaderParameters::Impl::kNoSlot;
    }

    ShaderParameters::Impl::Impl(const ShaderProgram& oProgram) : pProgram(&oProgram)
    {
        const std::span<const ShaderUniform> spanUniforms = oProgram.GetUniforms();
        vSlotIndices.reserve(spanUniforms.size());

        uint32_t uFloatCount = 0;
        for (size_t uIndex = 0; uIndex < spanUniforms.size(); ++uIndex)
        {
            const ShaderUniform& oUniform = spanUniforms[uIndex];
            const auto uUniformIndex = static_cast<uint32_t>(uIndex);
            if (oUniform.eType == ShaderUniformType::Sampler)
            {
                vSlotIndices.push_back(static_cast<uint32_t>(vTextures.size()));
                vTextures.push_back({uUniformIndex, oUniform.uTextureStage, nullptr, kTextureSamplerFlags});
                continue;
            }

            vSlotIndices.push_back(static_cast<uint32_t>(vValues.size()));
            vValues.push_back({uUniformIndex, uFloatCount, false});
            uFloatCount += GetFloatsPerElement(oUniform.eType) * oUniform.uCount;
        }
        vData.assign(uFloatCount, 0.0f);
    }

    void ShaderParameters::Impl::Apply() const
    {
        const ShaderProgram::Impl& oProgramImpl = pProgram->GetImpl();
        for (const ValueSlot& oSlot : vValues)
        {
            if (oSlot.bSet)
            {
                bgfx::setUniform(oProgramImpl.vUniforms[oSlot.uUniformIndex], vData.data() + oSlot.uOffset,
                                 oProgramImpl.vInterface[oSlot.uUniformIndex].uCount);
            }
        }
        for (const TextureSlot& oSlot : vTextures)
        {
            if (oSlot.pTexture != nullptr)
            {
                bgfx::setTexture(oSlot.uStage, oProgramImpl.vUniforms[oSlot.uUniformIndex], oSlot.pTexture->GetImpl().hTexture,
                                 oSlot.uSamplerFlags);
            }
        }
    }

    const ShaderUniform* ShaderParameters::Impl::FindUnboundTexture() const noexcept
    {
        for (const TextureSlot& oSlot : vTextures)
        {
            if (oSlot.pTexture == nullptr)
            {
                return &pProgram->GetUniforms()[oSlot.uUniformIndex];
            }
        }
        return nullptr;
    }

    namespace
    {
        template <typename TValue> std::span<const float> AsFloats(std::span<const TValue> spanValues)
        {
            if (spanValues.empty())
            {
                return {};
            }
            return {glm::value_ptr(spanValues.front()), spanValues.size() * (sizeof(TValue) / sizeof(float))};
        }

        class ShaderParameterWriter final
        {
        public:
            ShaderParameterWriter(ShaderParameters::Impl& oImpl, ShaderParameterId oId)
                : m_oImpl(oImpl), m_uUniformIndex(RequireIndex(oImpl, oId)), m_oUniform(oImpl.pProgram->GetUniforms()[m_uUniformIndex])
            {
            }

            void WriteValues(ShaderUniformType eType, std::span<const float> spanValues, size_t uElementCount)
            {
                RequireType(eType);
                if (uElementCount == 0 || uElementCount > m_oUniform.uCount)
                {
                    throw std::invalid_argument(std::format("Shader parameter '{}' holds {} element(s) but {} were given", m_oUniform.sName,
                                                            m_oUniform.uCount, uElementCount));
                }

                ShaderParameters::Impl::ValueSlot& oSlot = m_oImpl.vValues[m_oImpl.vSlotIndices[m_uUniformIndex]];
                std::ranges::copy(spanValues, m_oImpl.vData.begin() + oSlot.uOffset);
                oSlot.bSet = true;
            }

            void WriteTexture(const Texture& oTexture, uint32_t uSamplerFlags)
            {
                RequireType(ShaderUniformType::Sampler);
                ShaderParameters::Impl::TextureSlot& oSlot = m_oImpl.vTextures[m_oImpl.vSlotIndices[m_uUniformIndex]];
                oSlot.pTexture = &oTexture;
                oSlot.uSamplerFlags = uSamplerFlags;
            }

        private:
            static uint32_t RequireIndex(const ShaderParameters::Impl& oImpl, ShaderParameterId oId)
            {
                if (!oId.IsValid() || oId.uIndex >= oImpl.vSlotIndices.size())
                {
                    throw std::invalid_argument("Invalid shader parameter id");
                }
                return oId.uIndex;
            }

            void RequireType(ShaderUniformType eType) const
            {
                if (m_oUniform.eType != eType)
                {
                    throw std::invalid_argument(std::format("Shader parameter '{}' is {} but was set as {}", m_oUniform.sName,
                                                            ToTypeName(m_oUniform.eType), ToTypeName(eType)));
                }
            }

            ShaderParameters::Impl& m_oImpl;
            uint32_t m_uUniformIndex;
            const ShaderUniform& m_oUniform;
        };
    }

    ShaderParameters::ShaderParameters(const ShaderProgram& oProgram) : m_upImpl(std::make_unique<Impl>(oProgram))
    {
    }

    ShaderParameters::~ShaderParameters() = default;

    ShaderParameterId ShaderParameters::Find(std::string_view svName) const noexcept
    {
        const std::optional<uint32_t> optIndex = m_upImpl->pProgram->FindUniform(svName);
        return optIndex ? ShaderParameterId{*optIndex} : ShaderParameterId{};
    }

    ShaderParameterId ShaderParameters::Require(std::string_view svName) const
    {
        const ShaderParameterId oId = Find(svName);
        if (!oId.IsValid())
        {
            throw std::invalid_argument("Shader program has no parameter named '" + std::string(svName) + "'");
        }
        return oId;
    }

    void ShaderParameters::SetVec4(ShaderParameterId oId, const glm::vec4& vecValue)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteValues(ShaderUniformType::Vec4, AsFloats(std::span(&vecValue, 1)), 1);
    }

    void ShaderParameters::SetVec4Array(ShaderParameterId oId, std::span<const glm::vec4> spanValues)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteValues(ShaderUniformType::Vec4, AsFloats(spanValues), spanValues.size());
    }

    void ShaderParameters::SetMat3(ShaderParameterId oId, const glm::mat3& matValue)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteValues(ShaderUniformType::Mat3, AsFloats(std::span(&matValue, 1)), 1);
    }

    void ShaderParameters::SetMat3Array(ShaderParameterId oId, std::span<const glm::mat3> spanValues)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteValues(ShaderUniformType::Mat3, AsFloats(spanValues), spanValues.size());
    }

    void ShaderParameters::SetMat4(ShaderParameterId oId, const glm::mat4& matValue)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteValues(ShaderUniformType::Mat4, AsFloats(std::span(&matValue, 1)), 1);
    }

    void ShaderParameters::SetMat4Array(ShaderParameterId oId, std::span<const glm::mat4> spanValues)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteValues(ShaderUniformType::Mat4, AsFloats(spanValues), spanValues.size());
    }

    void ShaderParameters::SetTexture(ShaderParameterId oId, const Texture& oTexture)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteTexture(oTexture, Impl::kTextureSamplerFlags);
    }

    void ShaderParameters::SetTexture(ShaderParameterId oId, const Texture& oTexture, const SamplerState& oSampler)
    {
        ShaderParameterWriter(*m_upImpl, oId).WriteTexture(oTexture, BgfxRenderStateMapping::ToBgfxSamplerFlags(oSampler));
    }

    const ShaderProgram& ShaderParameters::GetProgram() const noexcept
    {
        return *m_upImpl->pProgram;
    }

    const ShaderParameters::Impl& ShaderParameters::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
