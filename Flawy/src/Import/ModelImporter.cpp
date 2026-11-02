#include "Flawy/Import/ModelImporter.hpp"

#include "Import/IModelReader.hpp"
#include "Import/ImportLog.hpp"
#include "Import/ImportedSceneCommitter.hpp"

#include <stdexcept>
#include <utility>

namespace Flawy::Import
{
    ModelImporter::ModelImporter(std::unique_ptr<IModelReader> upReader)
        : m_upReader(std::move(upReader))
    {
        if (!m_upReader)
        {
            throw std::invalid_argument("ModelImporter requires a model reader");
        }
    }

    ModelImporter::~ModelImporter() = default;

    std::span<const std::string_view> ModelImporter::GetExtensions() const noexcept
    {
        return m_upReader->GetExtensions();
    }

    Assets::AssetImportResult ModelImporter::Import(const Assets::AssetImportContext& oContext)
    {
        ImportedScene oScene = m_upReader->Read(oContext.oSourcePath, oContext.oOptions);
        for (const std::string& sWarning : oScene.vWarnings)
        {
            ImportLog::Warn("Import '{}' ({}): {}", oContext.sSourceAssetPath, m_upReader->GetFormatName(), sWarning);
        }
        return ImportedSceneCommitter(oContext).Commit(std::move(oScene));
    }
}
