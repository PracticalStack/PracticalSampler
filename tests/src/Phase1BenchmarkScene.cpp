#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeStreamingService.h"
#include "drs/engine/RuntimeVoice.h"
#include "standalone/MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <json/json.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

void serviceRestore(drs::plugin::Processor& processor, const std::string& context)
{
    const auto deadline = Clock::now() + std::chrono::seconds(5);
    while (Clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto restore = processor.getProjectRestoreSnapshot();
        if (restore != nullptr
            && (restore->state == drs::engine::ProjectRestoreState::active
                || restore->state == drs::engine::ProjectRestoreState::ready
                || restore->state == drs::engine::ProjectRestoreState::needsLocation
                || restore->state == drs::engine::ProjectRestoreState::failed))
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    throw std::runtime_error(context + " timed out.");
}

struct SceneMacroExpectation
{
    std::string id;
    double value = 0.0;
};

struct SceneVoice
{
    std::uint64_t voiceId = 0;
    int midiNote = 60;
    int velocity = 100;
    std::string articulationId;
};

struct ScenePage
{
    std::string sampleId;
    std::uint32_t pageIndex = 0;
};

struct BenchmarkScene
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string sceneId;
    std::string displayName;
    fs::path scenePath;
    fs::path referenceInstrumentManifestPath;
    fs::path referencePresetStatePath;

    struct
    {
        std::string articulationId;
        int midiNote = 60;
        int velocity = 100;
        std::string expectedZoneId;
    } ordinaryPlayback;

    struct
    {
        std::string loadProfileId;
        int warmupFrames = 0;
        int boundaryFrames = 0;
        int resumeFrames = 0;
        int releaseFrames = 0;
        std::size_t expectedPeakActiveVoiceCount = 0;
        std::vector<ScenePage> expectedReadyPages;
        std::vector<SceneVoice> voices;
    } moderatePolyphony;

    struct
    {
        std::string expectedLoadProfileId;
        std::string expectedArticulationId;
        std::vector<SceneMacroExpectation> expectedMacros;
    } presetReload;

    struct
    {
        std::string initialProfileId;
        std::string downgradedProfileId;
        int warmupFrames = 0;
        int boundaryFrames = 0;
        int resumeFrames = 0;
        int releaseFrames = 0;
        std::size_t expectedActiveLeaseCountBeforeSwitch = 0;
        SceneVoice voice;
        std::vector<ScenePage> additionalPageReads;
    } loadProfileSwitch;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "Could not open file: " + path.generic_string());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Could not open benchmark report artifact for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing benchmark report artifact: " + path.generic_string());
}

template <typename TPredicate>
bool waitUntil(TPredicate predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = Clock::now() + timeout;

    while (Clock::now() < deadline)
    {
        if (predicate())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return predicate();
}

bool nearlyEqual(double actual, double expected, double tolerance = 0.0001)
{
    return std::abs(actual - expected) <= tolerance;
}

fs::path resolveRelativePath(const fs::path& manifestPath, const std::string& rawPath)
{
    const fs::path candidate(rawPath);

    if (candidate.is_absolute())
        return candidate.lexically_normal();

    return (manifestPath.parent_path() / candidate).lexically_normal();
}

std::optional<double> findMacroValue(const drs::engine::RuntimeSessionStateSnapshot& sessionState, const std::string& macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const auto& macroValue)
                                       {
                                           return macroValue.id == macroId;
                                       });

    if (iterator == sessionState.macroValues.end())
        return std::nullopt;

    return iterator->value;
}

ScenePage parseScenePage(const json& root)
{
    ScenePage page;
    page.sampleId = root.at("sampleId").get<std::string>();
    page.pageIndex = root.at("pageIndex").get<std::uint32_t>();
    return page;
}

SceneVoice parseSceneVoice(const json& root)
{
    SceneVoice voice;
    voice.voiceId = root.at("voiceId").get<std::uint64_t>();
    voice.midiNote = root.at("midiNote").get<int>();
    voice.velocity = root.at("velocity").get<int>();
    voice.articulationId = root.at("articulationId").get<std::string>();
    return voice;
}

