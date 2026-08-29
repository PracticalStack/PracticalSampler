#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"
#include "PerformancePackageExportSecurityTestSupport.h"

#include "drs/engine/PackageReaderDispatch.h"
#include "drs/engine/NativeContent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return "(none)";

    std::string summary;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            summary += " | ";
        summary += issues[index];
    }
    return summary;
}

std::optional<double> findMacroValue(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                     const std::string& macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const drs::engine::RuntimePresetMacroValue& macroValue)
                                       {
                                           return macroValue.id == macroId;
                                       });
    return iterator != sessionState.macroValues.end()
        ? std::optional<double>(iterator->value)
        : std::nullopt;
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;
    }

    return nullptr;
}

float renderQueuedPerformanceSurfaceMagnitude(drs::plugin::Processor& processor,
                                              int midiNoteNumber,
                                              float velocity,
                                              int blockCount = 4)
{
    processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);

    float maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    return maxMagnitude;
}

fs::path buildReferenceContentRoot()
{
    return fs::path(drs::engine::getNativeContentRoots().samplesRoot);
}

bool hasEmptyProjectBindingAndPackageBinding(const std::string& serializedState)
{
    const auto parsed = drs::engine::parseHostSessionState(serializedState);
    if (!parsed.isValidHostState()
        || !parsed.hostState.has_value()
        || !parsed.hostState->performancePackageBinding.has_value())
    {
        return false;
    }

    const auto& binding = parsed.hostState->projectBinding;
    return binding.projectId.empty()
        && binding.manifestPath.empty()
        && binding.manifestFileName.empty()
        && binding.manifestDigest.empty()
        && binding.contentRootHint.empty()
        && binding.portableRelativePath.empty();
}

drs::engine::RuntimeProjectModel buildAuthoringProjectFixture()
{
    const auto contentRoot = buildReferenceContentRoot().lexically_normal().generic_string();

    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "drs.phase1.package-export-project";
    project.displayName = "Package Export Fixture";
    project.contentRootPath = contentRoot;
    project.defaultInstrumentManifestPath = (buildReferenceContentRoot() / "PackageExportFixture.drinst").generic_string();
    project.notes = { "Sprint 6 playable package export fixture." };

    project.sampleSources.push_back({ "sine-a3", "DRS_Sine_A3.wav", "core-sustain" });
    project.sampleSources.push_back({ "triangle-a4", "DRS_TriangleLead_A4.wav", "core-lead" });

    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    project.authoring.masterGainDb = -1.5;
    project.authoring.articulations.push_back({ "sustain", "Sustain", true, 0, std::nullopt });
    project.authoring.articulations.push_back({ "lead", "Lead", false, 1, std::nullopt });
    project.authoring.groups.push_back({ "pad-core", {}, "Pad Core", 0, true, -3.0, 0.0, {}, {} });
    project.authoring.groups.push_back({ "lead-core", {}, "Lead Core", 1, true, 1.5, 0.0, {}, {} });

    drs::engine::RuntimeProjectZoneDefinition padZone;
    padZone.id = "pad-a3";
    padZone.sampleSourceId = "sine-a3";
    padZone.displayName = "Pad A3";
    padZone.groupId = "pad-core";
    padZone.articulationId = "sustain";
    padZone.rootKey = 57;
    padZone.keyLow = 36;
    padZone.keyHigh = 76;
    padZone.velocityLow = 1;
    padZone.velocityHigh = 95;
    padZone.gainDb = -0.75;
    padZone.releaseSeconds = 1.25;
    padZone.releaseShape = -6.0;
    project.authoring.zones.push_back(std::move(padZone));

    drs::engine::RuntimeProjectZoneDefinition leadZone;
    leadZone.id = "lead-a4";
    leadZone.sampleSourceId = "triangle-a4";
    leadZone.displayName = "Lead A4";
    leadZone.groupId = "lead-core";
    leadZone.articulationId = "lead";
    leadZone.rootKey = 69;
    leadZone.keyLow = 60;
    leadZone.keyHigh = 96;
    leadZone.velocityLow = 1;
    leadZone.velocityHigh = 127;
    leadZone.gainDb = 0.5;
    leadZone.sampleStartFrame = 64;
    project.authoring.zones.push_back(std::move(leadZone));

    return project;
}

