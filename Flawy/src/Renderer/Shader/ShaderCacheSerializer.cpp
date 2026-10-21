#include "Renderer/Shader/ShaderCacheSerializer.hpp"

#include "Flawy/Core/Hash.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::array<std::byte, 4> kMagic = {std::byte{'F'}, std::byte{'L'}, std::byte{'S'}, std::byte{'C'}};
        constexpr size_t kChecksumSize = sizeof(uint64_t);
        constexpr uint8_t kMaxStage = static_cast<uint8_t>(ShaderStage::Compute);
        constexpr uint8_t kMaxUniformType = static_cast<uint8_t>(ShaderUniformType::Sampler);
        constexpr uint8_t kMaxVertexAttribute = static_cast<uint8_t>(VertexAttribute::TexCoord7);

        class ByteWriter final
        {
        public:
            void WriteBytes(std::span<const std::byte> spanBytes)
            {
                m_vBytes.insert(m_vBytes.end(), spanBytes.begin(), spanBytes.end());
            }

            void WriteU8(uint8_t uValue)
            {
                m_vBytes.push_back(static_cast<std::byte>(uValue));
            }

            void WriteU16(uint16_t uValue)
            {
                WriteUnsigned(uValue, sizeof(uint16_t));
            }

            void WriteU32(uint32_t uValue)
            {
                WriteUnsigned(uValue, sizeof(uint32_t));
            }

            void WriteU64(uint64_t uValue)
            {
                WriteUnsigned(uValue, sizeof(uint64_t));
            }

            void WriteSize(size_t uValue)
            {
                if (uValue > std::numeric_limits<uint32_t>::max())
                {
                    throw std::length_error("Shader cache entry field exceeds 4 GiB");
                }
                WriteU32(static_cast<uint32_t>(uValue));
            }

            void WriteString(std::string_view svText)
            {
                WriteSize(svText.size());
                WriteBytes(std::as_bytes(std::span<const char>(svText.data(), svText.size())));
            }

            [[nodiscard]] const std::vector<std::byte>& GetBytes() const noexcept
            {
                return m_vBytes;
            }

            [[nodiscard]] std::vector<std::byte> Release() noexcept
            {
                return std::move(m_vBytes);
            }

        private:
            void WriteUnsigned(uint64_t uValue, size_t uByteCount)
            {
                for (size_t uByte = 0; uByte < uByteCount; ++uByte)
                {
                    m_vBytes.push_back(static_cast<std::byte>((uValue >> (uByte * 8u)) & 0xffu));
                }
            }

            std::vector<std::byte> m_vBytes;
        };

        class ByteReader final
        {
        public:
            explicit ByteReader(std::span<const std::byte> spanBytes) noexcept
                : m_spanBytes(spanBytes)
            {
            }

            [[nodiscard]] bool ReadBytes(size_t uCount, std::span<const std::byte>& spanOut) noexcept
            {
                if (uCount > GetRemaining())
                {
                    return false;
                }
                spanOut = m_spanBytes.subspan(m_uPosition, uCount);
                m_uPosition += uCount;
                return true;
            }

            [[nodiscard]] bool ReadU8(uint8_t& uOut) noexcept
            {
                uint64_t uValue = 0;
                const bool bRead = ReadUnsigned(sizeof(uint8_t), uValue);
                uOut = static_cast<uint8_t>(uValue);
                return bRead;
            }

            [[nodiscard]] bool ReadU16(uint16_t& uOut) noexcept
            {
                uint64_t uValue = 0;
                const bool bRead = ReadUnsigned(sizeof(uint16_t), uValue);
                uOut = static_cast<uint16_t>(uValue);
                return bRead;
            }

            [[nodiscard]] bool ReadU32(uint32_t& uOut) noexcept
            {
                uint64_t uValue = 0;
                const bool bRead = ReadUnsigned(sizeof(uint32_t), uValue);
                uOut = static_cast<uint32_t>(uValue);
                return bRead;
            }

            [[nodiscard]] bool ReadU64(uint64_t& uOut) noexcept
            {
                return ReadUnsigned(sizeof(uint64_t), uOut);
            }

            [[nodiscard]] bool ReadString(std::string& sOut)
            {
                uint32_t uLength = 0;
                std::span<const std::byte> spanText;
                if (!ReadU32(uLength) || !ReadBytes(uLength, spanText))
                {
                    return false;
                }
                sOut.assign(reinterpret_cast<const char*>(spanText.data()), spanText.size());
                return true;
            }

            [[nodiscard]] size_t GetRemaining() const noexcept
            {
                return m_spanBytes.size() - m_uPosition;
            }

        private:
            [[nodiscard]] bool ReadUnsigned(size_t uByteCount, uint64_t& uOut) noexcept
            {
                if (uByteCount > GetRemaining())
                {
                    return false;
                }
                uOut = 0;
                for (size_t uByte = 0; uByte < uByteCount; ++uByte)
                {
                    uOut |= static_cast<uint64_t>(m_spanBytes[m_uPosition + uByte]) << (uByte * 8u);
                }
                m_uPosition += uByteCount;
                return true;
            }

            std::span<const std::byte> m_spanBytes;
            size_t m_uPosition = 0;
        };

        uint64_t ComputeChecksum(std::span<const std::byte> spanBytes) noexcept
        {
            Core::Fnv1aHasher oHasher;
            oHasher.Append(spanBytes);
            return oHasher.GetValue();
        }

        class EntryParser final
        {
        public:
            explicit EntryParser(std::span<const std::byte> spanPayload) noexcept
                : m_oReader(spanPayload)
            {
            }

            [[nodiscard]] ShaderCacheReadResult Parse(const ShaderCacheKey& oKey)
            {
                std::span<const std::byte> spanMagic;
                if (!m_oReader.ReadBytes(kMagic.size(), spanMagic) || !std::equal(spanMagic.begin(), spanMagic.end(), kMagic.begin()))
                {
                    return Fail("bad magic");
                }

                uint32_t uVersion = 0;
                if (!m_oReader.ReadU32(uVersion))
                {
                    return Fail("truncated header");
                }
                if (uVersion != ShaderCache::GetFormatVersion())
                {
                    return Fail(std::format("format version {} (expected {})", uVersion, ShaderCache::GetFormatVersion()));
                }

                ShaderCacheKey oStoredKey;
                if (!m_oReader.ReadU64(oStoredKey.uLow) || !m_oReader.ReadU64(oStoredKey.uHigh))
                {
                    return Fail("truncated header");
                }
                if (oStoredKey != oKey)
                {
                    return Fail(std::format("entry belongs to key {}", oStoredKey.ToHex()));
                }

                CompiledShader oShader;
                if (!ReadShader(oShader))
                {
                    return Fail(m_sError);
                }
                if (m_oReader.GetRemaining() != 0)
                {
                    return Fail(std::format("{} unexpected trailing byte(s)", m_oReader.GetRemaining()));
                }

                ShaderCacheReadResult oResult;
                oResult.optShader = std::move(oShader);
                return oResult;
            }

        private:
            [[nodiscard]] static ShaderCacheReadResult Fail(std::string sError)
            {
                ShaderCacheReadResult oResult;
                oResult.sError = std::move(sError);
                return oResult;
            }

            [[nodiscard]] bool Reject(std::string_view svError)
            {
                m_sError = std::string(svError);
                return false;
            }

            [[nodiscard]] bool ReadShader(CompiledShader& oShader)
            {
                uint8_t uStage = 0;
                if (!m_oReader.ReadU8(uStage) || !m_oReader.ReadString(oShader.sName) || !m_oReader.ReadString(oShader.sEntryPoint))
                {
                    return Reject("truncated shader header");
                }
                if (uStage > kMaxStage)
                {
                    return Reject(std::format("invalid stage {}", uStage));
                }
                oShader.eStage = static_cast<ShaderStage>(uStage);
                return ReadUniforms(oShader.oInterface) && ReadAttributes(oShader.oInterface) && ReadBinary(oShader);
            }

            [[nodiscard]] bool ReadUniforms(ShaderInterface& oInterface)
            {
                uint32_t uCount = 0;
                if (!m_oReader.ReadU32(uCount))
                {
                    return Reject("truncated uniform table");
                }
                for (uint32_t uIndex = 0; uIndex < uCount; ++uIndex)
                {
                    ShaderUniform oUniform;
                    uint8_t uType = 0;
                    uint8_t uPredefined = 0;
                    if (!m_oReader.ReadString(oUniform.sName) || !m_oReader.ReadU8(uType) || !m_oReader.ReadU16(oUniform.uCount) || !m_oReader.ReadU8(oUniform.uTextureStage) ||
                        !m_oReader.ReadU8(uPredefined))
                    {
                        return Reject("truncated uniform table");
                    }
                    if (uType > kMaxUniformType || uPredefined > 1)
                    {
                        return Reject(std::format("invalid uniform '{}'", oUniform.sName));
                    }
                    oUniform.eType = static_cast<ShaderUniformType>(uType);
                    oUniform.bPredefined = uPredefined == 1;
                    oInterface.vUniforms.push_back(std::move(oUniform));
                }
                return true;
            }

            [[nodiscard]] bool ReadAttributes(ShaderInterface& oInterface)
            {
                uint32_t uCount = 0;
                if (!m_oReader.ReadU32(uCount) || uCount > m_oReader.GetRemaining())
                {
                    return Reject("truncated vertex attribute table");
                }
                for (uint32_t uIndex = 0; uIndex < uCount; ++uIndex)
                {
                    uint8_t uAttribute = 0;
                    if (!m_oReader.ReadU8(uAttribute))
                    {
                        return Reject("truncated vertex attribute table");
                    }
                    if (uAttribute > kMaxVertexAttribute)
                    {
                        return Reject(std::format("invalid vertex attribute {}", uAttribute));
                    }
                    oInterface.vVertexAttributes.push_back(static_cast<VertexAttribute>(uAttribute));
                }
                if (!m_oReader.ReadU32(oInterface.uUniformBufferSize))
                {
                    return Reject("truncated uniform buffer size");
                }
                return true;
            }

            [[nodiscard]] bool ReadBinary(CompiledShader& oShader)
            {
                uint32_t uSize = 0;
                std::span<const std::byte> spanBinary;
                if (!m_oReader.ReadU32(uSize) || !m_oReader.ReadBytes(uSize, spanBinary))
                {
                    return Reject("truncated shader binary");
                }
                oShader.vBgfxBinary.assign(spanBinary.begin(), spanBinary.end());
                return true;
            }

            ByteReader m_oReader;
            std::string m_sError;
        };
    }

    std::vector<std::byte> ShaderCacheSerializer::Serialize(const ShaderCacheKey& oKey, const CompiledShader& oShader) const
    {
        ByteWriter oWriter;
        oWriter.WriteBytes(kMagic);
        oWriter.WriteU32(ShaderCache::GetFormatVersion());
        oWriter.WriteU64(oKey.uLow);
        oWriter.WriteU64(oKey.uHigh);
        oWriter.WriteU8(static_cast<uint8_t>(oShader.eStage));
        oWriter.WriteString(oShader.sName);
        oWriter.WriteString(oShader.sEntryPoint);

        oWriter.WriteSize(oShader.oInterface.vUniforms.size());
        for (const ShaderUniform& oUniform : oShader.oInterface.vUniforms)
        {
            oWriter.WriteString(oUniform.sName);
            oWriter.WriteU8(static_cast<uint8_t>(oUniform.eType));
            oWriter.WriteU16(oUniform.uCount);
            oWriter.WriteU8(oUniform.uTextureStage);
            oWriter.WriteU8(oUniform.bPredefined ? 1 : 0);
        }

        oWriter.WriteSize(oShader.oInterface.vVertexAttributes.size());
        for (const VertexAttribute eAttribute : oShader.oInterface.vVertexAttributes)
        {
            oWriter.WriteU8(static_cast<uint8_t>(eAttribute));
        }
        oWriter.WriteU32(oShader.oInterface.uUniformBufferSize);

        oWriter.WriteSize(oShader.vBgfxBinary.size());
        oWriter.WriteBytes(oShader.vBgfxBinary);
        oWriter.WriteU64(ComputeChecksum(oWriter.GetBytes()));
        return oWriter.Release();
    }

    ShaderCacheReadResult ShaderCacheSerializer::Deserialize(const ShaderCacheKey& oKey, std::span<const std::byte> spanBytes) const
    {
        if (spanBytes.size() < kChecksumSize)
        {
            ShaderCacheReadResult oResult;
            oResult.sError = std::format("file is only {} byte(s) long", spanBytes.size());
            return oResult;
        }

        const std::span<const std::byte> spanPayload = spanBytes.first(spanBytes.size() - kChecksumSize);
        uint64_t uStoredChecksum = 0;
        ByteReader oChecksumReader(spanBytes.last(kChecksumSize));
        static_cast<void>(oChecksumReader.ReadU64(uStoredChecksum));
        if (uStoredChecksum != ComputeChecksum(spanPayload))
        {
            ShaderCacheReadResult oResult;
            oResult.sError = "checksum mismatch (truncated or corrupted file)";
            return oResult;
        }
        return EntryParser(spanPayload).Parse(oKey);
    }
}
