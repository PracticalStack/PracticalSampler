#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimePresetState.h"
#include "drs/engine/PackageWriter.h"
#include "shared/ProjectStorage.h"

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        RuntimeProjectModel project;
        project.schemaName = "drs.project";
        project.schemaVersion = instrumentControlProjectSchemaVersion;
        project.projectId = "instrument-control-round-trip";
        project.displayName = "Instrument Control Round Trip";
        project.contentRootPath = "content";
        project.defaultInstrumentManifestPath = "instrument.drsinst";
        project.authoring.schemaName = "drs.authoring";
        project.authoring.schemaVersion = instrumentControlAuthoringSchemaVersion;
        RuntimeProjectArticulationDefinition articulation;
        articulation.id = "default";
        articulation.displayName = "Default";
        articulation.isDefault = true;
        articulation.displayOrder = 0;
        project.authoring.articulations.push_back(articulation);

        RuntimeProjectInstrumentControlDefinition control;
        control.id = "mixer.kick";
        control.displayName = "Kick In";
        control.category = RuntimeInstrumentControlCategory::mixer;
        control.kind = RuntimeInstrumentControlKind::decibels;
        control.unit = RuntimeInstrumentControlUnit::decibels;
        control.normalizedDefault = 0.75;
        control.displayMinimum = -96.0;
        control.displayMaximum = 6.0;
        control.displayPrecision = 1;
        control.section = "Kit Pieces";
        control.provenance = RuntimeInstrumentControlProvenance::importedSfz;
        control.importedSourceController = 20;
        project.authoring.instrumentControls.push_back(control);

        RuntimeProjectInstrumentControlTargetDefinition target;
        target.id = "target.mixer.kick.gain";
        target.controlId = control.id;
        target.ownerKind = "group";
        target.ownerId = "kick";
        target.targetKind = RuntimeInstrumentControlTargetKind::gain;
        target.parameterId = "gainDb";
        target.destinationMinimum = -96.0;
        target.destinationMaximum = 6.0;
        target.contributionMode = RuntimeInstrumentControlContributionMode::multiply;
        project.authoring.instrumentControlTargets.push_back(target);

        RuntimeProjectMidiControlBindingDefinition binding;
        binding.id = "binding.mixer.kick.cc20";
        binding.controlId = control.id;
        binding.controllerNumber = 20;
        binding.imported = true;
        binding.importedSourceController = 20;
        project.authoring.midiControlBindings.push_back(binding);

        const auto validation = validateRuntimeProjectModel(project);
        require(validation.valid, "The schema 11 control project must validate before serialization.");

        const auto serialized = serializeRuntimeProjectManifest(project, "round-trip.drsproj");
        require(serialized.find("instrumentControls") != std::string::npos
                    && serialized.find("midiControlBindings") != std::string::npos,
                "Schema 11 serialization must include control and binding arrays.");

        const auto parsed = parseRuntimeProjectManifest(serialized, "round-trip.drsproj", false);
        require(parsed.loaded, "A serialized schema 11 control project must reload successfully.");
        require(parsed.project.authoring.instrumentControls.size() == 1
                    && parsed.project.authoring.instrumentControls.front().id == control.id
                    && parsed.project.authoring.instrumentControlTargets.size() == 1
                    && parsed.project.authoring.midiControlBindings.size() == 1
                    && parsed.project.authoring.midiControlBindings.front().controllerNumber == 20,
                "Control identity, target identity, and MIDI binding source must survive round-trip.");

        const auto companionInstrument = drs::app::buildInstrumentManifestForProject(
            parsed.project, juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("round-trip.drsproj"));
        require(companionInstrument.schemaVersion == instrumentControlInstrumentSchemaVersion
                    && companionInstrument.instrumentControls.size() == 1
                    && companionInstrument.instrumentControlTargets.size() == 1
                    && companionInstrument.midiControlBindings.size() == 1,
                "Saving a schema 11 project must project controls, targets, and MIDI bindings into its companion runtime instrument.");
        auto legacyZeroDefaultProject = parsed.project;
        legacyZeroDefaultProject.authoring.instrumentControls.front().normalizedDefault = 0.0;
        const auto repairedCompanion = drs::app::buildInstrumentManifestForProject(
            legacyZeroDefaultProject,
            juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("legacy-zero-default.drsproj"));
        require(repairedCompanion.instrumentControls.front().normalizedDefault == 1.0,
                "Companion runtime generation must repair imported SFZ gain defaults created by the previous importer.");

        AuthoringSession session(parsed.project);
        auto secondControl = control;
        secondControl.id = "mixer.snare";
        secondControl.displayName = "Snare";
        const auto created = session.createInstrumentControl(secondControl, "Create Snare Control");
        require(created.applied && session.getInstrumentControls().size() == 2,
                "Instrument control creation must be an undoable authoring transaction.");
        auto secondBinding = binding;
        secondBinding.id = "binding.mixer.snare.cc21";
        secondBinding.controlId = secondControl.id;
        secondBinding.controllerNumber = 21;
        const auto bound = session.upsertMidiControlBinding(secondBinding, "Assign Snare CC");
        require(bound.applied && session.getMidiControlBindings().size() == 2,
                "MIDI binding assignment must be an undoable authoring transaction.");
        secondBinding.channelScope.kind = RuntimeMidiChannelScopeKind::exact;
        secondBinding.channelScope.channel = 2;
        const auto replaced = session.upsertMidiControlBinding(secondBinding, "Replace Snare CC channel");
        require(replaced.applied && session.getMidiControlBindings().back().channelScope.channel == 2,
                "MIDI binding replacement must preserve identity while updating channel scope.");
        auto conflict = secondBinding;
        conflict.id = "binding.conflict";
        conflict.controlId = control.id;
        conflict.controllerNumber = 21;
        conflict.channelScope = {};
        const auto rejected = session.upsertMidiControlBinding(conflict, "Reject conflicting CC");
        require(!rejected.applied,
                "Overlapping any-channel MIDI sources must be rejected by the authoring transaction.");
        const auto undone = session.undo();
        require(undone.applied && session.getMidiControlBindings().size() == 2,
                "Undo must revert the most recent MIDI binding replacement transaction.");

        auto legacy = parsed.project;
        legacy.schemaVersion = layerContractProjectSchemaVersion;
        legacy.authoring.schemaVersion = layerContractAuthoringSchemaVersion;
        legacy.authoring.instrumentControls.clear();
        legacy.authoring.instrumentControlTargets.clear();
        legacy.authoring.midiControlBindings.clear();
        legacy.authoring.controllerDefaults = { { 20, 96 } };
        const auto migrated = migrateRuntimeProjectToInstrumentControlSchema(legacy);
        require(migrated.migrated
                    && migrated.project.schemaVersion == instrumentControlProjectSchemaVersion
                    && migrated.project.authoring.instrumentControls.size() == 1
                    && migrated.project.authoring.instrumentControls.front().importedSourceController == 20
                    && migrated.project.authoring.midiControlBindings.size() == 1,
                "Schema 10 projects with controller defaults must migrate to stable hidden CC controls and bindings.");

        RuntimeInstrumentModel instrument;
        instrument.schemaName = "drs.instrument";
        instrument.schemaVersion = instrumentControlInstrumentSchemaVersion;
        instrument.instrumentId = "instrument-control-round-trip";
        instrument.instrumentControls = parsed.project.authoring.instrumentControls;
        instrument.instrumentControlTargets = parsed.project.authoring.instrumentControlTargets;
        instrument.midiControlBindings = parsed.project.authoring.midiControlBindings;
        RuntimeArticulationDefinition runtimeArticulation;
        runtimeArticulation.id = "default";
        runtimeArticulation.isDefault = true;
        instrument.articulations.push_back(runtimeArticulation);
        RuntimeSessionStateSnapshot sessionState;
        sessionState.instrumentControlValues.push_back({ control.id, 0.42 });
        auto preset = captureRuntimePresetState(sessionState);
        preset.schemaName = "drs.presetState";
        preset.schemaVersion = 1;
        preset.presetId = "control-values";
        preset.targetInstrumentId = instrument.instrumentId;
        preset.targetInstrumentSchemaName = instrument.schemaName;
        preset.targetInstrumentSchemaVersion = instrument.schemaVersion;
        preset.loadProfileId = "balanced";
        preset.selectedArticulationId = "default";
        const auto presetText = serializeRuntimePresetState(preset);
        const auto parsedPreset = parseRuntimePresetState(presetText);
        require(parsedPreset.loaded && parsedPreset.preset.instrumentControlValues.size() == 1
                    && parsedPreset.preset.instrumentControlValues.front().id == control.id
                    && parsedPreset.preset.instrumentControlValues.front().normalizedValue == 0.42,
                "Current instrument control values must survive preset serialization and parsing.");
        const auto presetValidation = validateRuntimePresetState(parsedPreset.preset, instrument);
        require(presetValidation.valid,
                "A preset value identified by a known control must validate.");
        auto missingControlValues = parsedPreset.preset;
        missingControlValues.instrumentControlValues.clear();
        require(validateRuntimePresetState(missingControlValues, instrument).valid,
                "A preset may omit current control values and must fall back to authored defaults.");
        auto unknownControlValue = parsedPreset.preset;
        unknownControlValue.instrumentControlValues.push_back({ "removed.control", 0.5 });
        require(!validateRuntimePresetState(unknownControlValue, instrument).valid,
                "A preset referencing an unknown stable control ID must fail closed.");

        instrument.displayName = "Instrument Control Package";
        instrument.defaultLoadProfile = "balanced";
        RuntimeGroupDefinition packageGroup;
        packageGroup.id = "default-group";
        packageGroup.name = "Default";
        instrument.groups.push_back(packageGroup);
        RuntimeZoneDefinition packageZone;
        packageZone.id = "default-zone";
        packageZone.groupId = packageGroup.id;
        packageZone.articulationId = "default";
        packageZone.samplePath = "sample.wav";
        packageZone.streamAssetPath = "stream.drsstream";
        instrument.zones.push_back(packageZone);
        const auto streamAssetPath = std::filesystem::temp_directory_path() / "stream.drsstream";
        std::ofstream(streamAssetPath, std::ios::binary | std::ios::trunc) << "stream";
        instrument.compiledStreamAssetPath = streamAssetPath.generic_string();
        const auto instrumentPath = std::filesystem::temp_directory_path() / "package-runtime.drinst";
        const auto instrumentJson = serializeRuntimeInstrumentManifest(instrument,
                                                                        instrumentPath.generic_string());
        const auto packagePath = std::filesystem::temp_directory_path()
            / "drs-instrument-control-round-trip.drpkg";
        PerformancePackageWritePlan packagePlan;
        packagePlan.outputPackagePath = packagePath.generic_string();
        packagePlan.manifest.packageId = "instrument-control-package";
        packagePlan.manifest.displayName = instrument.displayName;
        packagePlan.manifest.instrumentId = instrument.instrumentId;
        packagePlan.payloads.push_back({ "runtime-instrument",
                                         PerformancePackagePayloadKind::runtimeInstrument,
                                         "manifest/runtime-instrument.drinst",
                                         "application/json",
                                         std::vector<std::uint8_t>(instrumentJson.begin(), instrumentJson.end()) });
        const auto written = writePerformancePackage(packagePlan);
        require(written.written, "Instrument controls must survive package writing.");
        const auto inspected = inspectPerformancePackage(packagePath.generic_string());
        require(inspected.valid, "Instrument control package must reopen successfully.");
        const auto payload = std::find_if(inspected.payloads.begin(), inspected.payloads.end(),
                                          [](const auto& candidate)
                                          {
                                              return candidate.payloadKind == "runtimeInstrument";
                                          });
        require(payload != inspected.payloads.end(), "Package must contain the runtime instrument payload.");
        const std::string reopenedJson(payload->plaintextBytes.begin(), payload->plaintextBytes.end());
        const auto reopenedInstrument = parseRuntimeInstrumentManifest(
            reopenedJson, "package://manifest/runtime-instrument.drinst", false);
        require(reopenedInstrument.loaded
                    && reopenedInstrument.instrument.instrumentControls.size() == 1
                    && reopenedInstrument.instrument.midiControlBindings.size() == 1,
                "Packaged instrument controls and bindings must reload by stable identity.");
        std::error_code packageError;
        std::filesystem::remove(packagePath, packageError);
        std::filesystem::remove(streamAssetPath, packageError);
        std::filesystem::remove(instrumentPath, packageError);

        std::cout << "Instrument control persistence tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Instrument control persistence tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