drs::engine::RuntimeProjectModel buildUnloadedProjectState()
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = drs::engine::layerContractProjectSchemaVersion;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = drs::engine::layerContractAuthoringSchemaVersion;
    project.authoring.articulations = { { "default", "Default", true, 0, std::nullopt } };
    project.authoring.notes = { "Open a project or create a new one to begin authoring." };
    project.notes = { "This session starts without loading the checked-in reference project." };
    return project;
}

void addAuthoredFxRoutingGraph(drs::engine::RuntimeProjectModel& project)
{
    project.authoring.groups.front().routingBusId = "bus-group-pad-core";

    drs::engine::RuntimeProjectFxSlotDefinition drive;
    drive.id = "drive";
    drive.displayName = "Drive";
    drive.effectType = "drs.saturator";
    drive.effectVersion = 1;
    drive.bypassed = true;
    drive.parameters = {
        { "character", 0.0 },
        { "driveDb", 7.5 },
        { "tone", 0.55 },
        { "mix", 0.8 },
        { "outputDb", -1.0 }
    };
    project.authoring.fxSlots.push_back(std::move(drive));
    project.authoring.routingBuses.push_back(
        { "bus-group-pad-core", "Pad Core Insert", "groups/pad-core", { "drive" }, true });
}
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = fs::temp_directory_path() / "drs-phase1-performance-package-session-tests";
        std::error_code errorCode;
        fs::remove_all(scratchDirectory, errorCode);
        fs::create_directories(scratchDirectory);

        juce::ScopedJuceInitialiser_GUI gui;

        drs::standalone::MainComponent standalone(false);
        standalone.addToDesktop(0);
        standalone.setVisible(true);
        const auto exportSecurity = drs::tests::makePerformancePackageExportTestSecurityContext();
        require(standalone.getProcessor().getPerformancePackageExportService().setSecurityContext(
                    exportSecurity),
                "Standalone shell should accept the qualified V3 export security fixture.");
        drs::engine::PerformancePackageV3ActivationSecurityContext activationSecurity;
        activationSecurity.compatibilityId = exportSecurity->compatibilityId;
        activationSecurity.keyProvider = exportSecurity->keyProvider;
        activationSecurity.trustStore = exportSecurity->trustStore;

        auto graphProject = buildAuthoringProjectFixture();
        addAuthoredFxRoutingGraph(graphProject);
        require(standalone.getProcessor().replaceAuthoringProject(std::move(graphProject)),
                "Standalone shell should accept the graph-bearing export fixture.");
        const auto graphPackagePath = (scratchDirectory / "exported-fx-routing-fixture.drpkg")
            .generic_string();
        const auto graphExport = standalone.getProcessor().exportPerformancePackage(
            juce::File(juce::String::fromUTF8(graphPackagePath.c_str())));
        require(graphExport.exported,
                "The shell export adapter should delegate graph-bearing export to the shared pipeline. state="
                    + graphExport.state + " issues=" + summarizeIssues(graphExport.issues));
        const auto graphPackage = drs::engine::loadPerformancePackageV3Metadata(
            graphPackagePath, activationSecurity);
        require(graphPackage.loaded && graphPackage.metadata.loaded
                    && graphPackage.metadata.manifest.schemaVersion
                        == drs::engine::performancePackageFxRoutingSchemaVersion
                    && graphPackage.metadata.instrument.instrument.fxSlots.size() == 1
                    && graphPackage.metadata.instrument.instrument.routingBuses.size() == 1
                    && graphPackage.metadata.instrument.instrument.groups.front().routingBusId
                        == "bus-group-pad-core",
                "The shell adapter must emit the same graph metadata as the shared export service.");

        require(standalone.getProcessor().replaceAuthoringProject(buildAuthoringProjectFixture()),
                "Standalone shell should accept the export-fixture authoring project.");
        const auto packagePath = (scratchDirectory / "exported-session-fixture.drpkg").generic_string();
        const auto standaloneExport = standalone.getProcessor().exportPerformancePackage(
            juce::File(juce::String::fromUTF8(packagePath.c_str())));
        require(standaloneExport.exported,
                "Standalone shell should export a valid playable package from the authoring fixture. state="
                    + standaloneExport.state + " issues=" + summarizeIssues(standaloneExport.issues));
        const auto standaloneLoad = standalone.getProcessor().loadPerformancePackageWorkspace(
            juce::File(juce::String::fromUTF8(packagePath.c_str())));
        require(standaloneLoad.loaded,
                "Standalone shell should load a valid playable package. state="
                    + standaloneLoad.state + " issues=" + summarizeIssues(standaloneLoad.issues));
        const auto standalonePayload = standalone.getProcessor().getEngineFacade().getPerformancePackageActivationPayload();
        require(standalonePayload != nullptr && standalonePayload->snapshot != nullptr
                    && standalonePayload->prepared != nullptr,
                "Standalone package sessions should retain an activation payload after load.");
        require(std::abs(standalonePayload->snapshot->masterGainDb - (-1.5)) < 1.0e-9,
                "Standalone package sessions should preserve exported master gain.");
        require(std::abs(standalonePayload->snapshot->groupRoutes.at(0).gainDb - (-3.0)) < 1.0e-9,
                "Standalone package sessions should preserve exported group gain.");
        require(std::abs(standalonePayload->snapshot->zones.at(0).gainDb - (-0.75)) < 1.0e-9,
                "Standalone package sessions should preserve exported zone gain.");
        require(std::abs(standalonePayload->snapshot->zones.at(0).releaseSeconds - 1.25) < 1.0e-9,
                "Standalone package sessions should preserve exported zone release time.");
        require(std::abs(standalonePayload->snapshot->zones.at(0).releaseShape - (-6.0)) < 1.0e-9,
                "Standalone package sessions should preserve exported zone release shape.");
        require(standalonePayload->snapshot->zones.at(1).sampleStartFrame == 64,
                "Standalone package sessions should preserve exported sample start offsets.");
        require(std::abs(standalonePayload->prepared->masterGainDb - (-1.5)) < 1.0e-9,
                "Standalone prepared playback should preserve exported master gain.");
        require(std::abs(standalonePayload->prepared->groupRoutes.at(0).gainDb - (-3.0)) < 1.0e-9,
                "Standalone prepared playback should preserve exported group gain.");
        require(std::abs(standalonePayload->prepared->zones.at(0).gainDb - (-0.75)) < 1.0e-9,
                "Standalone prepared playback should preserve exported zone gain.");
        require(std::abs(standalonePayload->prepared->zones.at(0).releaseSeconds - 1.25) < 1.0e-9,
                "Standalone prepared playback should preserve exported zone release time.");
        require(std::abs(standalonePayload->prepared->zones.at(0).releaseShape - (-6.0)) < 1.0e-9,
                "Standalone prepared playback should preserve exported zone release shape.");
        require(standalonePayload->prepared->zones.at(1).sampleStartFrame == 64,
                "Standalone prepared playback should preserve exported sample start offsets.");
        standalone.resized();
        require(standalone.getProcessor().getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::performancePackage,
                "Standalone shell should mark package sessions as performance-package documents.");
        require(!standalone.getProcessor().getWorkspaceDocumentState().authoringAvailable,
                "Standalone package sessions should suppress authoring.");
        require(standalone.getProcessor().getWorkspaceDocumentState().readiness
                    == drs::engine::PackageSessionReadiness::playable
                    && standalone.getProcessor().getWorkspaceDocumentState().playable,
                "Standalone package sessions must become playable only after immutable activation succeeds.");
        auto* standaloneStatus = dynamic_cast<juce::Label*>(
            findDescendantById(standalone, "standaloneWorkspaceStatusLabel"));
        require(standaloneStatus != nullptr, "Standalone shell should expose a package-session status label.");
        require(standaloneStatus->getText().contains("Playable package")
                    && standaloneStatus->getText().contains("Read-only")
                    && standaloneStatus->getText().contains("Reader v1"),
                "Standalone package status label should explain package, read-only, and reader compatibility.");
        auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(
            findDescendantById(standalone, "workspaceTabs"));
        require(standaloneTabs != nullptr && standaloneTabs->getNumTabs() == 1,
                "Standalone package sessions should expose only the Perform tab.");
        require(findDescendantById(standalone, "authoringZoneSelector") == nullptr,
                "Standalone package sessions should not expose authoring descendants.");
        require(standalone.getProcessor().waitForHostStatePublication(),
                "Standalone package session state did not reach background host-state publication.");
        require(hasEmptyProjectBindingAndPackageBinding(standalone.exportStateJson()),
                "Standalone package sessions should export package-bound host state without a project binding.");
        const auto& standaloneSessionState = standalone.getProcessor().getEngineFacade().getCurrentSessionState();
        require(!findMacroValue(standaloneSessionState, "motion").has_value(),
                "Standalone package sessions should not reintroduce Motion when the exported package omitted macros.");
        standalone.getProcessor().prepareToPlay(44100.0, 512);
        standalone.getProcessor().serviceMessageThreadWork();
        require(renderQueuedPerformanceSurfaceMagnitude(standalone.getProcessor(), 69, 0.8f) > 0.0001f,
                "Standalone package sessions should remain playable through the performance surface.");

        drs::plugin::Processor processor;
        require(processor.setPerformancePackageActivationSecurityContext(
                    std::make_shared<const drs::engine::PerformancePackageV3ActivationSecurityContext>(
                        activationSecurity)),
                "Plugin shell should accept the qualified V3 activation security fixture.");
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor should construct for package-session tests.");
        editor->addToDesktop(0);
        editor->setVisible(true);
        const auto pluginLoad = processor.loadPerformancePackageWorkspace(
            juce::File(juce::String::fromUTF8(packagePath.c_str())));
        require(pluginLoad.loaded,
                "Plugin shell should load a valid playable package. state="
                    + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));
        const auto pluginPayload = processor.getEngineFacade().getPerformancePackageActivationPayload();
        require(pluginPayload != nullptr && pluginPayload->snapshot != nullptr
                    && pluginPayload->prepared != nullptr,
                "Plugin package sessions should retain an activation payload after load.");
        require(std::abs(pluginPayload->snapshot->masterGainDb - (-1.5)) < 1.0e-9,
                "Plugin package sessions should preserve exported master gain.");
        require(std::abs(pluginPayload->snapshot->groupRoutes.at(0).gainDb - (-3.0)) < 1.0e-9,
                "Plugin package sessions should preserve exported group gain.");
        require(std::abs(pluginPayload->snapshot->zones.at(0).gainDb - (-0.75)) < 1.0e-9,
                "Plugin package sessions should preserve exported zone gain.");
        require(std::abs(pluginPayload->snapshot->zones.at(0).releaseSeconds - 1.25) < 1.0e-9,
                "Plugin package sessions should preserve exported zone release time.");
        require(std::abs(pluginPayload->snapshot->zones.at(0).releaseShape - (-6.0)) < 1.0e-9,
                "Plugin package sessions should preserve exported zone release shape.");
        require(pluginPayload->snapshot->zones.at(1).sampleStartFrame == 64,
                "Plugin package sessions should preserve exported sample start offsets.");
        require(std::abs(pluginPayload->prepared->masterGainDb - (-1.5)) < 1.0e-9,
                "Plugin prepared playback should preserve exported master gain.");
        require(std::abs(pluginPayload->prepared->groupRoutes.at(0).gainDb - (-3.0)) < 1.0e-9,
                "Plugin prepared playback should preserve exported group gain.");
        require(std::abs(pluginPayload->prepared->zones.at(0).gainDb - (-0.75)) < 1.0e-9,
                "Plugin prepared playback should preserve exported zone gain.");
        require(std::abs(pluginPayload->prepared->zones.at(0).releaseSeconds - 1.25) < 1.0e-9,
                "Plugin prepared playback should preserve exported zone release time.");
        require(std::abs(pluginPayload->prepared->zones.at(0).releaseShape - (-6.0)) < 1.0e-9,
                "Plugin prepared playback should preserve exported zone release shape.");
        require(pluginPayload->prepared->zones.at(1).sampleStartFrame == 64,
                "Plugin prepared playback should preserve exported sample start offsets.");
        editor->resized();
        require(processor.getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::performancePackage,
                "Plugin shell should mark package sessions as performance-package documents.");
        require(!processor.getWorkspaceDocumentState().authoringAvailable,
                "Plugin package sessions should suppress authoring.");
        require(processor.getWorkspaceDocumentState().readiness
                    == drs::engine::PackageSessionReadiness::playable
                    && processor.getWorkspaceDocumentState().playable,
                "Plugin package sessions must become playable only after immutable activation succeeds.");
        auto* pluginStatus = dynamic_cast<juce::Label*>(
            findDescendantById(*editor, "pluginProjectStatusLabel"));
        require(pluginStatus != nullptr, "Plugin shell should expose a package-session status label.");
        require(pluginStatus->getText().contains("Playable package")
                    && pluginStatus->getText().contains("Read-only")
                    && pluginStatus->getText().contains("Reader v1"),
                "Plugin package status label should explain package, read-only, and reader compatibility.");
        auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(
            findDescendantById(*editor, "workspaceTabs"));
        require(pluginTabs != nullptr && pluginTabs->getNumTabs() == 1,
                "Plugin package sessions should expose only the Perform tab.");
        require(findDescendantById(*editor, "authoringZoneSelector") == nullptr,
                "Plugin package sessions should not expose authoring descendants.");
        juce::MemoryBlock pluginState;
        require(processor.waitForHostStatePublication(),
                "Package session state did not reach background host-state publication.");
        processor.getStateInformation(pluginState);
        const std::string serializedState(static_cast<const char*>(pluginState.getData()), pluginState.getSize());
        require(hasEmptyProjectBindingAndPackageBinding(serializedState),
                "Plugin package sessions should export package-bound host state without a project binding.");
        const auto& pluginSessionState = processor.getEngineFacade().getCurrentSessionState();
        require(!findMacroValue(pluginSessionState, "motion").has_value(),
                "Plugin package sessions should not reintroduce Motion when the exported package omitted macros.");
        processor.prepareToPlay(44100.0, 512);
        processor.serviceMessageThreadWork();
        require(renderQueuedPerformanceSurfaceMagnitude(processor, 69, 0.8f) > 0.0001f,
                "Plugin package sessions should remain playable through the performance surface.");

        processor.closePerformancePackageWorkspace(buildUnloadedProjectState());
        juce::AudioBuffer<float> closeBuffer(2, 512);
        closeBuffer.clear();
        juce::MidiBuffer closeMidi;
        processor.processBlock(closeBuffer, closeMidi);
        const auto closedPerformanceContext = processor.getPerformancePlaybackContextSnapshot();
        require(processor.getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::authoringProject
                    && processor.getWorkspaceDocumentState().authoringAvailable,
                "Closing a playable package should restore the authoring workspace.");
        require(processor.getEngineFacade().getPerformancePackageActivationPayload() == nullptr
                    && processor.getEngineFacade().getPerformancePackageRenderModel() == nullptr,
                "Closing a playable package should release package activation and render-model payloads.");
        require(!closedPerformanceContext.hasActiveActivation,
                "Closing a playable package must not leave its package activation active after the next audio block.");

        std::cout << "Phase 1 performance package session tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package session tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
