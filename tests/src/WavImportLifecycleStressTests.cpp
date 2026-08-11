#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "../support/WavImportTestSupport.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void pumpMessages(const int milliseconds = 20)
{
#if JUCE_MODAL_LOOPS_PERMITTED
    if (auto* messageManager = juce::MessageManager::getInstanceWithoutCreating())
        messageManager->runDispatchLoopUntil(milliseconds);
    else
        juce::Thread::sleep(milliseconds);
#else
    juce::Thread::sleep(milliseconds);
#endif
}

void processBlock(drs::plugin::Processor& processor, const int blockSize = 64)
{
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
}

std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> waitForRestore(
    drs::plugin::Processor& processor,
    const std::string& context,
    const std::chrono::milliseconds timeout = 10s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor);
        processor.serviceMessageThreadWork();
        const auto snapshot = processor.getProjectRestoreSnapshot();
        if (snapshot != nullptr
            && (snapshot->state == drs::engine::ProjectRestoreState::active
                || snapshot->state == drs::engine::ProjectRestoreState::ready
                || snapshot->state == drs::engine::ProjectRestoreState::failed
                || snapshot->state == drs::engine::ProjectRestoreState::needsLocation))
        {
            return snapshot;
        }
        std::this_thread::sleep_for(2ms);
    }

    throw std::runtime_error(context + " timed out while waiting for restore.");
}

bool waitForWaveformPreviewReady(drs::plugin::Processor& processor,
                                 const std::chrono::milliseconds timeout = 10s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        auto preview = processor.getAuthoringWaveformPreview();
        if (preview.available && !preview.points.empty() && preview.state == "Ready")
            return true;

        processor.serviceMessageThreadWork();
        std::this_thread::sleep_for(2ms);
    }

    return false;
}

std::shared_ptr<const drs::app::WavImportBatchSnapshot> waitForImportActive(
    drs::app::WavImportService::Client& client,
    const std::chrono::milliseconds timeout = 5s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto snapshot = client.getSnapshot();
        if (snapshot != nullptr
            && (snapshot->stage == drs::app::WavImportBatchStage::staging
                || snapshot->stage == drs::app::WavImportBatchStage::inspecting))
        {
            return snapshot;
        }
        if (snapshot != nullptr
            && (snapshot->stage == drs::app::WavImportBatchStage::completed
                || snapshot->stage == drs::app::WavImportBatchStage::failed
                || snapshot->stage == drs::app::WavImportBatchStage::canceled
                || snapshot->stage == drs::app::WavImportBatchStage::superseded))
        {
            return snapshot;
        }

        std::this_thread::sleep_for(1ms);
    }

    return nullptr;
}

std::uint64_t regularFileCount(const fs::path& root)
{
    std::uint64_t count = 0;
    if (!fs::exists(root))
        return 0;

    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file())
            ++count;
    }

    return count;
}

int resolveCycleCount()
{
    if (const auto* value = std::getenv("DRS_WAV_STRESS_CYCLE_COUNT");
        value != nullptr && *value != '\0')
    {
        const auto parsed = std::atoi(value);
        if (parsed > 0)
            return parsed;
    }

    return 100;
}

bool verboseStressLoggingEnabled()
{
    const auto* value = std::getenv("DRS_WAV_STRESS_VERBOSE");
    return value != nullptr && *value != '\0' && std::string(value) != "0";
}

void logStressPhase(const bool verbose,
                    const int cycle,
                    const std::string& phase)
{
    if (!verbose)
        return;

    std::cerr << "[wav-stress cycle " << cycle << "] " << phase << std::endl;
}

fs::path makeStressRoot()
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto root = fs::temp_directory_path() / ("drs-wav-import-lifecycle-stress-" + unique);
    fs::create_directories(root);
    return root;
}

drs::engine::RuntimeProjectModel buildUnloadedProjectState()
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 5;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 4;
    project.authoring.notes = { "WAV lifecycle stress unloaded state." };
    project.notes = { "WAV lifecycle stress unloaded state." };
    return project;
}

drs::engine::RuntimeProjectModel makeStressProject(const drs::engine::RuntimeProjectModel& baseProject,
                                                   const fs::path& projectRoot,
                                                   const int cycle)
{
    auto project = baseProject;
    project.projectId = "wav-lifecycle-stress-" + std::to_string(cycle);
    project.displayName = "WAV Lifecycle Stress " + std::to_string(cycle);
    project.contentRootPath = projectRoot.generic_string();
    project.defaultInstrumentManifestPath
        = (projectRoot / ("wav-lifecycle-stress-" + std::to_string(cycle) + ".drinst")).generic_string();
    return project;
}

