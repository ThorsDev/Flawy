#include "Flawy/Animation/SkeletonDescriptorSerializer.hpp"

#include "Flawy/Animation/SkeletonAsset.hpp"
#include "Flawy/Assets/AssetFile.hpp"
#include "Flawy/Core/BinaryFormatError.hpp"
#include "Flawy/Core/BinaryReader.hpp"
#include "Flawy/Core/BinaryWriter.hpp"
#include "Flawy/Core/FileBytes.hpp"

#include <format>
#include <stdexcept>

namespace Flawy::Animation
{
    namespace
    {
        constexpr size_t kMinimumJointSize = 4 + 4 + 10 * sizeof(float) + 1;

        class SkeletonPayloadWriter final
        {
        public:
            explicit SkeletonPayloadWriter(Core::BinaryWriter& oWriter) noexcept
                : m_oWriter(oWriter)
            {
            }

            void Write(const SkeletonDescriptor& oSkeleton)
            {
                m_oWriter.WriteString(oSkeleton.sName);
                m_oWriter.WriteU32(static_cast<uint32_t>(oSkeleton.vJoints.size()));
                for (const JointDescriptor& oJoint : oSkeleton.vJoints)
                {
                    m_oWriter.WriteString(oJoint.sName);
                    m_oWriter.WriteI32(oJoint.iParentIndex);
                    WriteVec3(oJoint.oLocalRestPose.vecTranslation);
                    WriteQuat(oJoint.oLocalRestPose.quatRotation);
                    WriteVec3(oJoint.oLocalRestPose.vecScale);
                    m_oWriter.WriteBool(oJoint.optInverseBindMatrix.has_value());
                    if (oJoint.optInverseBindMatrix)
                    {
                        WriteMat4(*oJoint.optInverseBindMatrix);
                    }
                }
            }

        private:
            void WriteVec3(const glm::vec3& vecValue)
            {
                m_oWriter.WriteF32(vecValue.x);
                m_oWriter.WriteF32(vecValue.y);
                m_oWriter.WriteF32(vecValue.z);
            }

            void WriteQuat(const glm::quat& quatValue)
            {
                m_oWriter.WriteF32(quatValue.x);
                m_oWriter.WriteF32(quatValue.y);
                m_oWriter.WriteF32(quatValue.z);
                m_oWriter.WriteF32(quatValue.w);
            }

            void WriteMat4(const glm::mat4& matValue)
            {
                for (glm::length_t iColumn = 0; iColumn < 4; ++iColumn)
                {
                    for (glm::length_t iRow = 0; iRow < 4; ++iRow)
                    {
                        m_oWriter.WriteF32(matValue[iColumn][iRow]);
                    }
                }
            }

            Core::BinaryWriter& m_oWriter;
        };

        class SkeletonPayloadReader final
        {
        public:
            explicit SkeletonPayloadReader(Core::BinaryReader& oReader) noexcept
                : m_oReader(oReader)
            {
            }

            [[nodiscard]] SkeletonDescriptor Read()
            {
                SkeletonDescriptor oSkeleton;
                oSkeleton.sName = m_oReader.ReadString(SkeletonDescriptor::kMaxNameLength);

                const size_t uCountOffset = m_oReader.GetOffset();
                const uint32_t uJointCount = m_oReader.ReadU32();
                if (uJointCount == 0 || uJointCount > SkeletonDescriptor::kMaxJoints)
                {
                    throw Core::BinaryFormatError(std::format("joint count {} is outside [1, {}]",
                                                              uJointCount,
                                                              SkeletonDescriptor::kMaxJoints), uCountOffset);
                }
                m_oReader.RequireElements(uJointCount, kMinimumJointSize);

                oSkeleton.vJoints.resize(uJointCount);
                for (JointDescriptor& oJoint : oSkeleton.vJoints)
                {
                    oJoint.sName = m_oReader.ReadString(SkeletonDescriptor::kMaxNameLength);
                    oJoint.iParentIndex = m_oReader.ReadI32();
                    oJoint.oLocalRestPose.vecTranslation = ReadVec3();
                    oJoint.oLocalRestPose.quatRotation = ReadQuat();
                    oJoint.oLocalRestPose.vecScale = ReadVec3();
                    if (m_oReader.ReadBool())
                    {
                        oJoint.optInverseBindMatrix = ReadMat4();
                    }
                }
                return oSkeleton;
            }

        private:
            [[nodiscard]] glm::vec3 ReadVec3()
            {
                const float fX = m_oReader.ReadF32();
                const float fY = m_oReader.ReadF32();
                const float fZ = m_oReader.ReadF32();
                return {fX, fY, fZ};
            }

            [[nodiscard]] glm::quat ReadQuat()
            {
                const float fX = m_oReader.ReadF32();
                const float fY = m_oReader.ReadF32();
                const float fZ = m_oReader.ReadF32();
                const float fW = m_oReader.ReadF32();
                return {fW, fX, fY, fZ};
            }

            [[nodiscard]] glm::mat4 ReadMat4()
            {
                glm::mat4 matValue(1.0f);
                for (glm::length_t iColumn = 0; iColumn < 4; ++iColumn)
                {
                    for (glm::length_t iRow = 0; iRow < 4; ++iRow)
                    {
                        matValue[iColumn][iRow] = m_oReader.ReadF32();
                    }
                }
                return matValue;
            }

            Core::BinaryReader& m_oReader;
        };
    }

    std::vector<std::byte> SkeletonDescriptorSerializer::Encode(const SkeletonDescriptor& oSkeleton)
    {
        oSkeleton.Validate();
        Core::BinaryWriter oWriter;
        SkeletonPayloadWriter(oWriter).Write(oSkeleton);
        return Assets::AssetFile::Encode(SkeletonAsset::kTypeId, kFormatVersion, oWriter.GetBytes());
    }

    SkeletonDescriptor SkeletonDescriptorSerializer::Decode(std::span<const std::byte> spanFile)
    {
        const Assets::AssetFile::Contents oContents = Assets::AssetFile::Decode(spanFile, SkeletonAsset::kTypeId);
        if (oContents.uFormatVersion != kFormatVersion)
        {
            throw Core::BinaryFormatError(std::format("unsupported skeleton format version {}, expected {}",
                                                      oContents.uFormatVersion,
                                                      kFormatVersion),
                                          Assets::AssetFile::kFormatVersionOffset);
        }

        Core::BinaryReader oReader(oContents.spanPayload, Assets::AssetFile::kPayloadOffset);
        SkeletonDescriptor oSkeleton = SkeletonPayloadReader(oReader).Read();
        if (!oReader.IsAtEnd())
        {
            throw Core::BinaryFormatError(std::format("{} unexpected trailing byte(s) in the skeleton payload",
                                                      oReader.GetRemaining()), oReader.GetOffset());
        }

        try
        {
            oSkeleton.Validate();
        }
        catch (const std::invalid_argument& oError)
        {
            throw Core::BinaryFormatError(std::format("invalid skeleton data: {}", oError.what()), Assets::AssetFile::kPayloadOffset);
        }
        return oSkeleton;
    }

    void SkeletonDescriptorSerializer::Save(const std::filesystem::path& oPath, const SkeletonDescriptor& oSkeleton)
    {
        const std::vector<std::byte> vBytes = Encode(oSkeleton);
        Core::FileBytes::WriteAtomically(oPath, vBytes);
    }
}
