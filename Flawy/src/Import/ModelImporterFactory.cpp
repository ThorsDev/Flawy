#include "Flawy/Import/ModelImporterFactory.hpp"

#include "Import/FbxModelReader.hpp"
#include "Import/GltfModelReader.hpp"
#include "Import/ObjModelReader.hpp"

namespace Flawy::Import
{
    std::unique_ptr<ModelImporter> ModelImporterFactory::CreateGltfImporter()
    {
        return std::make_unique<ModelImporter>(std::make_unique<GltfModelReader>());
    }

    std::unique_ptr<ModelImporter> ModelImporterFactory::CreateFbxImporter()
    {
        return std::make_unique<ModelImporter>(std::make_unique<FbxModelReader>());
    }

    std::unique_ptr<ModelImporter> ModelImporterFactory::CreateObjImporter()
    {
        return std::make_unique<ModelImporter>(std::make_unique<ObjModelReader>());
    }
}
