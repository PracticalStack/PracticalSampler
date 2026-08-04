#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/SampleImport.h"
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

drs::engine::RuntimeCompilePlan buildReferenceCompilePlan(const fs::path& outputDirectory)
{
    const auto projectPath = outputDirectory / "tiny-open-instrument.drsproj";
    const auto instrumentPath = outputDirectory / "tiny-open-instrument.drinst";
    const auto streamPath = outputDirectory / "tiny-open-instrument.drstrm";
    const auto contentRoot = fs::path(drs::engine::getPhase1ReferenceProjectManifestPath()).parent_path()
        / ".." / ".." / ".." / ".." / "hise_project";

    const auto sinePath = (contentRoot / "Samples" / "DRS_Sine_A3.wav").lexically_normal();
    const auto trianglePath = (contentRoot / "Samples" / "DRS_TriangleLead_A4.wav").lexically_normal();

    const auto sineImport = drs::engine::inspectSampleFile(sinePath.generic_string());
    require(sineImport.accepted, "Reference sine sample must inspect successfully before package-session tests run.");

    const auto triangleImport = drs::engine::inspectSampleFile(trianglePath.generic_string());
    require(triangleImport.accepted, "Reference triangle sample must inspect successfully before package-session tests run.");

    drs::engine::RuntimeCompilePlan plan;
    plan.outputProjectPath = projectPath.generic_string();
    plan.outputInstrumentPath = instrumentPath.generic_string();
    plan.outputStreamPath = streamPath.generic_string();
    plan.projectId = "drs.phase1.tiny-open-project";
    plan.projectDisplayName = "DRS Tiny Open Project";
    plan.contentRootPath = contentRoot.lexically_normal().generic_string();
    plan.instrumentId = "drs.phase1.tiny-open-instrument";
    plan.instrumentDisplayName = "DRS Tiny Open Instrument";
    plan.defaultLoadProfile = "balanced";
    plan.pageSizeBytes = 65536;

    drs::engine::RuntimeCompileSourceDefinition sineSource;
    sineSource.id = "sine-a3";
    sineSource.sourcePath = sinePath.generic_string();
    sineSource.role = "core-sustain";
    sineSource.metadata = sineImport.metadata;
    plan.sampleSources.push_back(std::move(sineSource));

    drs::engine::RuntimeCompileSourceDefinition triangleSource;
    triangleSource.id = "triangle-a4";
    triangleSource.sourcePath = trianglePath.generic_string();
    triangleSource.role = "core-lead";
    triangleSource.metadata = triangleImport.metadata;
    plan.sampleSources.push_back(std::move(triangleSource));

    drs::engine::RuntimeArticulationDefinition sustain;
    sustain.id = "sustain";
    sustain.name = "Sustain";
    sustain.isDefault = true;
    plan.articulations.push_back(std::move(sustain));

    drs::engine::RuntimeArticulationDefinition lead;
    lead.id = "lead";
    lead.name = "Lead";
    plan.articulations.push_back(std::move(lead));

    drs::engine::RuntimeGroupDefinition padCore;
    padCore.id = "pad-core";
    padCore.name = "Pad Core";
    padCore.articulationIds = { "sustain" };
    plan.groups.push_back(std::move(padCore));

    drs::engine::RuntimeGroupDefinition leadCore;
    leadCore.id = "lead-core";
    leadCore.name = "Lead Core";
    leadCore.articulationIds = { "lead" };
    plan.groups.push_back(std::move(leadCore));

    drs::engine::RuntimeCompileZoneDefinition padZone;
    padZone.id = "pad-a3";
    padZone.sourceId = "sine-a3";
    padZone.groupId = "pad-core";
    padZone.articulationId = "sustain";
    padZone.rootKey = 57;
    padZone.keyLow = 36;
    padZone.keyHigh = 76;
    padZone.velocityLow = 1;
    padZone.velocityHigh = 95;
    padZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(padZone));

    drs::engine::RuntimeCompileZoneDefinition leadZone;
    leadZone.id = "lead-a4";
    leadZone.sourceId = "triangle-a4";
    leadZone.groupId = "lead-core";
    leadZone.articulationId = "lead";
    leadZone.rootKey = 69;
    leadZone.keyLow = 60;
    leadZone.keyHigh = 96;
    leadZone.velocityLow = 1;
    leadZone.velocityHigh = 127;
    leadZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(leadZone));

    return plan;
}

std::string buildPackageFixture(const fs::path& scratchDirectory)
{
    auto compilePlan = buildReferenceCompilePlan(scratchDirectory / "compiled-runtime");
    auto compileResult = drs::engine::compileRuntimeInstrument(compilePlan);
    require(compileResult.compiled, "Reference compile plan should compile successfully for package-session tests.");

    const auto writeResult = drs::engine::writeCompiledStreamAssets(compileResult);
    require(writeResult.written, "Compiled stream assets should write successfully for package-session tests.");

    drs::engine::PerformancePackageManifest manifest;
    manifest.packageId = "drs.phase1.package-session";
    manifest.displayName = "Package Session Fixture";
    manifest.instrumentId = compilePlan.instrumentId;
    manifest.defaultLoadProfile = compilePlan.defaultLoadProfile;
    manifest.minimumReaderSchemaVersion = drs::engine::performancePackageSchemaVersion;
    manifest.notes = { "Sprint 5 package session fixture." };

    drs::engine::PerformancePackageCompileWritePlan packagePlan;
    packagePlan.manifest = std::move(manifest);
    packagePlan.compiledRuntime = std::move(compileResult);
    packagePlan.outputPackagePath = (scratchDirectory / "package-session-fixture.drpkg").generic_string();
    packagePlan.minimumCompatibleAppVersion = "0.5.0-internal";

    const auto packageWrite = drs::engine::writePerformancePackage(
        packagePlan,
        drs::engine::getDeterministicPackageCryptoProvider());
    require(packageWrite.written, "Package-session fixture should write successfully.");
    return packagePlan.outputPackagePath;
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
        const auto packagePath = buildPackageFixture(scratchDirectory);

        juce::ScopedJuceInitialiser_GUI gui;

        drs::standalone::MainComponent standalone(false);
        standalone.addToDesktop(0);
        standalone.setVisible(true);
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
