#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
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
    return fs::path(drs::engine::getPhase1ReferenceProjectManifestPath()).parent_path()
        / ".." / ".." / ".." / ".." / "hise_project";
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

    project.sampleSources.push_back({ "sine-a3", "Samples/DRS_Sine_A3.wav", "core-sustain" });
    project.sampleSources.push_back({ "triangle-a4", "Samples/DRS_TriangleLead_A4.wav", "core-lead" });

    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    project.authoring.articulations.push_back({ "sustain", "Sustain", true, 0, std::nullopt });
    project.authoring.articulations.push_back({ "lead", "Lead", false, 1, std::nullopt });
    project.authoring.groups.push_back({ "pad-core", "Pad Core", 0, true, 0.0, 0.0, {}, {} });
    project.authoring.groups.push_back({ "lead-core", "Lead Core", 1, true, 0.0, 0.0, {}, {} });

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
    project.authoring.zones.push_back(std::move(leadZone));

    return project;
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
        standalone.resized();
        require(standalone.getProcessor().getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::performancePackage,
                "Standalone shell should mark package sessions as performance-package documents.");
        require(!standalone.getProcessor().getWorkspaceDocumentState().authoringAvailable,
                "Standalone package sessions should suppress authoring.");
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
        require(standalone.exportStateJson().find("\"projectBinding\"") == std::string::npos,
                "Standalone package sessions should export preset-only host state without project bindings.");
        standalone.getProcessor().prepareToPlay(44100.0, 512);
        standalone.getProcessor().serviceMessageThreadWork();
        require(renderQueuedPerformanceSurfaceMagnitude(standalone.getProcessor(), 69, 0.8f) > 0.0001f,
                "Standalone package sessions should remain playable through the performance surface.");

        drs::plugin::Processor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor should construct for package-session tests.");
        editor->addToDesktop(0);
        editor->setVisible(true);
        const auto pluginLoad = processor.loadPerformancePackageWorkspace(
            juce::File(juce::String::fromUTF8(packagePath.c_str())));
        require(pluginLoad.loaded,
                "Plugin shell should load a valid playable package. state="
                    + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));
        editor->resized();
        require(processor.getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::performancePackage,
                "Plugin shell should mark package sessions as performance-package documents.");
        require(!processor.getWorkspaceDocumentState().authoringAvailable,
                "Plugin package sessions should suppress authoring.");
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
        processor.getStateInformation(pluginState);
        const std::string serializedState(static_cast<const char*>(pluginState.getData()), pluginState.getSize());
        require(serializedState.find("\"projectBinding\"") == std::string::npos,
                "Plugin package sessions should export preset-only host state without project bindings.");
        processor.prepareToPlay(44100.0, 512);
        processor.serviceMessageThreadWork();
        require(renderQueuedPerformanceSurfaceMagnitude(processor, 69, 0.8f) > 0.0001f,
                "Plugin package sessions should remain playable through the performance surface.");

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