BenchmarkScene loadBenchmarkScene(const fs::path& scenePath)
{
    BenchmarkScene scene;
    scene.scenePath = scenePath.lexically_normal();

    const auto root = json::parse(readTextFile(scene.scenePath));
    scene.schemaName = root.at("schemaName").get<std::string>();
    scene.schemaVersion = root.at("schemaVersion").get<int>();
    scene.sceneId = root.at("sceneId").get<std::string>();
    scene.displayName = root.at("displayName").get<std::string>();
    scene.referenceInstrumentManifestPath = resolveRelativePath(scene.scenePath,
                                                                root.at("referenceInstrumentManifestPath").get<std::string>());
    scene.referencePresetStatePath = resolveRelativePath(scene.scenePath,
                                                         root.at("referencePresetStatePath").get<std::string>());

    const auto& ordinaryPlayback = root.at("ordinaryPlayback");
    scene.ordinaryPlayback.articulationId = ordinaryPlayback.at("articulationId").get<std::string>();
    scene.ordinaryPlayback.midiNote = ordinaryPlayback.at("midiNote").get<int>();
    scene.ordinaryPlayback.velocity = ordinaryPlayback.at("velocity").get<int>();
    scene.ordinaryPlayback.expectedZoneId = ordinaryPlayback.at("expectedZoneId").get<std::string>();

    const auto& moderatePolyphony = root.at("moderatePolyphony");
    scene.moderatePolyphony.loadProfileId = moderatePolyphony.at("loadProfileId").get<std::string>();
    scene.moderatePolyphony.warmupFrames = moderatePolyphony.at("warmupFrames").get<int>();
    scene.moderatePolyphony.boundaryFrames = moderatePolyphony.at("boundaryFrames").get<int>();
    scene.moderatePolyphony.resumeFrames = moderatePolyphony.at("resumeFrames").get<int>();
    scene.moderatePolyphony.releaseFrames = moderatePolyphony.at("releaseFrames").get<int>();
    scene.moderatePolyphony.expectedPeakActiveVoiceCount = moderatePolyphony.at("expectedPeakActiveVoiceCount").get<std::size_t>();
    for (const auto& page : moderatePolyphony.at("expectedReadyPages"))
        scene.moderatePolyphony.expectedReadyPages.push_back(parseScenePage(page));
    for (const auto& voice : moderatePolyphony.at("voices"))
        scene.moderatePolyphony.voices.push_back(parseSceneVoice(voice));

    const auto& presetReload = root.at("presetReload");
    scene.presetReload.expectedLoadProfileId = presetReload.at("expectedLoadProfileId").get<std::string>();
    scene.presetReload.expectedArticulationId = presetReload.at("expectedArticulationId").get<std::string>();
    for (const auto& macro : presetReload.at("expectedMacros"))
    {
        scene.presetReload.expectedMacros.push_back({
            macro.at("id").get<std::string>(),
            macro.at("value").get<double>()
        });
    }

    const auto& loadProfileSwitch = root.at("loadProfileSwitch");
    scene.loadProfileSwitch.initialProfileId = loadProfileSwitch.at("initialProfileId").get<std::string>();
    scene.loadProfileSwitch.downgradedProfileId = loadProfileSwitch.at("downgradedProfileId").get<std::string>();
    scene.loadProfileSwitch.warmupFrames = loadProfileSwitch.at("warmupFrames").get<int>();
    scene.loadProfileSwitch.boundaryFrames = loadProfileSwitch.at("boundaryFrames").get<int>();
    scene.loadProfileSwitch.resumeFrames = loadProfileSwitch.at("resumeFrames").get<int>();
    scene.loadProfileSwitch.releaseFrames = loadProfileSwitch.at("releaseFrames").get<int>();
    scene.loadProfileSwitch.expectedActiveLeaseCountBeforeSwitch = loadProfileSwitch.at("expectedActiveLeaseCountBeforeSwitch").get<std::size_t>();
    scene.loadProfileSwitch.voice = parseSceneVoice(loadProfileSwitch.at("voice"));
    for (const auto& page : loadProfileSwitch.at("additionalPageReads"))
        scene.loadProfileSwitch.additionalPageReads.push_back(parseScenePage(page));

    require(scene.schemaName == "drs.benchmarkScene", "Benchmark scene schemaName must be 'drs.benchmarkScene'.");
    require(scene.schemaVersion == 1, "Benchmark scene schemaVersion must be 1.");
    require(fs::exists(scene.referenceInstrumentManifestPath), "Benchmark scene instrument manifest path must exist.");
    require(fs::exists(scene.referencePresetStatePath), "Benchmark scene preset state path must exist.");
    return scene;
}

