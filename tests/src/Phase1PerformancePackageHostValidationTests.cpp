#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"
#include "Phase1PerformancePackageSupport.h"
#include "drs/engine/PackageV2StreamingExport.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;
namespace package_support = drs::tests::performance_package;

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

float renderQueuedPerformanceSurfaceMagnitude(drs::plugin::Processor& processor,
                                              const int midiNoteNumber,
                                              const float velocity,
                                              const int blockCount = 8,
                                              const bool paceAsAudioDevice = false)
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
        if (paceAsAudioDevice)
            std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    return maxMagnitude;
}

drs::plugin::PerformancePackageWorkspaceLoadResult loadThroughBackgroundPreparation(
    drs::plugin::Processor& processor,
    const juce::File& packageFile,
    std::chrono::microseconds& preparationElapsed,
    std::chrono::microseconds& activationElapsed)
{
    const auto preparationStarted = Clock::now();
    auto prepared = drs::plugin::preparePerformancePackageWorkspaceInBackground(
        packageFile.getFullPathName().toStdString());
    preparationElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now() - preparationStarted);
    if (!prepared.prepared)
    {
        drs::plugin::PerformancePackageWorkspaceLoadResult failed;
        failed.failureCategory = prepared.failureCategory;
        failed.state = prepared.state;
        failed.issues = prepared.issues;
        failed.timings = prepared.timings;
        return failed;
    }

    const auto activationStarted = Clock::now();
    auto activated = processor.activatePreparedPerformancePackageWorkspace(
        std::move(prepared.activation), packageFile);
    activationElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now() - activationStarted);
    return activated;
}

void requirePerformanceOnlyWorkspace(const drs::engine::WorkspaceDocumentState& state,
                                     const std::string& context)
{
    require(state.kind == drs::engine::WorkspaceDocumentKind::performancePackage,
            context + " should mark the workspace as a performance package.");
    require(state.workspaceMode == drs::engine::WorkspaceMode::performanceOnly,
            context + " should keep the workspace in performance-only mode.");
    require(!state.authoringAvailable,
            context + " should suppress authoring controls.");
}

