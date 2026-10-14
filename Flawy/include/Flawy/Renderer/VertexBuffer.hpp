#pragma once

#include "Flawy/Renderer/BufferUsage.hpp"
#include "Flawy/Renderer/VertexLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Flawy::Renderer
{
    class VertexBuffer final
    {
    public:
        struct Impl;

        VertexBuffer(std::span<const std::byte> spanData, const VertexLayout& oLayout, BufferUsage eUsage = BufferUsage::Static);
        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&&) = delete;
        VertexBuffer& operator=(VertexBuffer&&) = delete;

        void Update(uint32_t uFirstVertex, std::span<const std::byte> spanData);

        [[nodiscard]] uint32_t GetVertexCount() const noexcept;
        [[nodiscard]] BufferUsage GetUsage() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