drs::engine::RuntimeProjectModel makeHookedStressProject(
    const drs::engine::RuntimeProjectModel& baseProject,
    const fs::path& projectRoot,
    const int cycle,
    drs::tests::DeterministicSampleImportHooks& hooks)
{
    auto project = makeStressProject(baseProject, projectRoot, cycle);
    auto sampleIndex = 0u;
    for (auto& sampleSource : project.sampleSources)
        sampleSource.path = "wav-lifecycle-stress-" + std::to_string(cycle)
            + "-" + sampleSource.id + ".wav";

    for (const auto& sampleSource : project.sampleSources)
    {
        hooks.addReaderFixture({
            sampleSource.path,
            "WAV file",
            48000.0,
            8192 + (sampleIndex * 1024),
            sampleIndex == 0 ? 2u : 1u,
            32,
            true,
            {},
            false
        });
        ++sampleIndex;
    }
    return project;
}

drs::engine::RuntimeProjectModel makeSyntheticPreviewProject(
    const drs::engine::RuntimeProjectModel& baseProject,
    const fs::path& projectRoot,
    const std::string& syntheticPath,
    const int cycle)
{
    auto project = makeStressProject(baseProject, projectRoot, cycle);
    const auto selectedZone = std::find_if(project.authoring.zones.begin(),
                                           project.authoring.zones.end(),
                                           [&](const auto& zone)
                                           {
                                               return zone.id == project.authoring.selectedZoneId;
                                           });
    require(selectedZone != project.authoring.zones.end(),
            "Synthetic preview project requires a selected reference zone.");

    auto sampleSource = std::find_if(project.sampleSources.begin(),
                                     project.sampleSources.end(),
                                     [&](const auto& item)
                                     {
                                         return item.id == selectedZone->sampleSourceId;
                                     });
    require(sampleSource != project.sampleSources.end(),
            "Synthetic preview project requires the selected reference sample source.");
    sampleSource->path = syntheticPath;
    return project;
}

void exerciseRapidWaveformSelection(drs::plugin::Processor& processor,
                                    const std::string& context,
                                    const bool requireReadyAtEnd = true)
{
    static const std::array<const char*, 6> zoneIds {
        "lead-a4-sustain",
        "pad-a3-low",
        "pad-a3-high",
        "lead-a4-sustain",
        "pad-a3-high",
        "pad-a3-low"
    };

    for (const auto* zoneId : zoneIds)
    {
        require(processor.getAuthoringSession().selectZone(zoneId).applied,
                context + " could not select zone '" + zoneId + "'.");
        processor.authorizeAuthoringWaveformPreviewLoad();
        static_cast<void>(processor.getAuthoringWaveformPreview());
        processor.serviceMessageThreadWork();
    }

    require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
            context + " could not restore the final waveform selection.");
    processor.authorizeAuthoringWaveformPreviewLoad();
    if (requireReadyAtEnd)
    {
        require(waitForWaveformPreviewReady(processor),
                context + " did not publish a ready waveform preview after rapid selection.");
    }
}

void runCanceledImportCycle(drs::plugin::Processor& processor,
                            const fs::path& projectRoot,
                            const drs::tests::GeneratedWavImportBatchCorpus& corpus,
                            const std::string& projectId,
                            const std::string& selectedGroupId,
                            const std::string& context)
{
    {
        auto client = processor.getWavImportService().openClient();
        drs::app::WavImportRequest request;
        request.projectId = projectId;
        request.contentRootPath = projectRoot.generic_string();
        request.selectedGroupId = selectedGroupId;
        request.sourcePaths.assign(256, corpus.cleanPath.generic_string());

        const auto accepted = client.submit(request);
        require(accepted.wasAccepted(), context + " did not accept the stress import request.");
        const auto active = waitForImportActive(client);
        require(active != nullptr, context + " never published an import snapshot.");
        require(active->stage == drs::app::WavImportBatchStage::staging
                    || active->stage == drs::app::WavImportBatchStage::inspecting,
                context + " completed before the stress harness could cancel it.");
        require(client.cancel("Stress import canceled"),
                context + " did not accept the cancellation request.");
        require(client.waitForTerminal(10s),
                context + " did not reach a terminal import state after cancellation.");
        const auto canceled = client.getSnapshot();
        require(canceled != nullptr
                    && canceled->stage == drs::app::WavImportBatchStage::canceled
                    && canceled->terminalDisposition == drs::app::WavImportTerminalDisposition::canceled,
                context + " did not publish a canceled terminal import snapshot.");
    }

    std::this_thread::sleep_for(5ms);
    require(regularFileCount(projectRoot / "Samples") == 0,
            context + " leaked staged or final files after import cancellation.");
}

