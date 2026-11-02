#include "Flawy/Animation/AnimationClipDescriptorSerializer.hpp"

#include "Flawy/Animation/AnimationClipAsset.hpp"
#include "Flawy/Assets/AssetFile.hpp"
#include "Flawy/Core/BinaryFormatError.hpp"
#include "Flawy/Core/BinaryReader.hpp"
#include "Flawy/Core/BinaryWriter.hpp"
#include "Flawy/Core/FileBytes.hpp"

#include <format>
#include <stdexcept>
#include <string_view>

namespace Flawy::Animation
{
    namespace
    {
        constexpr size_t kMinimumTrackSize = 4 + 3 * 4;
        constexpr size_t kVec3KeySize = 4 * sizeof(float);
        constexpr size_t kQuatKeySize = 5 * sizeof(float);

        class ClipPayloadWriter final
        {
        public:
            explicit ClipPayloadWriter(Core::BinaryWriter& oWriter) noexcept
                : m_oWriter(oWriter)
            {
            }

            void Write(const AnimationClipDescriptor& oClip)
            {
                m_oWriter.WriteString(oClip.sSkeletonPath);
                m_oWriter.WriteString(oClip.sName);
                m_oWriter.WriteF32(oClip.fDurationSeconds);
                m_oWriter.WriteU32(static_cast<uint32_t>(oClip.vTracks.size()));
                for (const JointTrackDescriptor& oTrack : oClip.vTracks)
                {
                    m_oWriter.WriteString(oTrack.sJointName);
                    WriteVec3Keys(oTrack.vTranslations);
                    WriteQuatKeys(oTrack.vRotations);
                    WriteVec3Keys(oTrack.vScales);
                }
            }

        private:
            void WriteVec3Keys(const std::vector<Vec3Key>& vKeys)
            {
                m_oWriter.WriteU32(static_cast<uint32_t>(vKeys.size()));
                for (const Vec3Key& oKey : vKeys)
                {
                    m_oWriter.WriteF32(oKey.fTimeSeconds);
                    m_oWriter.WriteF32(oKey.vecValue.x);
                    m_oWriter.WriteF32(oKey.vecValue.y);
                    m_oWriter.WriteF32(oKey.vecValue.z);
                }
            }

            void WriteQuatKeys(const std::vector<QuatKey>& vKeys)
            {
                m_oWriter.WriteU32(static_cast<uint32_t>(vKeys.size()));
                for (const QuatKey& oKey : vKeys)
                {
                    m_oWriter.WriteF32(oKey.fTimeSeconds);
                    m_oWriter.WriteF32(oKey.quatValue.x);
                    m_oWriter.WriteF32(oKey.quatValue.y);
                    m_oWriter.WriteF32(oKey.quatValue.z);
                    m_oWriter.WriteF32(oKey.quatValue.w);
                }
            }

            Core::BinaryWriter& m_oWriter;
        };

        class ClipPayloadReader final
        {
        public:
            explicit ClipPayloadReader(Core::BinaryReader& oReader) noexcept
                : m_oReader(oReader)
            {
            }

            [[nodiscard]] AnimationClipDescriptor Read()
            {
                AnimationClipDescriptor oClip;
                oClip.sSkeletonPath = m_oReader.ReadString(AnimationClipDescriptor::kMaxNameLength);
                oClip.sName = m_oReader.ReadString(AnimationClipDescriptor::kMaxNameLength);
                oClip.fDurationSeconds = m_oReader.ReadF32();

                const uint32_t uTrackCount = ReadCount(AnimationClipDescriptor::kMaxTracks, kMinimumTrackSize, "track");
                oClip.vTracks.resize(uTrackCount);
                for (JointTrackDescriptor& oTrack : oClip.vTracks)
                {
                    oTrack.sJointName = m_oReader.ReadString(AnimationClipDescriptor::kMaxNameLength);
                    ReadVec3Keys(oTrack.vTranslations);
                    ReadQuatKeys(oTrack.vRotations);
                    ReadVec3Keys(oTrack.vScales);
                }
                return oClip;
            }

