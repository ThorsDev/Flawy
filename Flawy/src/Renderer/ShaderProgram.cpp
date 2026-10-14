#include "Flawy/Renderer/ShaderProgram.hpp"

#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        bgfx::UniformType::Enum ToBgfxUniformType(ShaderUniformType eType) noexcept
        {
            switch (eType)
            {
            case ShaderUniformType::Vec4:
                return bgfx::UniformType::Vec4;
            case ShaderUniformType::Mat3:
                return bgfx::UniformType::Mat3;
            case ShaderUniformType::Mat4:
                return bgfx::UniformType::Mat4;
            case ShaderUniformType::Sampler:
                return bgfx::UniformType::Sampler;
            }
            return bgfx::UniformType::Count;
        }

        bgfx::ShaderHandle CreateShader(const CompiledShader& oShader)
        {
            if (oShader.vBgfxBinary.empty() || oShader.vBgfxBinary.size() > std::numeric_limits<uint32_t>::max())
            {
                throw std::invalid_argument("Compiled shader '" + oShader.sName + "' has an invalid binary size");
            }

            const bgfx::ShaderHandle hShader = bgfx::createShader(bgfx::copy(oShader.vBgfxBinary.data(), static_cast<uint32_t>(oShader.vBgfxBinary.size())));
            if (bgfx::isValid(hShader))
            {
                bgfx::setName(hShader, oShader.sName.c_str(), static_cast<int32_t>(oShader.sName.size()));
            }
            return hShader;
        }

        class ScopedShaderHandle final
        {
        public:
            explicit ScopedShaderHandle(bgfx::ShaderHandle hShader) noexcept
                : m_hShader(hShader)
            {
            }

            ~ScopedShaderHandle()
            {
                if (bgfx::isValid(m_hShader))
                {
                    bgfx::destroy(m_hShader);
                }
            }

            ScopedShaderHandle(const ScopedShaderHandle&) = delete;
            ScopedShaderHandle& operator=(const ScopedShaderHandle&) = delete;
            ScopedShaderHandle(ScopedShaderHandle&&) = delete;
            ScopedShaderHandle& operator=(ScopedShaderHandle&&) = delete;

            [[nodiscard]] bool IsValid() const noexcept
            {
                return bgfx::isValid(m_hShader);
            }

            [[nodiscard]] bgfx::ShaderHandle Get() const noexcept
            {
                return m_hShader;
            }

        private:
            bgfx::ShaderHandle m_hShader;
        };

        class SamplerStageValidator final
        {
        public:
            void Check(const ShaderUniform& oUniform)
            {
                if (oUniform.eType != ShaderUniformType::Sampler)
                {
                    return;
                }

                for (const ShaderUniform* pKnown : m_vSamplers)
                {
                    const bool bSameName = pKnown->sName == oUniform.sName;
                    const bool bSameStage = pKnown->uTextureStage == oUniform.uTextureStage;
                    if (bSameName && !bSameStage)
                    {
                        throw std::invalid_argument(std::format("Sampler '{}' is declared with texture stage {} and {}", oUniform.sName, pKnown->uTextureStage, oUniform.uTextureStage));
                    }
                    if (!bSameName && bSameStage)
                    {
                        throw std::invalid_argument(std::format("Samplers '{}' and '{}' both use texture stage {}", pKnown->sName, oUniform.sName, oUniform.uTextureStage));
                    }
                }
                m_vSamplers.push_back(&oUniform);
            }

        private:
            std::vector<const ShaderUniform*> m_vSamplers;
        };

        void RequireStage(const CompiledShader& oShader, ShaderStage eExpected, const char* szRole)
        {
            if (oShader.eStage != eExpected)
            {
                throw std::invalid_argument("Shader '" + oShader.sName + "' passed as " + szRole + " shader has a different stage");
            }
        }
    }

    ShaderProgram::ShaderProgram(const CompiledShader& oVertexShader, const CompiledShader& oFragmentShader)
        : m_upImpl(std::make_unique<Impl>())
    {
        RequireStage(oVertexShader, ShaderStage::Vertex, "vertex");
        RequireStage(oFragmentShader, ShaderStage::Fragment, "fragment");

        SamplerStageValidator oSamplerValidator;
        for (const CompiledShader* pShader : {&oVertexShader, &oFragmentShader})
        {
            for (const ShaderUniform& oUniform : pShader->oInterface.vUniforms)
            {
                oSamplerValidator.Check(oUniform);
            }
        }

        std::unordered_set<std::string> setRegistered;
        for (const CompiledShader* pShader : {&oVertexShader, &oFragmentShader})
        {
            for (const ShaderUniform& oUniform : pShader->oInterface.vUniforms)
            {
                if (oUniform.bPredefined || !setRegistered.insert(oUniform.sName).second)
                {
                    continue;
                }
                const bgfx::UniformHandle hUniform = bgfx::createUniform(oUniform.sName.c_str(), ToBgfxUniformType(oUniform.eType), oUniform.uCount);
                if (!bgfx::isValid(hUniform))
                {
                    throw std::runtime_error("Failed to create uniform '" + oUniform.sName + "' for shader '" + pShader->sName + "'");
                }
                m_upImpl->vUniforms.push_back(hUniform);
                m_upImpl->vInterface.push_back(oUniform);
            }
        }

        const ScopedShaderHandle oVertex(CreateShader(oVertexShader));
        const ScopedShaderHandle oFragment(CreateShader(oFragmentShader));
        if (!oVertex.IsValid() || !oFragment.IsValid())
        {
            throw std::runtime_error("bgfx rejected the shader binaries of '" + oVertexShader.sName + "' / '" + oFragmentShader.sName + "'");
        }

        m_upImpl->hProgram = bgfx::createProgram(oVertex.Get(), oFragment.Get(), false);
        if (!bgfx::isValid(m_upImpl->hProgram))
        {
            throw std::runtime_error("Failed to link shader program '" + oVertexShader.sName + "' / '" + oFragmentShader.sName + "'");
        }
    }

    ShaderProgram::~ShaderProgram() = default;

    std::span<const ShaderUniform> ShaderProgram::GetUniforms() const noexcept
    {
        return m_upImpl->vInterface;
    }

    std::optional<uint32_t> ShaderProgram::FindUniform(std::string_view svName) const noexcept
    {
        const std::vector<ShaderUniform>& vInterface = m_upImpl->vInterface;
        for (size_t uIndex = 0; uIndex < vInterface.size(); ++uIndex)
        {
            if (vInterface[uIndex].sName == svName)
            {
                return static_cast<uint32_t>(uIndex);
            }
        }
        return std::nullopt;
    }

    const ShaderProgram::Impl& ShaderProgram::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