struct PreviewShutdownProbe
{
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<std::uint64_t> callbackCount { 0 };
    bool buildReached = false;
    bool releaseBuild = false;
};

void runUnloadPreviewProbe(const drs::engine::RuntimeProjectModel& baseProject,
                           const fs::path& root,
                           const int cycle,
                           std::uint64_t& observedCallbacks)
{
    const auto syntheticPath = "wav-lifecycle-stress-preview-" + std::to_string(cycle) + ".wav";
    drs::tests::DeterministicSampleImportHooks hooks;
    hooks.addReaderFixture({ syntheticPath, "WAV file", 48000.0, 32768, 2, 32, true, {}, false });

    auto probe = std::make_shared<PreviewShutdownProbe>();
    drs::app::WaveformPreviewServiceOptions options;
    options.sampleImportHooks = &hooks;
    options.stageObserver = [probe](const drs::app::WaveformPreviewServiceStage stage)
    {
        probe->callbackCount.fetch_add(1, std::memory_order_relaxed);
        if (stage != drs::app::WaveformPreviewServiceStage::building)
            return;

        std::unique_lock<std::mutex> lock(probe->mutex);
        probe->buildReached = true;
        probe->condition.notify_all();
        probe->condition.wait(lock, [&] { return probe->releaseBuild; });
    };

    auto processor = std::make_unique<drs::plugin::Processor>(options);
    processor->prepareToPlay(44100.0, 64);
    require(processor->replaceAuthoringProject(
                makeSyntheticPreviewProject(baseProject, root / "project", syntheticPath, cycle)),
            "Preview-unload probe could not load the synthetic project.");
    processor->authorizeAuthoringWaveformPreviewLoad();

    {
        std::unique_lock<std::mutex> lock(probe->mutex);
        require(probe->condition.wait_for(lock, 5s, [&] { return probe->buildReached; }),
                "Preview-unload probe never reached the building checkpoint.");
    }

    std::atomic<bool> destroyCompleted { false };
    std::thread destroyThread([&]
    {
        processor.reset();
        destroyCompleted.store(true, std::memory_order_release);
    });

    std::this_thread::sleep_for(10ms);
    require(!destroyCompleted.load(std::memory_order_acquire),
            "Processor unload should wait for the active preview worker to unwind.");

    {
        std::lock_guard<std::mutex> lock(probe->mutex);
        probe->releaseBuild = true;
    }
    probe->condition.notify_all();
    destroyThread.join();

    const auto callbacksAtDestroy = probe->callbackCount.load(std::memory_order_acquire);
    std::this_thread::sleep_for(30ms);
    require(probe->callbackCount.load(std::memory_order_acquire) == callbacksAtDestroy,
            "Waveform preview published callbacks after processor unload.");
    observedCallbacks += callbacksAtDestroy;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "WAV lifecycle stress requires the Phase 2 reference project.");

        const auto stressRoot = makeStressRoot();
        const auto cycleCount = resolveCycleCount();
        const auto verbose = verboseStressLoggingEnabled();
        std::uint64_t totalUnloadPreviewCallbacks = 0;
        const auto sharedRoot = stressRoot / "shared";
        const auto sharedSourceRoot = sharedRoot / "source";
        const auto sharedProjectRoot = sharedRoot / "project";
        fs::create_directories(sharedProjectRoot / "Samples");
        const auto corpus = drs::tests::createGeneratedWavImportBatchCorpus(sharedSourceRoot);

        drs::tests::DeterministicSampleImportHooks previewHooks;
        const auto hookedProject = makeHookedStressProject(projectLoad.project,
                                                           sharedProjectRoot,
                                                           0,
                                                           previewHooks);
        drs::app::WaveformPreviewServiceOptions previewOptions;
        previewOptions.sampleImportHooks = &previewHooks;

        juce::MemoryBlock stateBlock;
        {
            drs::plugin::Processor processor(previewOptions);
            processor.prepareToPlay(44100.0, 64);
            require(processor.replaceAuthoringProject(hookedProject),
                    "Shared WAV lifecycle stress processor could not replace the stress project.");

            for (int cycle = 0; cycle < cycleCount; ++cycle)
            {
                logStressPhase(verbose, cycle, "editor-open-close");
                auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
                require(editor != nullptr, "Editor stress cycle could not open the plugin editor.");
                editor->addToDesktop(0);
                editor->setVisible(true);
                pumpMessages(1);
            }

            for (int cycle = 0; cycle < cycleCount; ++cycle)
            {
                logStressPhase(verbose, cycle, "rapid-waveform-selection");
                exerciseRapidWaveformSelection(processor,
                                              "Waveform stress cycle " + std::to_string(cycle),
                                              ((cycle + 1) % 10) == 0);
            }

            for (int cycle = 0; cycle < cycleCount; ++cycle)
            {
                const auto selectedGroup = processor.getAuthoringSession().getSelectedGroup();
                logStressPhase(verbose, cycle, "import-cancel");
                runCanceledImportCycle(processor,
                                       sharedProjectRoot,
                                       corpus,
                                       hookedProject.projectId,
                                       selectedGroup.has_value() ? selectedGroup->id : std::string {},
                                       "Import stress cycle " + std::to_string(cycle));
            }

            for (int cycle = 0; cycle < cycleCount; ++cycle)
            {
                logStressPhase(verbose, cycle, "close-reopen");
                processor.closeAuthoringProject(buildUnloadedProjectState());
                pumpMessages(1);
                require(processor.replaceAuthoringProject(hookedProject),
                        "Replace stress cycle " + std::to_string(cycle) + " could not reopen the stress project.");
                processor.serviceMessageThreadWork();
            }

            auto editedZone = processor.getAuthoringSession().getSelectedZone();
            require(editedZone.has_value(), "Restore stress source lost the selected zone before state capture.");
            editedZone->gainDb += 0.25;
            require(processor.getAuthoringSession().updateSelectedZone(*editedZone,
                                                                      "WAV lifecycle stress restore source edit").applied,
                    "Restore stress source could not dirty the project before host-state capture.");
            require(processor.getAuthoringSession().getDocumentState().dirty,
                    "Restore stress source did not produce a dirty project before host-state capture.");
            processor.serviceMessageThreadWork();
            require(processor.waitForHostStatePublication(),
                    "Restore stress source checkpoint did not reach background host-state publication.");
            processor.getStateInformation(stateBlock);
            require(stateBlock.getSize() > 0,
                    "Restore stress source produced an empty host-state chunk.");
        }

        for (int cycle = 0; cycle < cycleCount; ++cycle)
        {
            drs::tests::DeterministicSampleImportHooks restoredPreviewHooks;
            auto restoredProject = hookedProject;
            for (const auto& sampleSource : restoredProject.sampleSources)
            {
                restoredPreviewHooks.addReaderFixture({
                    sampleSource.path,
                    "WAV file",
                    48000.0,
                    8192,
                    2,
                    32,
                    true,
                    {},
                    false
                });
            }
            drs::app::WaveformPreviewServiceOptions restoredPreviewOptions;
            restoredPreviewOptions.sampleImportHooks = &restoredPreviewHooks;
            drs::plugin::Processor restored(restoredPreviewOptions);
            restored.prepareToPlay(44100.0, 64);
            logStressPhase(verbose, cycle, "restore");
            restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
            const auto restore = waitForRestore(restored, "Restore stress cycle " + std::to_string(cycle));
            restored.serviceMessageThreadWork();
            processBlock(restored);
            restored.serviceMessageThreadWork();
            require(restore->state == drs::engine::ProjectRestoreState::ready
                        || restore->state == drs::engine::ProjectRestoreState::active,
                    "Restore stress cycle " + std::to_string(cycle)
                        + " did not restore to a ready or active state.");
            require(restored.getAuthoringSession().getProject().projectId == restoredProject.projectId,
                    "Restore stress cycle " + std::to_string(cycle) + " restored the wrong project identity.");
        }

        require(regularFileCount(sharedProjectRoot / "Samples") == 0,
                "Shared WAV lifecycle stress left staged or committed sample files behind after teardown.");

        for (int cycle = 0; cycle < cycleCount; ++cycle)
        {
            logStressPhase(verbose, cycle, "unload-probe");
            runUnloadPreviewProbe(projectLoad.project,
                                  stressRoot / ("unload-preview-" + std::to_string(cycle)),
                                  cycle,
                                  totalUnloadPreviewCallbacks);
        }

        std::cout << "WAV lifecycle stress tests passed: cycles=" << cycleCount
                  << ", unloadPreviewCallbacks=" << totalUnloadPreviewCallbacks
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV lifecycle stress tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