fs::path buildSemanticPackageV2Fixture(const fs::path& scratchDirectory)
{
    fs::remove_all(scratchDirectory);
    fs::create_directories(scratchDirectory);
    const auto packagePath = scratchDirectory / "semantic-route-v2.drpkg";
    auto packagePlan = package_support::buildPackagePlan(
        scratchDirectory / "runtime", packagePath);
    const auto v2Plan = drs::engine::buildPerformancePackageV2StreamingExportPlan(
        packagePlan.manifest,
        packagePlan.compiledRuntime,
        packagePath.generic_string(),
        packagePlan.additionalPayloads);
    require(v2Plan.built,
            "The semantic package-v2 host fixture plan must build.");
    const auto written = drs::engine::writePackageV2Streaming(v2Plan.plan);
    require(written.written && written.verified && written.atomicallyPublished,
            "The semantic package-v2 host fixture must export and verify.");
    return packagePath;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const auto generatedFixtureRoot = fs::temp_directory_path()
            / "drs-semantic-package-v2-host-validation";
        const auto selectedPackage = argc >= 2
            ? std::filesystem::absolute(std::filesystem::path(argv[1]))
            : buildSemanticPackageV2Fixture(generatedFixtureRoot);
        const auto largeV2Qualification = argc >= 2;
        require(std::filesystem::exists(selectedPackage),
                "The selected performance package fixture must exist for host validation.");

        juce::ScopedJuceInitialiser_GUI gui;

        const auto packageFile = juce::File(juce::String::fromUTF8(selectedPackage.generic_string().c_str()));

        auto standalone = std::make_unique<drs::standalone::MainComponent>(false);
        std::chrono::microseconds standalonePreparationElapsed {};
        std::chrono::microseconds standaloneActivationElapsed {};
        const auto standaloneLoad = loadThroughBackgroundPreparation(
            standalone->getProcessor(), packageFile,
            standalonePreparationElapsed, standaloneActivationElapsed);
        const auto standaloneOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            standalonePreparationElapsed + standaloneActivationElapsed);
        require(standaloneLoad.loaded,
                "Standalone host validation should load the checked-in package. state="
                    + standaloneLoad.state + " issues=" + summarizeIssues(standaloneLoad.issues));
        require(standaloneLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Standalone host validation should not publish a package failure category for the valid fixture.");
        requirePerformanceOnlyWorkspace(standalone->getProcessor().getWorkspaceDocumentState(),
                                        "Standalone host validation");
        standalone->getProcessor().prepareToPlay(44100.0, 512);
        standalone->getProcessor().serviceMessageThreadWork();

        const auto standalonePlaybackStarted = Clock::now();
        const auto standaloneMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            standalone->getProcessor(), 69, 0.8f,
            largeV2Qualification ? 375 : 8, largeV2Qualification);
        const auto standalonePlaybackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - standalonePlaybackStarted);
        standalone->getProcessor().serviceMessageThreadWork();
        require(standaloneMagnitude > 0.0001f,
                "Standalone host validation should produce audible output from the checked-in package.");
        const auto standaloneDiagnostics = standalone->getEngineFacade().getDiagnosticsSnapshot();
        const auto standaloneRealtime = standalone->getProcessor().getRealtimeSafetySnapshot();
        require(standaloneDiagnostics.available,
                "Standalone host validation should publish diagnostics after opening the checked-in package.");
        require(standaloneRealtime.getAudioThreadViolationCount() == 0,
                "Standalone package playback reported a realtime-thread violation.");
        require(!largeV2Qualification
                    || (standaloneDiagnostics.cacheMissCount > 0
                        && standaloneDiagnostics.backgroundReadCount > 0),
                "Standalone large-package playback did not service pages beyond the resident head.");
        require(standaloneOpenElapsed <= std::chrono::milliseconds(3000),
                "Standalone package open exceeded the reviewed host-validation budget.");
        require(standalonePlaybackElapsed <= (largeV2Qualification
                    ? std::chrono::milliseconds(6000) : std::chrono::milliseconds(1000)),
                "Standalone initial package playback exceeded the reviewed host-validation budget.");

        auto processor = std::make_unique<drs::plugin::Processor>();
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());
        require(editor != nullptr, "Plugin editor should construct for package host validation.");

        std::chrono::microseconds pluginPreparationElapsed {};
        std::chrono::microseconds pluginActivationElapsed {};
        const auto pluginLoad = loadThroughBackgroundPreparation(
            *processor, packageFile, pluginPreparationElapsed, pluginActivationElapsed);
        const auto pluginOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            pluginPreparationElapsed + pluginActivationElapsed);
        require(pluginLoad.loaded,
                "Plugin host validation should load the checked-in package. state="
                    + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));
        require(pluginLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Plugin host validation should not publish a package failure category for the valid fixture.");
        requirePerformanceOnlyWorkspace(processor->getWorkspaceDocumentState(),
                                        "Plugin host validation");
        processor->prepareToPlay(44100.0, 512);
        processor->serviceMessageThreadWork();

        const auto pluginPlaybackStarted = Clock::now();
        const auto pluginMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            *processor, 69, 0.8f,
            largeV2Qualification ? 375 : 8, largeV2Qualification);
        const auto pluginPlaybackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - pluginPlaybackStarted);
        processor->serviceMessageThreadWork();
        require(pluginMagnitude > 0.0001f,
                "Plugin host validation should produce audible output from the checked-in package.");
        const auto pluginDiagnostics = processor->getEngineFacade().getDiagnosticsSnapshot();
        const auto pluginRealtime = processor->getRealtimeSafetySnapshot();
        require(pluginDiagnostics.available,
                "Plugin host validation should publish diagnostics after opening the checked-in package.");
        require(pluginRealtime.getAudioThreadViolationCount() == 0,
                "Plugin package playback reported a realtime-thread violation.");
        require(!largeV2Qualification
                    || (pluginDiagnostics.cacheMissCount > 0
                        && pluginDiagnostics.backgroundReadCount > 0),
                "Plugin large-package playback did not service pages beyond the resident head.");
        require(pluginOpenElapsed <= std::chrono::milliseconds(3000),
                "Plugin package open exceeded the reviewed host-validation budget.");
        require(pluginPlaybackElapsed <= (largeV2Qualification
                    ? std::chrono::milliseconds(6000) : std::chrono::milliseconds(1000)),
                "Plugin initial package playback exceeded the reviewed host-validation budget.");
        require(!largeV2Qualification
                    || (standaloneActivationElapsed <= std::chrono::milliseconds(16)
                        && pluginActivationElapsed <= std::chrono::milliseconds(16)),
                "Prepared package activation exceeded the 16 ms message-thread budget: standalone="
                    + std::to_string(standaloneActivationElapsed.count()) + " us, plugin="
                    + std::to_string(pluginActivationElapsed.count()) + " us.");

        for (int iteration = 0; iteration < 8; ++iteration)
        {
            std::unique_ptr<juce::AudioProcessorEditor> churnedEditor(processor->createEditor());
            require(churnedEditor != nullptr,
                    "Plugin editor churn failed to construct an editor.");
        }

        juce::MemoryBlock savedState;
        require(processor->waitForHostStatePublication(),
                "Package locator state did not reach background host-state publication.");
        processor->getStateInformation(savedState);
        const std::string savedStateText(
            static_cast<const char*>(savedState.getData()), savedState.getSize());
        require(savedStateText.find("performancePackageBinding") != std::string::npos
                    && savedStateText.find(selectedPackage.filename().generic_string())
                        != std::string::npos,
                "Package host state did not persist its explicit package locator binding.");

        const auto restoreStarted = Clock::now();
        auto restoredProcessor = std::make_unique<drs::plugin::Processor>();
        restoredProcessor->prepareToPlay(44100.0, 512);
        const auto setStateStarted = Clock::now();
        restoredProcessor->setStateInformation(savedState.getData(),
                                               static_cast<int>(savedState.getSize()));
        const auto setStateElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - setStateStarted);
        const auto restoreDeadline = Clock::now() + std::chrono::seconds(15);
        while (Clock::now() < restoreDeadline
               && restoredProcessor->getWorkspaceDocumentState().kind
                   != drs::engine::WorkspaceDocumentKind::performancePackage)
        {
            restoredProcessor->serviceMessageThreadWork();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        restoredProcessor->serviceMessageThreadWork();
        const auto restoreElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - restoreStarted);
        require(setStateElapsed <= std::chrono::milliseconds(16),
                "setStateInformation blocked while restoring a package locator.");
        require(restoredProcessor->getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::performancePackage
                    && restoredProcessor->getWorkspaceDocumentState().playable,
                "Editor-closed host recall did not restore the package workspace to playable.");
        require(renderQueuedPerformanceSurfaceMagnitude(
                    *restoredProcessor, 69, 0.8f, largeV2Qualification ? 64 : 8,
                    largeV2Qualification) > 0.0001f,
                "Editor-closed host recall did not restore audible package playback.");
        std::unique_ptr<juce::AudioProcessorEditor> restoredEditor(
            restoredProcessor->createEditor());
        require(restoredEditor != nullptr,
                "An editor could not open after editor-closed package recall.");

        std::cout << "Performance package host validation passed:"
                  << " standaloneOpenMs=" << standaloneOpenElapsed.count()
                  << " standalonePrepareUs=" << standalonePreparationElapsed.count()
                  << " standaloneActivateUs=" << standaloneActivationElapsed.count()
                  << " standaloneEngineActivateUs="
                  << standaloneLoad.timings.engineSessionActivationMicros
                  << " standaloneWorkspaceUs="
                  << standaloneLoad.timings.workspaceTransitionMicros
                  << " standalonePlaybackMs=" << standalonePlaybackElapsed.count()
                  << " standaloneHeadBytesRead=" << standaloneDiagnostics.headBytesRead
                  << " standaloneCacheMissCount=" << standaloneDiagnostics.cacheMissCount
                  << " standaloneBackgroundReadCount=" << standaloneDiagnostics.backgroundReadCount
                  << " pluginOpenMs=" << pluginOpenElapsed.count()
                  << " pluginPrepareUs=" << pluginPreparationElapsed.count()
                  << " pluginActivateUs=" << pluginActivationElapsed.count()
                  << " pluginEngineActivateUs="
                  << pluginLoad.timings.engineSessionActivationMicros
                  << " pluginWorkspaceUs="
                  << pluginLoad.timings.workspaceTransitionMicros
                  << " pluginPlaybackMs=" << pluginPlaybackElapsed.count()
                  << " pluginHeadBytesRead=" << pluginDiagnostics.headBytesRead
                  << " pluginCacheMissCount=" << pluginDiagnostics.cacheMissCount
                  << " pluginBackgroundReadCount=" << pluginDiagnostics.backgroundReadCount
                  << " restoreMs=" << restoreElapsed.count()
                  << " setStateUs=" << setStateElapsed.count()
                  << " standaloneRtViolations="
                  << standaloneRealtime.getAudioThreadViolationCount()
                  << " pluginRtViolations=" << pluginRealtime.getAudioThreadViolationCount()
                  << std::endl;
        restoredEditor.reset();
        restoredProcessor.reset();
        editor.reset();
        processor.reset();
        standalone.reset();
        if (argc < 2)
            fs::remove_all(generatedFixtureRoot);
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package host validation tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
