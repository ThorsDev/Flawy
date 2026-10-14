#include "Renderer/Bgfx/BgfxDrawSubmitter.hpp"

#include "Flawy/Renderer/IndexBuffer.hpp"
#include "Flawy/Renderer/ShaderParameters.hpp"
#include "Flawy/Renderer/ShaderProgram.hpp"
#include "Flawy/Renderer/VertexBuffer.hpp"
#include "Renderer/Bgfx/BgfxRenderStateMapping.hpp"
#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        void BindVertexBuffer(const VertexBuffer& oBuffer) noexcept
        {
            const VertexBuffer::Impl& oImpl = oBuffer.GetImpl();
            if (oImpl.eUsage == BufferUsage::Dynamic)
            {
                bgfx::setVertexBuffer(0, oImpl.hDynamic);
                return;
            }
            bgfx::setVertexBuffer(0, oImpl.hStatic);
        }

        void BindIndexBuffer(const IndexBuffer& oBuffer) noexcept
        {
            const IndexBuffer::Impl& oImpl = oBuffer.GetImpl();
            if (oImpl.eUsage == BufferUsage::Dynamic)
            {
                bgfx::setIndexBuffer(oImpl.hDynamic);
                return;
            }
            bgfx::setIndexBuffer(oImpl.hStatic);
        }

        const ShaderUniform* FindFirstSampler(const ShaderProgram& oProgram) noexcept
        {
            for (const ShaderUniform& oUniform : oProgram.GetUniforms())
            {
                if (oUniform.eType == ShaderUniformType::Sampler)
                {
                    return &oUniform;
                }
            }
            return nullptr;
        }
    }

    void BgfxDrawSubmitter::Submit(bgfx::ViewId uViewId, const DrawCommand& oCommand)
    {
        Validate(oCommand);

        bgfx::setTransform(glm::value_ptr(oCommand.matModel));
        BindVertexBuffer(*oCommand.pVertexBuffer);
        if (oCommand.pIndexBuffer != nullptr)
        {
            BindIndexBuffer(*oCommand.pIndexBuffer);
        }
        if (oCommand.pParameters != nullptr)
        {
            oCommand.pParameters->GetImpl().Apply();
        }
        bgfx::setState(BgfxRenderStateMapping::ToBgfxState(oCommand.oState));
        bgfx::submit(uViewId, oCommand.pProgram->GetImpl().hProgram);
    }

    void BgfxDrawSubmitter::Validate(const DrawCommand& oCommand)
    {
        if (oCommand.pVertexBuffer == nullptr || oCommand.pProgram == nullptr)
        {
            throw std::invalid_argument("DrawCommand requires a vertex buffer and a shader program");
        }

        if (oCommand.pParameters == nullptr)
        {
            const ShaderUniform* pSampler = FindFirstSampler(*oCommand.pProgram);
            if (pSampler != nullptr)
            {
                throw std::logic_error("DrawCommand has no shader parameters but the program samples texture '" + pSampler->sName + "'");
            }
            return;
        }

        if (&oCommand.pParameters->GetProgram() != oCommand.pProgram)
        {
            throw std::invalid_argument("DrawCommand shader parameters belong to a different shader program");
        }

        const ShaderUniform* pUnbound = oCommand.pParameters->GetImpl().FindUnboundTexture();
        if (pUnbound != nullptr)
        {
            throw std::logic_error("DrawCommand shader parameters have no texture set for sampler '" + pUnbound->sName + "'");
        }
    }
}
