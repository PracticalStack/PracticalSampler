#include "drs/engine/RuntimeLoader.h"

#include <json/json.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;
using json = nlohmann::json;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

RuntimeProjectModel migrateReferenceProjectToSchemaNine()
{
    auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Layer schema persistence requires the Phase 2 reference project.");

    auto project = loaded.project;
    const auto curated = migrateRuntimeProjectToCuratedDspSchema(project);
    require(curated.valid, "Reference project must migrate through the curated DSP schema.");
    project = curated.project;

    const auto articulations = migrateRuntimeProjectToPerformanceArticulationSchema(project);
    require(articulations.valid, "Reference project must migrate through the articulation schema.");
    project = articulations.project;

    const auto dampers = migrateRuntimeProjectToContinuousDamperSchema(project);
    require(dampers.valid, "Reference project must migrate through the continuous damper schema.");
    project = dampers.project;

    const auto playback = migrateRuntimeProjectToPlaybackRegionSchema(project);
    require(playback.valid, "Reference project must migrate through the playback region schema.");
    project = playback.project;

    const auto loops = migrateRuntimeProjectToLoopCrossfadeSchema(project);
    require(loops.valid, "Reference project must migrate through the loop crossfade schema.");
    return loops.project;
}

void verifyMigrationCreatesDefaultLayer()
{
    const auto schemaNine = migrateReferenceProjectToSchemaNine();
    require(schemaNine.schemaVersion == loopCrossfadeProjectSchemaVersion
                && schemaNine.authoring.schemaVersion == loopCrossfadeAuthoringSchemaVersion,
            "Layer migration must start from the loop-crossfade project schema.");

    const auto migrated = migrateRuntimeProjectToLayerSchema(schemaNine);
    require(migrated.valid && migrated.migrated,
            "Schema-nine project must migrate successfully to the layer contract schema.");
    require(migrated.project.schemaVersion == layerContractProjectSchemaVersion
                && migrated.project.authoring.schemaVersion == layerContractAuthoringSchemaVersion,
            "Layer migration must advance project and authoring schema versions together.");
    require(migrated.project.authoring.layers.size() == 1
                && migrated.project.authoring.layers.front().id == "default-layer"
                && migrated.project.authoring.selectedLayerId == "default-layer",
            "Existing explicit groups must be placed in one selected default layer.");
    require(!migrated.project.authoring.groups.empty(),
            "Layer migration characterization requires existing explicit groups.");
    for (const auto& group : migrated.project.authoring.groups)
        require(group.layerId == "default-layer",
                "Every migrated group must reference the default layer.");

    const auto& layer = migrated.project.authoring.layers.front();
    require(layer.crossfade.source == LayerCrossfadeSource::none
                && !layer.crossfade.controllerNumber.has_value()
                && layer.crossfade.curve == LayerCrossfadeCurve::linear,
            "Migrated layers must default to no crossfade with the linear curve contract.");
}

void verifyLayerRoundTripAndValidation()
{
    auto project = migrateRuntimeProjectToLayerSchema(migrateReferenceProjectToSchemaNine()).project;
    project.authoring.layers.front().gainDb = -3.0;
    project.authoring.layers.front().pan = 0.25;
    project.authoring.layers.front().crossfade.source = LayerCrossfadeSource::controller;
    project.authoring.layers.front().crossfade.controllerNumber = 1;
    project.authoring.layers.front().crossfade.low = 32;
    project.authoring.layers.front().crossfade.high = 95;

    const auto text = serializeRuntimeProjectManifest(project, "layer-schema-roundtrip/project.drsproject");
    const auto root = json::parse(text);
    require(root.at("schemaVersion") == layerContractProjectSchemaVersion
                && root.at("authoring").at("schemaVersion") == layerContractAuthoringSchemaVersion
                && root.at("authoring").at("selectedLayerId") == "default-layer",
            "Serialized layer projects must emit the layer schema and selected layer.");
    require(root.at("authoring").at("layers").at(0).at("crossfade").at("source") == "controller"
                && root.at("authoring").at("layers").at(0).at("crossfade").at("controllerNumber") == 1,
            "Serialized layer crossfade metadata must retain its typed controller source.");
    require(root.at("authoring").at("groups").at(0).at("layerId") == "default-layer",
            "Serialized groups must retain child-side layer membership.");

    const auto reopened = parseRuntimeProjectManifest(text, "layer-schema-roundtrip/project.drsproject", false);
    if (!reopened.loaded)
    {
        const auto detail = reopened.issues.empty() ? std::string { "no loader issue" } : reopened.issues.front();
        throw std::runtime_error("Layer schema projects must reopen through the native loader: " + detail);
    }
    require(reopened.project.authoring.layers.size() == 1,
            "Layer schema projects must reopen with one layer.");
    require(reopened.project.authoring.layers.front().crossfade.controllerNumber == 1
                && reopened.project.authoring.layers.front().gainDb == -3.0
                && reopened.project.authoring.layers.front().pan == 0.25,
            "Layer crossfade and independent layer gain/pan values must survive round trip.");

    auto invalid = reopened.project;
    invalid.authoring.layers.front().crossfade.controllerNumber.reset();
    const auto validation = validateRuntimeProjectModel(invalid);
    require(!validation.valid,
            "A controller layer crossfade without a controller number must fail validation.");
}
} // namespace

int main()
{
    try
    {
        verifyMigrationCreatesDefaultLayer();
        verifyLayerRoundTripAndValidation();
        std::cout << "Layer schema persistence tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Layer schema persistence tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
