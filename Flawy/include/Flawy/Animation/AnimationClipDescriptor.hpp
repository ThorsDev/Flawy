#pragma once

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace Flawy::Animation
{
    struct Vec3Key
    {
        float fTimeSeconds = 0.0f;
        glm::vec3 vecValue{0.0f};
    };

    struct QuatKey
    {
        float fTimeSeconds = 0.0f;
        glm::quat quatValue{1.0f, 0.0f, 0.0f, 0.0f};
    };

    struct JointTrackDescriptor
    {
        std::string sJointName;
        std::vector<Vec3Key> vTranslations;
        std::vector<QuatKey> vRotations;
        std::vector<Vec3Key> vScales;
    };

    struct AnimationClipDescriptor
    {
        static constexpr uint32_t kMaxTracks = 1024;
        static constexpr uint32_t kMaxKeysPerComponent = 1024u * 1024u;
        static constexpr uint32_t kMaxNameLength = 4096;

        std::string sName;
        std::string sSkeletonPath;
        float fDurationSeconds = 1.0f;
        std::vector<JointTrackDescriptor> vTracks;

        JointTrackDescriptor& AddTrack(std::string sJointName);
        void Validate() const;
    };
}
