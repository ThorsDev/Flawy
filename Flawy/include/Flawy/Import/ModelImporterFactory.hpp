#pragma once

#include "Flawy/Import/ModelImporter.hpp"

#include <memory>

namespace Flawy::Import
{
    class ModelImporterFactory final
    {
    public:
        ModelImporterFactory() = delete;

        [[nodiscard]] static std::unique_ptr<ModelImporter> CreateGltfImporter();
        [[nodiscard]] static std::unique_ptr<ModelImporter> CreateFbxImporter();
        [[nodiscard]] static std::unique_ptr<ModelImporter> CreateObjImporter();
    };
}