        private:
            [[nodiscard]] uint32_t ReadCount(uint32_t uMaxCount, size_t uElementSize, std::string_view svWhat)
            {
                const size_t uOffset = m_oReader.GetOffset();
                const uint32_t uCount = m_oReader.ReadU32();
                if (uCount > uMaxCount)
                {
                    throw Core::BinaryFormatError(std::format("{} count {} exceeds the maximum of {}", svWhat, uCount, uMaxCount), uOffset);
                }
                m_oReader.RequireElements(uCount, uElementSize);
                return uCount;
            }

            void ReadVec3Keys(std::vector<Vec3Key>& vOut)
            {
                vOut.resize(ReadCount(AnimationClipDescriptor::kMaxKeysPerComponent, kVec3KeySize, "key"));
                for (Vec3Key& oKey : vOut)
                {
                    oKey.fTimeSeconds = m_oReader.ReadF32();
                    const float fX = m_oReader.ReadF32();
                    const float fY = m_oReader.ReadF32();
                    const float fZ = m_oReader.ReadF32();
                    oKey.vecValue = {fX, fY, fZ};
                }
            }

            void ReadQuatKeys(std::vector<QuatKey>& vOut)
            {
                vOut.resize(ReadCount(AnimationClipDescriptor::kMaxKeysPerComponent, kQuatKeySize, "key"));
                for (QuatKey& oKey : vOut)
                {
                    oKey.fTimeSeconds = m_oReader.ReadF32();
                    const float fX = m_oReader.ReadF32();
                    const float fY = m_oReader.ReadF32();
                    const float fZ = m_oReader.ReadF32();
                    const float fW = m_oReader.ReadF32();
                    oKey.quatValue = glm::quat(fW, fX, fY, fZ);
                }
            }

            Core::BinaryReader& m_oReader;
        };
    }

    std::vector<std::byte> AnimationClipDescriptorSerializer::Encode(const AnimationClipDescriptor& oClip)
    {
        oClip.Validate();
        Core::BinaryWriter oWriter;
        ClipPayloadWriter(oWriter).Write(oClip);
        return Assets::AssetFile::Encode(AnimationClipAsset::kTypeId, kFormatVersion, oWriter.GetBytes());
    }

    AnimationClipDescriptor AnimationClipDescriptorSerializer::Decode(std::span<const std::byte> spanFile)
    {
        const Assets::AssetFile::Contents oContents = Assets::AssetFile::Decode(spanFile, AnimationClipAsset::kTypeId);
        if (oContents.uFormatVersion != kFormatVersion)
        {
            throw Core::BinaryFormatError(std::format("unsupported animation clip format version {}, expected {}",
                                                      oContents.uFormatVersion,
                                                      kFormatVersion),
                                          Assets::AssetFile::kFormatVersionOffset);
        }

        Core::BinaryReader oReader(oContents.spanPayload, Assets::AssetFile::kPayloadOffset);
        AnimationClipDescriptor oClip = ClipPayloadReader(oReader).Read();
        if (!oReader.IsAtEnd())
        {
            throw Core::BinaryFormatError(std::format("{} unexpected trailing byte(s) in the animation clip payload",
                                                      oReader.GetRemaining()), oReader.GetOffset());
        }

        try
        {
            oClip.Validate();
        }
        catch (const std::invalid_argument& oError)
        {
            throw Core::BinaryFormatError(std::format("invalid animation clip data: {}", oError.what()), Assets::AssetFile::kPayloadOffset);
        }
        return oClip;
    }

    void AnimationClipDescriptorSerializer::Save(const std::filesystem::path& oPath, const AnimationClipDescriptor& oClip)
    {
        const std::vector<std::byte> vBytes = Encode(oClip);
        Core::FileBytes::WriteAtomically(oPath, vBytes);
    }
}
