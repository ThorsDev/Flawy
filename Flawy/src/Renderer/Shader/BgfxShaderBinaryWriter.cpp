#include "Renderer/Shader/BgfxShaderBinaryWriter.hpp"

#include "Flawy/Renderer/Shader/ShaderCompileError.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint8_t kShaderBinaryVersion = 12;
        constexpr uint8_t kFragmentUniformFlag = 0x10;
        constexpr uint8_t kSamplerUniformFlag = 0x20;

        constexpr uint32_t MakeFourCc(char cA, char cB, char cC, uint8_t uD) noexcept
        {
            return static_cast<uint32_t>(static_cast<uint8_t>(cA))
                 | (static_cast<uint32_t>(static_cast<uint8_t>(cB)) << 8)
                 | (static_cast<uint32_t>(static_cast<uint8_t>(cC)) << 16)
                 | (static_cast<uint32_t>(uD) << 24);
        }

        uint32_t GetMagic(ShaderStage eStage) noexcept
        {
            switch (eStage)
            {
            case ShaderStage::Vertex:
                return MakeFourCc('V', 'S', 'H', kShaderBinaryVersion);
            case ShaderStage::Fragment:
                return MakeFourCc('F', 'S', 'H', kShaderBinaryVersion);
            case ShaderStage::Compute:
                return MakeFourCc('C', 'S', 'H', kShaderBinaryVersion);
            }
            return 0;
        }

        class LittleEndianWriter final
        {
        public:
            void WriteU8(uint8_t uValue)
            {
                m_vBytes.push_back(static_cast<std::byte>(uValue));
            }

            void WriteU16(uint16_t uValue)
            {
                WriteU8(static_cast<uint8_t>(uValue & 0xffu));
                WriteU8(static_cast<uint8_t>(uValue >> 8));
            }

            void WriteU32(uint32_t uValue)
            {
                WriteU16(static_cast<uint16_t>(uValue & 0xffffu));
                WriteU16(static_cast<uint16_t>(uValue >> 16));
            }

            void WriteBytes(std::span<const std::byte> spanBytes)
            {
                m_vBytes.insert(m_vBytes.end(), spanBytes.begin(), spanBytes.end());
            }

            void WriteText(std::string_view svText)
            {
                for (const char cCharacter : svText)
                {
                    WriteU8(static_cast<uint8_t>(cCharacter));
                }
            }

            [[nodiscard]] std::vector<std::byte> Release() noexcept
            {
                return std::move(m_vBytes);
            }

        private:
            std::vector<std::byte> m_vBytes;
        };

        template <typename TValue>
        TValue CheckedNarrow(size_t uValue, std::string_view svWhat)
        {
            if (uValue > std::numeric_limits<TValue>::max())
            {
                throw ShaderCompileError(std::string(svWhat) + " exceeds the bgfx shader binary limit");
            }
            return static_cast<TValue>(uValue);
        }
    }

    std::vector<std::byte> BgfxShaderBinaryWriter::Write(ShaderStage eStage, const BgfxShaderInterface& oInterface, std::span<const std::byte> spanCode) const
    {
        const uint8_t uStageFlag = eStage == ShaderStage::Fragment ? kFragmentUniformFlag : 0;

        LittleEndianWriter oWriter;
        oWriter.WriteU32(GetMagic(eStage));
        oWriter.WriteU32(0);
        oWriter.WriteU32(0);
        oWriter.WriteU32(0);
        oWriter.WriteU32(0);

        oWriter.WriteU16(CheckedNarrow<uint16_t>(oInterface.vUniforms.size(), "Uniform count"));
        for (const BgfxUniform& oUniform : oInterface.vUniforms)
        {
            oWriter.WriteU8(CheckedNarrow<uint8_t>(oUniform.sName.size(), "Uniform name length"));
            oWriter.WriteText(oUniform.sName);
            const uint8_t uSamplerFlag = oUniform.uType == bgfx::UniformType::Sampler ? kSamplerUniformFlag : 0;
            oWriter.WriteU8(static_cast<uint8_t>(oUniform.uType | uStageFlag | uSamplerFlag));
            oWriter.WriteU8(oUniform.uNum);
            oWriter.WriteU16(oUniform.uRegIndex);
            oWriter.WriteU16(oUniform.uRegCount);
            oWriter.WriteU8(oUniform.uTexComponent);
            oWriter.WriteU8(oUniform.uTexDimension);
            oWriter.WriteU16(oUniform.uTexFormat);
        }

        oWriter.WriteU32(CheckedNarrow<uint32_t>(spanCode.size(), "Shader code size"));
        oWriter.WriteBytes(spanCode);
        oWriter.WriteU8(0);

        oWriter.WriteU8(CheckedNarrow<uint8_t>(oInterface.vAttributeIds.size(), "Vertex attribute count"));
        for (const uint16_t uAttributeId : oInterface.vAttributeIds)
        {
            oWriter.WriteU16(uAttributeId);
        }

        oWriter.WriteU16(oInterface.uUniformBufferSize);
        return oWriter.Release();
    }
}
