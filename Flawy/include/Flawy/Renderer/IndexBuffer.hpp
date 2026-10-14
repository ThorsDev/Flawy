#pragma once

#include "Flawy/Renderer/BufferUsage.hpp"

#include <cstdint>
#include <memory>
#include <span>

namespace Flawy::Renderer
{
    enum class IndexFormat : uint8_t
    {
        Uint16,
        Uint32
    };

    class IndexBuffer final
    {
    public:
        struct Impl;

        explicit IndexBuffer(std::span<const uint16_t> spanIndices, BufferUsage eUsage = BufferUsage::Static);
        explicit IndexBuffer(std::span<const uint32_t> spanIndices, BufferUsage eUsage = BufferUsage::Static);
        ~IndexBuffer();

        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;
        IndexBuffer(IndexBuffer&&) = delete;
        IndexBuffer& operator=(IndexBuffer&&) = delete;

        void Update(uint32_t uFirstIndex, std::span<const uint16_t> spanIndices);
        void Update(uint32_t uFirstIndex, std::span<const uint32_t> spanIndices);

        [[nodiscard]] uint32_t GetIndexCount() const noexcept;
        [[nodiscard]] IndexFormat GetFormat() const noexcept;
        [[nodiscard]] BufferUsage GetUsage() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
