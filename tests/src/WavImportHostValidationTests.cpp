#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string describeIoCounters(const drs::engine::SampleImportIoCounters& counters)
{
    return "fingerprintOpenCount=" + std::to_string(counters.fingerprintOpenCount)
        + ", readerOpenCount=" + std::to_string(counters.readerOpenCount)
        + ", bytesReadCount=" + std::to_string(counters.bytesReadCount)
        + ", fullFrameReadCount=" + std::to_string(counters.fullFrameReadCount)
        + ", copyCount=" + std::to_string(counters.copyCount)
        + ", peakChunkReadCount=" + std::to_string(counters.peakChunkReadCount);
}

void requireNoImportIo(const std::string& context)
{
    const auto counters = drs::engine::getSampleImportIoCounters();
    require(counters.fingerprintOpenCount == 0
                && counters.readerOpenCount == 0
                && counters.bytesReadCount == 0
                && counters.fullFrameReadCount == 0
                && counters.copyCount == 0
                && counters.peakChunkReadCount == 0,
            context + " unexpectedly performed sample import IO: " + describeIoCounters(counters));
}

struct Scenario
{
    std::string name;
    std::vector<std::string> paths;
};

drs::engine::RuntimeProjectModel rewriteSampleLocations(drs::engine::RuntimeProjectModel project,
                                                        const Scenario& scenario)
{
    require(project.sampleSources.size() == scenario.paths.size(),
            "Scenario '" + scenario.name + "' path count must match the reference sample source count.");
    project.displayName += " (" + scenario.name + ")";
    for (std::size_t index = 0; index < scenario.paths.size(); ++index)
        project.sampleSources[index].path = scenario.paths[index];
    return project;
}

void runScenario(const Scenario& scenario)
{
    const auto manifestPath = drs::engine::getPhase2ReferenceProjectManifestPath();
    const auto loaded = drs::engine::loadRuntimeProjectManifest(manifestPath);
    require(loaded.loaded,
            "The Phase 2 reference project fixture must load for WAV host validation.");
    const auto project = rewriteSampleLocations(loaded.project, scenario);

    drs::engine::resetSampleImportIoCounters();
    const auto constructionStarted = Clock::now();
    drs::standalone::MainComponent standalone(false);
    const auto constructionElapsed
        = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - constructionStarted);
    require(!standalone.isAudioOutputEnabled(),
            "Host validation must keep standalone audio output disabled.");
    requireNoImportIo("Standalone construction for scenario '" + scenario.name + "'");

    drs::engine::resetSampleImportIoCounters();
    const auto replaceStarted = Clock::now();
    require(standalone.getProcessor().replaceAuthoringProject(project),
            "Standalone host validation must accept scenario '" + scenario.name + "'.");
    standalone.getProcessor().serviceMessageThreadWork();
    const auto replaceElapsed
        = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - replaceStarted);
    requireNoImportIo("Standalone project replace for scenario '" + scenario.name + "'");

    const auto snapshot = standalone.getProcessor().getAuthoringImportResponsivenessSnapshot();
    require(snapshot.available,
            "Scenario '" + scenario.name + "' must publish an authoring import snapshot.");
    require(snapshot.state == "not-run",
            "Scenario '" + scenario.name + "' should remain not-run until explicit import work is requested.");
    require(snapshot.totalItemCount == project.sampleSources.size(),
            "Scenario '" + scenario.name + "' should report the project sample-source count.");
    require(snapshot.processedCount == 0 && snapshot.pendingCount == 0,
            "Scenario '" + scenario.name + "' must not enqueue startup work.");

    // Standalone shell construction includes fixed JUCE/UI bootstrapping overhead in Debug.
    constexpr auto maximumConstructionMs = std::chrono::milliseconds(1500);
    constexpr auto maximumReplaceMs = std::chrono::milliseconds(750);
    require(constructionElapsed <= maximumConstructionMs,
            "Standalone construction exceeded the WAV host-validation budget for scenario '" + scenario.name
                + "': " + std::to_string(constructionElapsed.count()) + " ms.");
    require(replaceElapsed <= maximumReplaceMs,
            "Standalone project replace exceeded the WAV host-validation budget for scenario '"
                + scenario.name + "': " + std::to_string(replaceElapsed.count()) + " ms.");

    std::cout << "WAV host validation scenario '" << scenario.name << "' passed: construction="
              << constructionElapsed.count() << "ms, replace=" << replaceElapsed.count() << "ms\n";
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const std::vector<Scenario> scenarios = {
            { "missing-local",
              {
                  "C:/PracticalSamplerValidationMissing/Samples/missing-local-a3.wav",
                  "C:/PracticalSamplerValidationMissing/Samples/missing-local-a4.wav",
              } },
            { "removable-drive",
              {
                  "R:/RemovedMedia/DRS_Sine_A3.wav",
                  "R:/RemovedMedia/DRS_TriangleLead_A4.wav",
              } },
            { "network-unc",
              {
                  "//offline-host/drs/DRS_Sine_A3.wav",
                  "//offline-host/drs/DRS_TriangleLead_A4.wav",
              } },
        };

        for (const auto& scenario : scenarios)
            runScenario(scenario);

        std::cout << "WAV import host validation tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import host validation tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
