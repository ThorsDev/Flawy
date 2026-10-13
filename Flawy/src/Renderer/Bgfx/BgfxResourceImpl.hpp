#pragma once

#include "Flawy/Renderer/ShaderProgram.hpp"
#include "Flawy/Renderer/VertexBuffer.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <vector>

namespace Flawy::Renderer
{
    struct VertexBuffer::Impl
    {
        Impl() = default;

        ~Impl()
        {
            if (bgfx::isValid(hBuffer))
            {
                bgfx::destroy(hBuffer);
            }
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        bgfx::VertexBufferHandle hBuffer = BGFX_INVALID_HANDLE;
        uint32_t uVertexCount = 0;
    };

    struct ShaderProgram::Impl
    {
        Impl() = default;

        ~Impl()
        {
            if (bgfx::isValid(hProgram))
            {
                bgfx::destroy(hProgram);
            }
            for (const bgfx::UniformHandle hUniform : vUniforms)
            {
                bgfx::destroy(hUniform);
            }
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        bgfx::ProgramHandle hProgram = BGFX_INVALID_HANDLE;
        std::vector<bgfx::UniformHandle> vUniforms;
    };
}
