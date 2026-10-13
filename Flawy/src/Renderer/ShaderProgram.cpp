#include "Flawy/Renderer/ShaderProgram.hpp"

#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>

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

    const ShaderProgram::Impl& ShaderProgram::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