bool sessionMatchesExpected(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                           const BenchmarkScene& scene)
{
    if (sessionState.loadProfileId != scene.presetReload.expectedLoadProfileId
        || sessionState.selectedArticulationId != scene.presetReload.expectedArticulationId)
    {
        return false;
    }

    for (const auto& macro : scene.presetReload.expectedMacros)
    {
        const auto value = findMacroValue(sessionState, macro.id);
        if (!value.has_value() || !nearlyEqual(*value, macro.value))
            return false;
    }

    return true;
}
} // namespace

int main(int argc, char* argv[])
{
    const auto scenePath = argc >= 2
        ? fs::path(argv[1])
        : fs::path(drs::engine::getPhase1ReferenceBenchmarkScenePath());
    const auto outputPath = argc >= 3
        ? fs::path(argv[2])
        : fs::temp_directory_path() / "drs-phase1-benchmark-scene.json";

    ordered_json report;
    report["report"] = "drs.phase1.benchmarkScene";
    report["scenePath"] = scenePath.lexically_normal().generic_string();

    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto scene = loadBenchmarkScene(scenePath);
        report["sceneId"] = scene.sceneId;
        report["displayName"] = scene.displayName;

        const auto referenceInstrument = drs::engine::loadPhase1ReferenceInstrumentManifest();
        const auto referenceStream = drs::engine::loadPhase1ReferenceStreamContainer();
        require(referenceInstrument.loaded, "Reference instrument must load before benchmark scene runs.");
        require(referenceStream.loaded, "Reference stream must load before benchmark scene runs.");
        require(fs::equivalent(scene.referenceInstrumentManifestPath,
                               fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath())),
                "Benchmark scene currently supports the checked-in Phase 1 reference instrument manifest only.");

        ordered_json ordinaryPlaybackSection;
        {
            drs::engine::EngineFacade engineFacade;
            require(engineFacade.setSelectedArticulation(scene.ordinaryPlayback.articulationId),
                    "Benchmark scene ordinary playback articulation could not be selected.");

            const auto start = Clock::now();
            const auto preview = engineFacade.auditionPreviewNote(scene.ordinaryPlayback.midiNote,
                                                                  scene.ordinaryPlayback.velocity);
            const auto durationMicros = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();

            ordinaryPlaybackSection["articulationId"] = scene.ordinaryPlayback.articulationId;
            ordinaryPlaybackSection["midiNote"] = scene.ordinaryPlayback.midiNote;
            ordinaryPlaybackSection["velocity"] = scene.ordinaryPlayback.velocity;
            ordinaryPlaybackSection["effectiveMidiNote"] = preview.effectiveMidiNote;
            ordinaryPlaybackSection["effectiveVelocity"] = preview.effectiveVelocity;
            ordinaryPlaybackSection["zoneId"] = preview.zoneId;
            ordinaryPlaybackSection["waitedForPage"] = preview.waitedForPage;
            ordinaryPlaybackSection["durationMicros"] = durationMicros;
            const bool ordinaryPlaybackPassed = preview.succeeded
                && preview.zoneId == scene.ordinaryPlayback.expectedZoneId;
            ordinaryPlaybackSection["passed"] = ordinaryPlaybackPassed;
            report["ordinaryPlayback"] = std::move(ordinaryPlaybackSection);
            require(ordinaryPlaybackPassed, "Benchmark scene ordinary playback result changed unexpectedly.");
        }

        ordered_json moderatePolyphonySection;
        {
            const auto profile = drs::engine::findPhase1RuntimeLoadProfile(scene.moderatePolyphony.loadProfileId);
            require(profile.has_value(), "Benchmark scene moderate polyphony load profile was unavailable.");

            auto sceneInstrument = referenceInstrument.instrument;
            sceneInstrument.defaultLoadProfile = scene.moderatePolyphony.loadProfileId;

            drs::engine::RuntimeStreamingService service(
                referenceStream.container,
                drs::engine::buildRuntimeStreamingServiceOptions(*profile, 5000));
            std::vector<drs::engine::RuntimeVoice> voices(scene.moderatePolyphony.voices.size());
            std::string errorMessage;
            bool allAllocated = true;
            bool allWaited = true;
            bool allResumed = true;

            const auto start = Clock::now();

            for (std::size_t index = 0; index < scene.moderatePolyphony.voices.size(); ++index)
            {
                const auto& voiceDefinition = scene.moderatePolyphony.voices[index];
                const auto allocated = voices[index].allocate(sceneInstrument,
                                                              referenceStream.container,
                                                              {
                                                                  voiceDefinition.voiceId,
                                                                  "",
                                                                  voiceDefinition.midiNote,
                                                                  voiceDefinition.velocity,
                                                                  {},
                                                                  voiceDefinition.articulationId
                                                              },
                                                              errorMessage);
                allAllocated = allAllocated && allocated;
            }

            require(allAllocated, "Benchmark scene moderate polyphony voice allocation failed.");

            for (auto& voice : voices)
            {
                voice.advanceFrames(scene.moderatePolyphony.warmupFrames, service);
                const auto boundaryAdvance = voice.advanceFrames(scene.moderatePolyphony.boundaryFrames, service);
                allWaited = allWaited && boundaryAdvance.waitingForPage;
            }

            const auto pagesReady = waitUntil(
                [&]
                {
                    return std::all_of(scene.moderatePolyphony.expectedReadyPages.begin(),
                                       scene.moderatePolyphony.expectedReadyPages.end(),
                                       [&](const auto& page)
                                       {
                                           return service.isPageReady({ page.sampleId, page.pageIndex });
                                       });
                },
                std::chrono::milliseconds(800));

            for (auto& voice : voices)
            {
                const auto resumedAdvance = voice.advanceFrames(scene.moderatePolyphony.resumeFrames, service);
                allResumed = allResumed && resumedAdvance.acquiredPageLease;
            }

            for (auto& voice : voices)
                voice.beginRelease();

            const auto allFinished = waitUntil(
                [&]
                {
                    return std::all_of(voices.begin(),
                                       voices.end(),
                                       [&](auto& voice)
                                       {
                                           const auto advance = voice.advanceFrames(scene.moderatePolyphony.releaseFrames, service);
                                           return advance.voiceFinished
                                               || voice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished;
                                       });
                },
                std::chrono::milliseconds(1500));

            const auto metrics = service.getMetrics();
            const auto durationMicros = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
            moderatePolyphonySection["voiceCount"] = voices.size();
            moderatePolyphonySection["allocated"] = allAllocated;
            moderatePolyphonySection["waitedForPages"] = allWaited;
            moderatePolyphonySection["expectedPagesReady"] = pagesReady;
            moderatePolyphonySection["resumedAfterReads"] = allResumed;
            moderatePolyphonySection["allFinished"] = allFinished;
            moderatePolyphonySection["peakActiveVoiceCount"] = metrics.peakActiveVoiceCount;
            moderatePolyphonySection["pageMissCount"] = metrics.pageMissCount;
            moderatePolyphonySection["backgroundReadCount"] = metrics.backgroundReadCount;
            moderatePolyphonySection["averageReadLatencyMicros"] = metrics.averageReadLatencyMicros;
            moderatePolyphonySection["durationMicros"] = durationMicros;
            const bool moderatePolyphonyPassed = allAllocated
                && allWaited
                && pagesReady
                && allResumed
                && allFinished
                && metrics.peakActiveVoiceCount >= scene.moderatePolyphony.expectedPeakActiveVoiceCount;
            moderatePolyphonySection["passed"] = moderatePolyphonyPassed;
            report["moderatePolyphony"] = std::move(moderatePolyphonySection);
            require(moderatePolyphonyPassed, "Benchmark scene moderate polyphony result changed unexpectedly.");
        }

        ordered_json presetReloadSection;
        {
            drs::standalone::MainComponent sourceComponent;
            const auto presetJson = readTextFile(scene.referencePresetStatePath);
            const auto restored = sourceComponent.restoreStateJson(presetJson);
            serviceRestore(sourceComponent.getProcessor(), "Benchmark source preset restore");
            const auto exportedStateJson = sourceComponent.exportStateJson();

            drs::standalone::MainComponent reloadedComponent;
            const auto reloaded = reloadedComponent.restoreStateJson(exportedStateJson);
            serviceRestore(reloadedComponent.getProcessor(), "Benchmark reloaded preset restore");
            const auto& reloadedSession = reloadedComponent.getEngineFacade().getCurrentSessionState();
            const bool sessionMatches = reloaded.restored && sessionMatchesExpected(reloadedSession, scene);

            presetReloadSection["restored"] = restored.restored;
            presetReloadSection["reloaded"] = reloaded.restored;
            presetReloadSection["exportedStateBytes"] = exportedStateJson.size();
            presetReloadSection["loadProfileId"] = reloadedSession.loadProfileId;
            presetReloadSection["selectedArticulationId"] = reloadedSession.selectedArticulationId;

            ordered_json macroValues = ordered_json::array();
            for (const auto& expectation : scene.presetReload.expectedMacros)
            {
                ordered_json macroEntry;
                macroEntry["id"] = expectation.id;
                const auto value = findMacroValue(reloadedSession, expectation.id);
                macroEntry["value"] = value.has_value() ? *value : -1.0;
                macroEntry["expectedValue"] = expectation.value;
                macroEntry["matched"] = value.has_value() && nearlyEqual(*value, expectation.value);
                macroValues.push_back(std::move(macroEntry));
            }

            presetReloadSection["macroValues"] = std::move(macroValues);
            presetReloadSection["passed"] = restored.restored && sessionMatches;
            report["presetReload"] = std::move(presetReloadSection);
            require(restored.restored && sessionMatches, "Benchmark scene preset reload result changed unexpectedly.");
        }

        ordered_json loadProfileSwitchSection;
        {
            const auto initialProfile = drs::engine::findPhase1RuntimeLoadProfile(scene.loadProfileSwitch.initialProfileId);
            const auto downgradedProfile = drs::engine::findPhase1RuntimeLoadProfile(scene.loadProfileSwitch.downgradedProfileId);
            require(initialProfile.has_value() && downgradedProfile.has_value(),
                    "Benchmark scene load-profile switch profiles were unavailable.");

            auto sceneInstrument = referenceInstrument.instrument;
            sceneInstrument.defaultLoadProfile = scene.loadProfileSwitch.initialProfileId;

            drs::engine::RuntimeStreamingService service(
                referenceStream.container,
                drs::engine::buildRuntimeStreamingServiceOptions(*initialProfile, 5000));
            drs::engine::RuntimeVoice voice;
            std::string errorMessage;

            const auto start = Clock::now();
            const auto allocated = voice.allocate(sceneInstrument,
                                                  referenceStream.container,
                                                  {
                                                      scene.loadProfileSwitch.voice.voiceId,
                                                      "",
                                                      scene.loadProfileSwitch.voice.midiNote,
                                                      scene.loadProfileSwitch.voice.velocity,
                                                      {},
                                                      scene.loadProfileSwitch.voice.articulationId
                                                  },
                                                  errorMessage);
            require(allocated, "Benchmark scene load-profile switch voice allocation failed.");

            voice.advanceFrames(scene.loadProfileSwitch.warmupFrames, service);
            const auto boundaryAdvance = voice.advanceFrames(scene.loadProfileSwitch.boundaryFrames, service);
            const auto pageReady = waitUntil(
                [&] { return service.isPageReady({ "sine-a3", 0 }); },
                std::chrono::milliseconds(300));
            const auto resumedAdvance = voice.advanceFrames(scene.loadProfileSwitch.resumeFrames, service);
            const auto preSwitchMetrics = service.getMetrics();

            for (const auto& page : scene.loadProfileSwitch.additionalPageReads)
                require(service.enqueuePageRead({ page.sampleId, page.pageIndex }).accepted,
                        "Benchmark scene load-profile switch warm-cache request was rejected.");

            const auto extraReadsCompleted = waitUntil(
                [&]
                {
                    return service.getMetrics().backgroundReadCount >= scene.loadProfileSwitch.additionalPageReads.size() + 1;
                },
                std::chrono::milliseconds(800));

            service.applyLoadProfile(drs::engine::buildRuntimeStreamingServiceOptions(*downgradedProfile, 5000));
            const auto downgradedMetrics = service.getMetrics();
            const auto postSwitchAdvance = voice.advanceFrames(scene.loadProfileSwitch.resumeFrames, service);

            voice.beginRelease();
            const auto finished = waitUntil(
                [&]
                {
                    const auto advance = voice.advanceFrames(scene.loadProfileSwitch.releaseFrames, service);
                    return advance.voiceFinished
                        || voice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished;
                },
                std::chrono::milliseconds(1500));

            service.purgeDormantPages();
            const auto finalMetrics = service.getMetrics();
            const auto durationMicros = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();

            loadProfileSwitchSection["initialProfileId"] = scene.loadProfileSwitch.initialProfileId;
            loadProfileSwitchSection["downgradedProfileId"] = downgradedMetrics.activeLoadProfileId;
            loadProfileSwitchSection["waitedForPage"] = boundaryAdvance.waitingForPage;
            loadProfileSwitchSection["pageReady"] = pageReady;
            loadProfileSwitchSection["acquiredPageLease"] = resumedAdvance.acquiredPageLease;
            loadProfileSwitchSection["activeLeaseCountBeforeSwitch"] = preSwitchMetrics.activeLeaseCount;
            loadProfileSwitchSection["extraReadsCompleted"] = extraReadsCompleted;
            loadProfileSwitchSection["continuedAfterSwitch"] = postSwitchAdvance.advanced;
            loadProfileSwitchSection["finishedAfterRelease"] = finished;
            loadProfileSwitchSection["postPurgeResidentPageCount"] = finalMetrics.residentPageCount;
            loadProfileSwitchSection["evictedPageCount"] = finalMetrics.evictedPageCount;
            loadProfileSwitchSection["durationMicros"] = durationMicros;
            const bool loadProfileSwitchPassed = boundaryAdvance.waitingForPage
                && pageReady
                && resumedAdvance.acquiredPageLease
                && preSwitchMetrics.activeLeaseCount == scene.loadProfileSwitch.expectedActiveLeaseCountBeforeSwitch
                && extraReadsCompleted
                && downgradedMetrics.activeLoadProfileId == scene.loadProfileSwitch.downgradedProfileId
                && postSwitchAdvance.advanced
                && finished
                && finalMetrics.residentPageCount <= downgradedProfile->maxCachedPages
                && finalMetrics.evictedPageCount >= 1;
            loadProfileSwitchSection["passed"] = loadProfileSwitchPassed;
            report["loadProfileSwitch"] = std::move(loadProfileSwitchSection);
            require(loadProfileSwitchPassed, "Benchmark scene load-profile switch result changed unexpectedly.");
        }

        report["passed"] = true;
        writeTextFile(outputPath, report.dump(2) + "\n");
        std::cout << "Phase 1 benchmark scene passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        report["passed"] = false;
        report["error"] = exception.what();
        writeTextFile(outputPath, report.dump(2) + "\n");
        std::cerr << "Phase 1 benchmark scene failed: " << exception.what() << std::endl;
        return 1;
    }
}
