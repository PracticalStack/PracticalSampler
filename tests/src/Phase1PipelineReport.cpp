#include "drs/engine/EngineFacade.h"
#include "drs/engine/HostSessionState.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeStreamingService.h"
#include "drs/engine/RuntimeVoice.h"
#include "drs/engine/SampleImport.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <json/json.hpp>

#include <algorithm>
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
using ordered_json = nlohmann::ordered_json;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void serviceRestore(drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline)
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
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Could not open pipeline report artifact for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing pipeline report artifact: " + path.generic_string());
}

bool containsText(const std::vector<std::string>& messages, const std::string& needle)
{
    return std::any_of(messages.begin(),
                       messages.end(),
                       [&](const std::string& message)
                       {
                           return message.find(needle) != std::string::npos;
                       });
}

bool nearlyEqual(double actual, double expected, double tolerance = 0.0001)
{
    return std::abs(actual - expected) <= tolerance;
}

std::optional<double> findMacroValue(const drs::engine::EngineFacade& engineFacade, const std::string& macroId)
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        if (macro.id == macroId)
            return macro.currentValue;
    }

    return std::nullopt;
}

bool sessionMatchesLeadPerformance(const drs::engine::RuntimeSessionStateSnapshot& sessionState)
{
    if (sessionState.loadProfileId != "performance" || sessionState.selectedArticulationId != "lead")
        return false;

    const auto toneIterator = std::find_if(sessionState.macroValues.begin(),
                                           sessionState.macroValues.end(),
                                           [](const drs::engine::RuntimePresetMacroValue& macroValue)
                                           {
                                               return macroValue.id == "tone";
                                           });
    const auto motionIterator = std::find_if(sessionState.macroValues.begin(),
                                             sessionState.macroValues.end(),
                                             [](const drs::engine::RuntimePresetMacroValue& macroValue)
                                             {
                                                 return macroValue.id == "motion";
                                             });

    return toneIterator != sessionState.macroValues.end()
        && motionIterator != sessionState.macroValues.end()
        && nearlyEqual(toneIterator->value, 0.62)
        && nearlyEqual(motionIterator->value, 0.78);
}

bool legacyPresetMatchesLeadPerformance(const std::string& serializedState)
{
    const auto parsed = drs::engine::parseHostSessionState(serializedState);
    if (!parsed.isLegacyPreset() || !parsed.legacyPreset.has_value())
        return false;

    const auto& preset = *parsed.legacyPreset;
    if (preset.loadProfileId != "performance" || preset.selectedArticulationId != "lead")
        return false;

    const auto toneIterator = std::find_if(preset.macroValues.begin(), preset.macroValues.end(),
                                           [](const auto& macro) { return macro.id == "tone"; });
    const auto motionIterator = std::find_if(preset.macroValues.begin(), preset.macroValues.end(),
                                             [](const auto& macro) { return macro.id == "motion"; });
    return toneIterator != preset.macroValues.end()
        && motionIterator != preset.macroValues.end()
        && nearlyEqual(toneIterator->value, 0.62)
        && nearlyEqual(motionIterator->value, 0.78);
}

drs::engine::RuntimeStreamingServiceOptions makeStreamingOptions(
    const std::optional<drs::engine::RuntimeLoadProfileDefinition>& profile,
    std::size_t fallbackMaxCachedPages,
    std::uint64_t simulatedReadLatencyMicros)
{
    if (profile.has_value())
        return drs::engine::buildRuntimeStreamingServiceOptions(*profile, simulatedReadLatencyMicros);

    drs::engine::RuntimeStreamingServiceOptions options;
    options.maxCachedPages = fallbackMaxCachedPages;
    options.simulatedReadLatencyMicros = simulatedReadLatencyMicros;
    return options;
}

fs::path getReferenceDirectory()
{
    return fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath()).parent_path();
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
    require(sineImport.accepted, "Reference sine sample must inspect before the pipeline report runs.");

    const auto triangleImport = drs::engine::inspectSampleFile(trianglePath.generic_string());
    require(triangleImport.accepted, "Reference triangle sample must inspect before the pipeline report runs.");

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
    plan.projectNotes = {
        "Sprint 1 project fixture used to validate the product-owned runtime model and loader seam.",
        "The native importer and compiled stream writer are intentionally deferred to Sprint 2."
    };
    plan.instrumentValidationNotes = {
        "Uses the existing open HISE sample assets as stand-in sample sources for Sprint 1.",
        "Exercises two articulations, two groups, velocity-layer routing, and explicit prefetch metadata.",
        "Acts as the canonical loader fixture until the import compiler lands in Sprint 2."
    };

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

    drs::engine::RuntimeMacroDefinition tone;
    tone.id = "tone";
    tone.name = "Tone";
    tone.defaultValue = 0.35;
    tone.minValue = 0.0;
    tone.maxValue = 1.0;
    plan.macros.push_back(std::move(tone));

    drs::engine::RuntimeMacroDefinition motion;
    motion.id = "motion";
    motion.name = "Motion";
    motion.defaultValue = 0.15;
    motion.minValue = 0.0;
    motion.maxValue = 1.0;
    plan.macros.push_back(std::move(motion));

    drs::engine::RuntimeArticulationDefinition sustain;
    sustain.id = "sustain";
    sustain.name = "Sustain";
    sustain.isDefault = true;
    plan.articulations.push_back(std::move(sustain));

    drs::engine::RuntimeArticulationDefinition lead;
    lead.id = "lead";
    lead.name = "Triangle Lead";
    lead.isDefault = false;
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

    drs::engine::RuntimeCompileZoneDefinition padAccentZone;
    padAccentZone.id = "pad-a3-accent";
    padAccentZone.sourceId = "sine-a3";
    padAccentZone.groupId = "pad-core";
    padAccentZone.articulationId = "sustain";
    padAccentZone.rootKey = 57;
    padAccentZone.keyLow = 36;
    padAccentZone.keyHigh = 76;
    padAccentZone.velocityLow = 96;
    padAccentZone.velocityHigh = 127;
    padAccentZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(padAccentZone));

    drs::engine::RuntimeCompileZoneDefinition leadZone;
    leadZone.id = "lead-a4";
    leadZone.sourceId = "triangle-a4";
    leadZone.groupId = "lead-core";
    leadZone.articulationId = "lead";
    leadZone.rootKey = 69;
    leadZone.keyLow = 60;
    leadZone.keyHigh = 96;
    leadZone.velocityLow = 1;
    leadZone.velocityHigh = 95;
    leadZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(leadZone));

    drs::engine::RuntimeCompileZoneDefinition leadAccentZone;
    leadAccentZone.id = "lead-a4-accent";
    leadAccentZone.sourceId = "triangle-a4";
    leadAccentZone.groupId = "lead-core";
    leadAccentZone.articulationId = "lead";
    leadAccentZone.rootKey = 69;
    leadAccentZone.keyLow = 60;
    leadAccentZone.keyHigh = 96;
    leadAccentZone.velocityLow = 96;
    leadAccentZone.velocityHigh = 127;
    leadAccentZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(leadAccentZone));

    return plan;
}

ordered_json buildImportEntry(const drs::engine::SampleInspectionResult& result)
{
    ordered_json entry;
    entry["sourcePath"] = result.sourcePath;
    entry["imported"] = result.accepted;
    entry["state"] = result.state;
    entry["warningCount"] = result.warnings.size();
    entry["issueCount"] = result.issues.size();
    entry["warnings"] = result.warnings;
    entry["issues"] = result.issues;

    if (result.accepted)
    {
        entry["formatName"] = result.metadata.formatName;
        entry["sampleRate"] = result.metadata.sampleRate;
        entry["frameCount"] = result.metadata.frameCount;
        entry["channelCount"] = result.metadata.channelCount;
    }

    return entry;
}

ordered_json buildCompileEntry(const drs::engine::RuntimeCompileResult& result)
{
    ordered_json entry;
    entry["compiled"] = result.compiled;
    entry["state"] = result.state;
    entry["warningCount"] = result.warnings.size();
    entry["issueCount"] = result.issues.size();
    entry["warnings"] = result.warnings;
    entry["issues"] = result.issues;
    entry["streamSampleCount"] = result.streamSamples.size();
    entry["totalPayloadBytes"] = result.totalPayloadBytes;
    return entry;
}

template <typename TPredicate>
bool waitUntil(TPredicate predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return predicate();
}
} // namespace

int main(int argc, char* argv[])
{
    const auto outputPath = argc >= 2
        ? fs::path(argv[1])
        : fs::temp_directory_path() / "drs-phase1-pipeline-report.json";

    ordered_json report;
    report["report"] = "drs.phase1.pipelineStatus";
    report["referenceCorpusId"] = "tiny-open-instrument";

    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto referenceProject = drs::engine::loadPhase1ReferenceProjectManifest();
        const auto referenceInstrument = drs::engine::loadPhase1ReferenceInstrumentManifest();

        ordered_json loaderSection;
        loaderSection["projectLoaded"] = referenceProject.loaded;
        loaderSection["instrumentLoaded"] = referenceInstrument.loaded;
        loaderSection["projectIssues"] = referenceProject.issues;
        loaderSection["instrumentIssues"] = referenceInstrument.issues;
        loaderSection["sampleSourceCount"] = referenceProject.project.sampleSources.size();
        loaderSection["macroCount"] = referenceInstrument.metrics.macroCount;
        loaderSection["zoneCount"] = referenceInstrument.metrics.zoneCount;
        loaderSection["sourceProjectResolved"] = referenceInstrument.metrics.sourceProjectResolved;
        loaderSection["compiledStreamAssetResolved"] = referenceInstrument.metrics.compiledStreamAssetResolved;
        const bool loaderPassed = referenceProject.loaded
            && referenceInstrument.loaded
            && referenceInstrument.metrics.sourceProjectResolved
            && referenceInstrument.metrics.compiledStreamAssetResolved;
        loaderSection["passed"] = loaderPassed;
        report["loader"] = std::move(loaderSection);

        const auto referenceStream = drs::engine::loadRuntimeStreamContainerForInstrument(referenceInstrument);
        ordered_json streamReaderSection;
        streamReaderSection["loaded"] = referenceStream.loaded;
        streamReaderSection["state"] = referenceStream.state;
        streamReaderSection["issues"] = referenceStream.issues;
        streamReaderSection["sampleCount"] = referenceStream.metrics.sampleCount;
        streamReaderSection["pageCount"] = referenceStream.metrics.pageCount;
        streamReaderSection["checksumValidatedCount"] = referenceStream.metrics.checksumValidatedCount;
        const bool streamReaderPassed = referenceStream.loaded
            && referenceStream.metrics.sampleCount == 2
            && referenceStream.metrics.checksumValidatedCount == 2;
        streamReaderSection["passed"] = streamReaderPassed;
        report["streamReader"] = std::move(streamReaderSection);

        const auto ecoProfile = drs::engine::findPhase1RuntimeLoadProfile("eco");
        const auto balancedProfile = drs::engine::findPhase1RuntimeLoadProfile("balanced");
        const auto performanceProfile = drs::engine::findPhase1RuntimeLoadProfile("performance");

        drs::engine::RuntimeStreamingService schedulerSimulation(
            referenceStream.container,
            makeStreamingOptions(balancedProfile, 4, 4000));

        std::vector<drs::engine::RuntimeStreamPageRequest> schedulerRequests;
        for (const auto& sample : referenceStream.container.samples)
        {
            for (const auto& page : sample.pages)
                schedulerRequests.push_back({ sample.sampleId, page.pageIndex });
        }

        for (const auto& request : schedulerRequests)
            require(schedulerSimulation.enqueuePageRead(request).accepted,
                    "Scheduler simulation should accept all reference stream-page requests.");

        const auto schedulerSettled = waitUntil(
            [&]
            {
                return schedulerSimulation.getMetrics().backgroundReadCount >= schedulerRequests.size();
            },
            std::chrono::milliseconds(1000));

        const auto schedulerMetrics = schedulerSimulation.getMetrics();
        ordered_json schedulerSection;
        schedulerSection["settled"] = schedulerSettled;
        schedulerSection["backgroundReadCount"] = schedulerMetrics.backgroundReadCount;
        schedulerSection["cacheHitCount"] = schedulerMetrics.cacheHitCount;
        schedulerSection["pendingPageCount"] = schedulerMetrics.pendingPageCount;
        schedulerSection["peakPendingPageCount"] = schedulerMetrics.peakPendingPageCount;
        schedulerSection["residentPageCount"] = schedulerMetrics.residentPageCount;
        schedulerSection["workerThreadIdHash"] = schedulerMetrics.workerThreadIdHash;
        schedulerSection["requesterThreadIdHash"] = schedulerMetrics.requesterThreadIdHash;
        const bool schedulerPassed = schedulerSettled
            && schedulerMetrics.backgroundReadCount == schedulerRequests.size()
            && schedulerMetrics.pendingPageCount == 0
            && schedulerMetrics.workerThreadIdHash != 0
            && schedulerMetrics.workerThreadIdHash != schedulerMetrics.requesterThreadIdHash;
        schedulerSection["passed"] = schedulerPassed;
        report["streamScheduler"] = std::move(schedulerSection);

        drs::engine::RuntimeStreamingService voiceSimulationService(
            referenceStream.container,
            makeStreamingOptions(balancedProfile, 4, 4000));
        drs::engine::RuntimeVoice referenceVoice;
        std::string voiceErrorMessage;
        const auto voiceAllocated = referenceVoice.allocate(
            referenceInstrument.instrument,
            referenceStream.container,
            {
                501,
                "pad-a3",
                57,
                100,
                {
                    { "tone", 0.4 },
                    { "motion", 0.2 }
                }
            },
            voiceErrorMessage);

        bool voiceWaitedForPage = false;
        bool voiceAcquiredLease = false;
        bool voiceFinished = false;

        if (voiceAllocated)
        {
            const auto headAdvance = referenceVoice.advanceFrames(4096, voiceSimulationService);
            (void)headAdvance;
            const auto boundaryAdvance = referenceVoice.advanceFrames(64, voiceSimulationService);
            voiceWaitedForPage = boundaryAdvance.waitingForPage;

            if (voiceWaitedForPage)
            {
                const auto voicePageReady = waitUntil(
                    [&] { return voiceSimulationService.isPageReady({ "sine-a3", 0 }); },
                    std::chrono::milliseconds(400));

                if (voicePageReady)
                {
                    const auto resumedAdvance = referenceVoice.advanceFrames(64, voiceSimulationService);
                    voiceAcquiredLease = resumedAdvance.acquiredPageLease;
                    referenceVoice.beginRelease();
                    voiceFinished = waitUntil(
                        [&]
                        {
                            const auto advance = referenceVoice.advanceFrames(8192, voiceSimulationService);
                            return advance.voiceFinished
                                || referenceVoice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished;
                        },
                        std::chrono::milliseconds(1500));
                }
            }
        }

        const auto voiceSnapshot = referenceVoice.getSnapshot();
        const auto voiceMetrics = voiceSimulationService.getMetrics();
        ordered_json voiceSection;
        voiceSection["allocated"] = voiceAllocated;
        voiceSection["waitedForPage"] = voiceWaitedForPage;
        voiceSection["acquiredLease"] = voiceAcquiredLease;
        voiceSection["finished"] = voiceFinished;
        voiceSection["finalState"] = static_cast<int>(voiceSnapshot.state);
        voiceSection["activeLeaseCount"] = voiceMetrics.activeLeaseCount;
        voiceSection["errorMessage"] = voiceErrorMessage;
        const bool voicePassed = voiceAllocated
            && voiceWaitedForPage
            && voiceAcquiredLease
            && voiceFinished
            && voiceSnapshot.state == drs::engine::RuntimeVoiceLifecycleState::finished
            && voiceSnapshot.cursor.currentLeaseId == 0
            && voiceMetrics.activeLeaseCount == 0;
        voiceSection["passed"] = voicePassed;
        report["voiceRuntime"] = std::move(voiceSection);

        const auto defaultSoftRoute = drs::engine::resolveRuntimeVoiceRoute(
            referenceInstrument.instrument,
            referenceStream.container,
            { 1001, "", 57, 64, {}, "" });
        const auto defaultAccentRoute = drs::engine::resolveRuntimeVoiceRoute(
            referenceInstrument.instrument,
            referenceStream.container,
            { 1002, "", 57, 120, {}, "" });
        const auto leadSoftRoute = drs::engine::resolveRuntimeVoiceRoute(
            referenceInstrument.instrument,
            referenceStream.container,
            { 1003, "", 69, 72, {}, "lead" });
        const auto leadAccentRoute = drs::engine::resolveRuntimeVoiceRoute(
            referenceInstrument.instrument,
            referenceStream.container,
            { 1004, "", 69, 120, {}, "lead" });
        const auto invalidArticulationRoute = drs::engine::resolveRuntimeVoiceRoute(
            referenceInstrument.instrument,
            referenceStream.container,
            { 1005, "", 69, 90, {}, "unknown-articulation" });

        bool routedPlaybackAllocated = false;
        bool routedPlaybackWaited = false;
        bool routedPlaybackResumed = false;
        std::string noteRoutingErrorMessage;

        drs::engine::RuntimeStreamingService noteRoutingService(
            referenceStream.container,
            makeStreamingOptions(balancedProfile, 4, 4000));
        drs::engine::RuntimeVoice routedPlaybackVoice;
        if (routedPlaybackVoice.allocate(referenceInstrument.instrument,
                                         referenceStream.container,
                                         { 1006, "", 57, 120, { { "tone", 0.6 } }, "" },
                                         noteRoutingErrorMessage))
        {
            routedPlaybackAllocated = true;
            const auto headAdvance = routedPlaybackVoice.advanceFrames(4096, noteRoutingService);
            (void)headAdvance;
            const auto boundaryAdvance = routedPlaybackVoice.advanceFrames(64, noteRoutingService);
            routedPlaybackWaited = boundaryAdvance.waitingForPage;

            if (routedPlaybackWaited
                && waitUntil([&] { return noteRoutingService.isPageReady({ "sine-a3", 0 }); },
                             std::chrono::milliseconds(400)))
            {
                routedPlaybackResumed = routedPlaybackVoice.advanceFrames(64, noteRoutingService).acquiredPageLease;
            }
        }

        ordered_json noteRoutingSection;
        noteRoutingSection["defaultSoftResolved"] = defaultSoftRoute.resolved;
        noteRoutingSection["defaultSoftZoneId"] = defaultSoftRoute.zoneId;
        noteRoutingSection["defaultAccentResolved"] = defaultAccentRoute.resolved;
        noteRoutingSection["defaultAccentZoneId"] = defaultAccentRoute.zoneId;
        noteRoutingSection["leadSoftResolved"] = leadSoftRoute.resolved;
        noteRoutingSection["leadSoftZoneId"] = leadSoftRoute.zoneId;
        noteRoutingSection["leadAccentResolved"] = leadAccentRoute.resolved;
        noteRoutingSection["leadAccentZoneId"] = leadAccentRoute.zoneId;
        noteRoutingSection["defaultArticulationUsed"] = defaultSoftRoute.usedDefaultArticulation;
        noteRoutingSection["invalidArticulationRejected"] = !invalidArticulationRoute.resolved;
        noteRoutingSection["invalidArticulationState"] = invalidArticulationRoute.state;
        noteRoutingSection["routedPlaybackAllocated"] = routedPlaybackAllocated;
        noteRoutingSection["routedPlaybackWaited"] = routedPlaybackWaited;
        noteRoutingSection["routedPlaybackResumed"] = routedPlaybackResumed;
        noteRoutingSection["errorMessage"] = noteRoutingErrorMessage;
        const bool noteRoutingPassed = defaultSoftRoute.resolved
            && defaultSoftRoute.zoneId == "pad-a3"
            && defaultAccentRoute.resolved
            && defaultAccentRoute.zoneId == "pad-a3-accent"
            && leadSoftRoute.resolved
            && leadSoftRoute.zoneId == "lead-a4"
            && leadAccentRoute.resolved
            && leadAccentRoute.zoneId == "lead-a4-accent"
            && defaultSoftRoute.usedDefaultArticulation
            && !invalidArticulationRoute.resolved
            && routedPlaybackAllocated
            && routedPlaybackWaited
            && routedPlaybackResumed;
        noteRoutingSection["passed"] = noteRoutingPassed;
        report["noteRouting"] = std::move(noteRoutingSection);

        ordered_json loadProfileSection;
        loadProfileSection["profiles"] = ordered_json::array();
        for (const auto& profile : drs::engine::getPhase1RuntimeLoadProfiles())
        {
            ordered_json profileEntry;
            profileEntry["id"] = profile.id;
            profileEntry["displayName"] = profile.displayName;
            profileEntry["maxPrefetchBytesPerVoice"] = profile.maxPrefetchBytesPerVoice;
            profileEntry["maxCachedPages"] = profile.maxCachedPages;
            profileEntry["summary"] = profile.summary;
            loadProfileSection["profiles"].push_back(std::move(profileEntry));
        }

        const bool loadProfilesPresent = ecoProfile.has_value()
            && balancedProfile.has_value()
            && performanceProfile.has_value();
        loadProfileSection["profilesPresent"] = loadProfilesPresent;

        bool loadProfileBudgetsAscending = false;
        bool loadProfileDowngradedToEco = false;
        bool loadProfileLeasePreserved = false;
        bool loadProfileContinuedAfterDowngrade = false;
        bool loadProfilePurgedDormantPages = false;
        bool loadProfileUnknownRejected = false;
        std::uint64_t ecoVoicePrefetchBytes = 0;
        std::uint64_t balancedVoicePrefetchBytes = 0;
        std::uint64_t performanceVoicePrefetchBytes = 0;
        std::size_t postPurgeResidentPageCount = 0;
        std::size_t postPurgeEvictedPageCount = 0;
        std::string loadProfileErrorMessage;

        if (loadProfilesPresent && referenceInstrument.loaded && referenceStream.loaded)
        {
            loadProfileBudgetsAscending = ecoProfile->maxPrefetchBytesPerVoice < balancedProfile->maxPrefetchBytesPerVoice
                && balancedProfile->maxPrefetchBytesPerVoice < performanceProfile->maxPrefetchBytesPerVoice
                && ecoProfile->maxCachedPages < balancedProfile->maxCachedPages
                && balancedProfile->maxCachedPages < performanceProfile->maxCachedPages;

            auto ecoInstrument = referenceInstrument.instrument;
            ecoInstrument.defaultLoadProfile = "eco";
            auto balancedInstrument = referenceInstrument.instrument;
            balancedInstrument.defaultLoadProfile = "balanced";
            auto performanceInstrument = referenceInstrument.instrument;
            performanceInstrument.defaultLoadProfile = "performance";

            drs::engine::RuntimeVoice ecoVoice;
            drs::engine::RuntimeVoice balancedVoice;
            drs::engine::RuntimeVoice performanceVoice;

            if (ecoVoice.allocate(ecoInstrument,
                                  referenceStream.container,
                                  { 801, "pad-a3", 57, 100, {} },
                                  loadProfileErrorMessage))
            {
                ecoVoicePrefetchBytes = ecoVoice.getSnapshot().cursor.prefetchBytes;
            }

            if (balancedVoice.allocate(balancedInstrument,
                                       referenceStream.container,
                                       { 802, "pad-a3", 57, 100, {} },
                                       loadProfileErrorMessage))
            {
                balancedVoicePrefetchBytes = balancedVoice.getSnapshot().cursor.prefetchBytes;
            }

            if (performanceVoice.allocate(performanceInstrument,
                                          referenceStream.container,
                                          { 803, "pad-a3", 57, 100, {} },
                                          loadProfileErrorMessage))
            {
                performanceVoicePrefetchBytes = performanceVoice.getSnapshot().cursor.prefetchBytes;
            }

            drs::engine::RuntimeStreamingService loadProfileService(
                referenceStream.container,
                drs::engine::buildRuntimeStreamingServiceOptions(*performanceProfile, 5000));
            drs::engine::RuntimeVoice activeLoadProfileVoice;

            if (activeLoadProfileVoice.allocate(performanceInstrument,
                                                referenceStream.container,
                                                { 901, "pad-a3", 57, 100, { { "tone", 0.55 } } },
                                                loadProfileErrorMessage))
            {
                const auto headAdvance = activeLoadProfileVoice.advanceFrames(4096, loadProfileService);
                (void)headAdvance;
                const auto boundaryAdvance = activeLoadProfileVoice.advanceFrames(64, loadProfileService);

                if (boundaryAdvance.waitingForPage
                    && waitUntil([&] { return loadProfileService.isPageReady({ "sine-a3", 0 }); },
                                 std::chrono::milliseconds(400)))
                {
                    const auto resumedAdvance = activeLoadProfileVoice.advanceFrames(64, loadProfileService);
                    loadProfileLeasePreserved = resumedAdvance.acquiredPageLease;

                    for (const auto& request : std::vector<drs::engine::RuntimeStreamPageRequest> {
                             { "sine-a3", 1 },
                             { "sine-a3", 2 },
                             { "triangle-a4", 0 }
                         })
                    {
                        loadProfileService.enqueuePageRead(request);
                    }

                    waitUntil(
                        [&]
                        {
                            return loadProfileService.getMetrics().backgroundReadCount >= 4;
                        },
                        std::chrono::milliseconds(800));

                    loadProfileService.applyLoadProfile(
                        drs::engine::buildRuntimeStreamingServiceOptions(*ecoProfile, 5000));
                    const auto downgradedMetrics = loadProfileService.getMetrics();
                    loadProfileDowngradedToEco = downgradedMetrics.activeLoadProfileId == "eco"
                        && downgradedMetrics.configuredMaxCachedPages == ecoProfile->maxCachedPages;
                    loadProfileLeasePreserved = loadProfileLeasePreserved
                        && downgradedMetrics.activeLeaseCount == 1;

                    const auto postDowngradeAdvance = activeLoadProfileVoice.advanceFrames(64, loadProfileService);
                    loadProfileContinuedAfterDowngrade = postDowngradeAdvance.advanced
                        && activeLoadProfileVoice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::active;

                    activeLoadProfileVoice.beginRelease();
                    const auto finishedAfterRelease = waitUntil(
                        [&]
                        {
                            const auto advance = activeLoadProfileVoice.advanceFrames(8192, loadProfileService);
                            return advance.voiceFinished
                                || activeLoadProfileVoice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished;
                        },
                        std::chrono::milliseconds(1500));

                    if (finishedAfterRelease && loadProfileService.getMetrics().activeLeaseCount == 0)
                    {
                        loadProfileService.purgeDormantPages();
                        const auto finalLoadProfileMetrics = loadProfileService.getMetrics();
                        postPurgeResidentPageCount = finalLoadProfileMetrics.residentPageCount;
                        postPurgeEvictedPageCount = finalLoadProfileMetrics.evictedPageCount;
                        loadProfilePurgedDormantPages = finalLoadProfileMetrics.residentPageCount <= ecoProfile->maxCachedPages
                            && finalLoadProfileMetrics.evictedPageCount >= 1;
                    }
                }
            }

            auto invalidProfileInstrument = referenceInstrument.instrument;
            invalidProfileInstrument.defaultLoadProfile = "unknown-profile";
            drs::engine::RuntimeVoice invalidProfileVoice;
            loadProfileUnknownRejected = !invalidProfileVoice.allocate(invalidProfileInstrument,
                                                                       referenceStream.container,
                                                                       { 902, "pad-a3", 57, 100, {} },
                                                                       loadProfileErrorMessage)
                && !loadProfileErrorMessage.empty();
        }

        loadProfileSection["budgetsAscending"] = loadProfileBudgetsAscending;
        loadProfileSection["ecoVoicePrefetchBytes"] = ecoVoicePrefetchBytes;
        loadProfileSection["balancedVoicePrefetchBytes"] = balancedVoicePrefetchBytes;
        loadProfileSection["performanceVoicePrefetchBytes"] = performanceVoicePrefetchBytes;
        loadProfileSection["downgradedToEco"] = loadProfileDowngradedToEco;
        loadProfileSection["activeLeasePreserved"] = loadProfileLeasePreserved;
        loadProfileSection["continuedAfterDowngrade"] = loadProfileContinuedAfterDowngrade;
        loadProfileSection["purgedDormantPages"] = loadProfilePurgedDormantPages;
        loadProfileSection["postPurgeResidentPageCount"] = postPurgeResidentPageCount;
        loadProfileSection["postPurgeEvictedPageCount"] = postPurgeEvictedPageCount;
        loadProfileSection["unknownProfileRejected"] = loadProfileUnknownRejected;
        loadProfileSection["errorMessage"] = loadProfileErrorMessage;
        const bool loadProfilePassed = loadProfilesPresent
            && loadProfileBudgetsAscending
            && ecoVoicePrefetchBytes == 8192
            && balancedVoicePrefetchBytes == 16384
            && performanceVoicePrefetchBytes == 16384
            && loadProfileDowngradedToEco
            && loadProfileLeasePreserved
            && loadProfileContinuedAfterDowngrade
            && loadProfilePurgedDormantPages
            && loadProfileUnknownRejected;
        loadProfileSection["passed"] = loadProfilePassed;
        report["loadProfile"] = std::move(loadProfileSection);

        ordered_json runtimeCountersSection;
        bool runtimeCountersAllocated = false;
        bool runtimeCountersWaited = false;
        bool runtimeCountersRecovered = false;
        bool runtimeCountersPassed = false;
        std::string runtimeCountersErrorMessage;

        if (performanceProfile.has_value() && ecoProfile.has_value())
        {
            drs::engine::RuntimeStreamingService runtimeCounterService(
                referenceStream.container,
                drs::engine::buildRuntimeStreamingServiceOptions(*performanceProfile, 5000));
            drs::engine::RuntimeVoice counterVoiceA;
            drs::engine::RuntimeVoice counterVoiceB;
            drs::engine::RuntimeVoice counterVoiceC;

            runtimeCountersAllocated = counterVoiceA.allocate(referenceInstrument.instrument,
                                                              referenceStream.container,
                                                              { 1201, "", 57, 64, { { "tone", 0.3 } }, "" },
                                                              runtimeCountersErrorMessage)
                && counterVoiceB.allocate(referenceInstrument.instrument,
                                          referenceStream.container,
                                          { 1202, "", 57, 120, { { "tone", 0.75 } }, "" },
                                          runtimeCountersErrorMessage)
                && counterVoiceC.allocate(referenceInstrument.instrument,
                                          referenceStream.container,
                                          { 1203, "", 69, 120, { { "motion", 0.45 } }, "lead" },
                                          runtimeCountersErrorMessage);

            if (runtimeCountersAllocated)
            {
                counterVoiceA.advanceFrames(4096, runtimeCounterService);
                counterVoiceB.advanceFrames(4096, runtimeCounterService);
                counterVoiceC.advanceFrames(2048, runtimeCounterService);

                runtimeCountersWaited = counterVoiceA.advanceFrames(64, runtimeCounterService).waitingForPage
                    && counterVoiceB.advanceFrames(64, runtimeCounterService).waitingForPage
                    && counterVoiceC.advanceFrames(64, runtimeCounterService).waitingForPage;

                if (runtimeCountersWaited
                    && waitUntil(
                        [&]
                        {
                            return runtimeCounterService.isPageReady({ "sine-a3", 0 })
                                && runtimeCounterService.isPageReady({ "triangle-a4", 0 });
                        },
                        std::chrono::milliseconds(500)))
                {
                    runtimeCountersRecovered = counterVoiceA.advanceFrames(64, runtimeCounterService).acquiredPageLease
                        && counterVoiceB.advanceFrames(64, runtimeCounterService).acquiredPageLease
                        && counterVoiceC.advanceFrames(64, runtimeCounterService).acquiredPageLease;

                    for (const auto& request : std::vector<drs::engine::RuntimeStreamPageRequest> {
                             { "sine-a3", 1 },
                             { "sine-a3", 2 },
                             { "triangle-a4", 1 },
                             { "triangle-a4", 2 }
                         })
                    {
                        runtimeCounterService.enqueuePageRead(request);
                    }

                    waitUntil(
                        [&]
                        {
                            return runtimeCounterService.getMetrics().backgroundReadCount >= 6;
                        },
                        std::chrono::milliseconds(1000));

                    counterVoiceA.beginRelease();
                    counterVoiceB.beginRelease();
                    counterVoiceC.beginRelease();

                    waitUntil(
                        [&]
                        {
                            const auto advanceA = counterVoiceA.advanceFrames(8192, runtimeCounterService);
                            const auto advanceB = counterVoiceB.advanceFrames(8192, runtimeCounterService);
                            const auto advanceC = counterVoiceC.advanceFrames(8192, runtimeCounterService);
                            return (advanceA.voiceFinished
                                        || counterVoiceA.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished)
                                && (advanceB.voiceFinished
                                        || counterVoiceB.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished)
                                && (advanceC.voiceFinished
                                        || counterVoiceC.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished);
                        },
                        std::chrono::milliseconds(1500));

                    runtimeCounterService.applyLoadProfile(
                        drs::engine::buildRuntimeStreamingServiceOptions(*ecoProfile, 5000));
                    runtimeCounterService.purgeDormantPages();
                }
            }

            const auto runtimeCounterMetrics = runtimeCounterService.getMetrics();
            runtimeCountersSection["allocated"] = runtimeCountersAllocated;
            runtimeCountersSection["waitedForPages"] = runtimeCountersWaited;
            runtimeCountersSection["recoveredAfterReads"] = runtimeCountersRecovered;
            runtimeCountersSection["pageMissCount"] = runtimeCounterMetrics.pageMissCount;
            runtimeCountersSection["headUsageCount"] = runtimeCounterMetrics.headUsageCount;
            runtimeCountersSection["headFramesRead"] = runtimeCounterMetrics.headFramesRead;
            runtimeCountersSection["headBytesRead"] = runtimeCounterMetrics.headBytesRead;
            runtimeCountersSection["activeVoiceCount"] = runtimeCounterMetrics.activeVoiceCount;
            runtimeCountersSection["peakActiveVoiceCount"] = runtimeCounterMetrics.peakActiveVoiceCount;
            runtimeCountersSection["averageReadLatencyMicros"] = runtimeCounterMetrics.averageReadLatencyMicros;
            runtimeCountersSection["maxReadLatencyMicros"] = runtimeCounterMetrics.maxReadLatencyMicros;
            runtimeCountersSection["purgePassCount"] = runtimeCounterMetrics.purgePassCount;
            runtimeCountersSection["dormantPurgeCount"] = runtimeCounterMetrics.dormantPurgeCount;
            runtimeCountersSection["lastPurgeEvictedPageCount"] = runtimeCounterMetrics.lastPurgeEvictedPageCount;
            runtimeCountersSection["evictedPageCount"] = runtimeCounterMetrics.evictedPageCount;
            runtimeCountersSection["residentPageCount"] = runtimeCounterMetrics.residentPageCount;
            runtimeCountersSection["errorMessage"] = runtimeCountersErrorMessage;

            runtimeCountersPassed = runtimeCountersAllocated
                && runtimeCountersWaited
                && runtimeCountersRecovered
                && runtimeCounterMetrics.pageMissCount >= 3
                && runtimeCounterMetrics.headUsageCount >= 3
                && runtimeCounterMetrics.headFramesRead >= 10240
                && runtimeCounterMetrics.headBytesRead >= 49152
                && runtimeCounterMetrics.peakActiveVoiceCount >= 3
                && runtimeCounterMetrics.activeVoiceCount == 0
                && runtimeCounterMetrics.averageReadLatencyMicros > 0
                && runtimeCounterMetrics.maxReadLatencyMicros >= runtimeCounterMetrics.averageReadLatencyMicros
                && runtimeCounterMetrics.purgePassCount >= 1
                && runtimeCounterMetrics.dormantPurgeCount >= 1
                && runtimeCounterMetrics.evictedPageCount >= 1
                && runtimeCounterMetrics.residentPageCount <= ecoProfile->maxCachedPages;
            runtimeCountersSection["passed"] = runtimeCountersPassed;
            report["runtimeCounters"] = std::move(runtimeCountersSection);

            if (!runtimeCountersPassed)
                throw std::runtime_error("Runtime counter section did not meet the Phase 1 observability expectations.");
        }
        else
        {
            runtimeCountersSection["passed"] = false;
            runtimeCountersSection["errorMessage"] = "Required load profiles were unavailable for runtime counter capture.";
            report["runtimeCounters"] = std::move(runtimeCountersSection);
            throw std::runtime_error("Runtime counter section could not run because required load profiles were unavailable.");
        }

        const auto referencePlan = buildReferenceCompilePlan(getReferenceDirectory());
        const auto tempDirectory = fs::temp_directory_path() / "drs-phase1-pipeline-report";
        const auto tempPlan = buildReferenceCompilePlan(tempDirectory);

        ordered_json importerSection;
        importerSection["samples"] = ordered_json::array();
        bool importerPassed = true;
        for (const auto& source : referencePlan.sampleSources)
        {
            const auto importResult = drs::engine::inspectSampleFile(source.sourcePath);
            importerSection["samples"].push_back(buildImportEntry(importResult));
            importerPassed = importerPassed && importResult.accepted && importResult.issues.empty();
        }
        importerSection["passed"] = importerPassed;
        report["importer"] = std::move(importerSection);

        const auto referenceCompile = drs::engine::compileRuntimeInstrument(referencePlan);
        const auto secondReferenceCompile = drs::engine::compileRuntimeInstrument(referencePlan);
        const auto tempCompile = drs::engine::compileRuntimeInstrument(tempPlan);

        const auto referenceProjectJson = drs::engine::serializeRuntimeProjectManifest(referenceCompile.project,
                                                                                       referencePlan.outputProjectPath);
        const auto referenceInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(referenceCompile.instrument,
                                                                                            referencePlan.outputInstrumentPath);
        const auto referenceStreamJson = drs::engine::serializePrototypeStreamContainer(referenceCompile,
                                                                                         referencePlan.outputStreamPath);

        const auto deterministicProject = referenceProjectJson
            == drs::engine::serializeRuntimeProjectManifest(secondReferenceCompile.project,
                                                            referencePlan.outputProjectPath);
        const auto deterministicInstrument = referenceInstrumentJson
            == drs::engine::serializeRuntimeInstrumentManifest(secondReferenceCompile.instrument,
                                                               referencePlan.outputInstrumentPath);
        const auto deterministicStream = referenceStreamJson
            == drs::engine::serializePrototypeStreamContainer(secondReferenceCompile,
                                                              referencePlan.outputStreamPath);

        const auto checkedInProjectPath = getReferenceDirectory() / "tiny-open-instrument.drsproj";
        const auto checkedInInstrumentPath = getReferenceDirectory() / "tiny-open-instrument.drinst";
        const auto checkedInStreamPath = getReferenceDirectory() / "tiny-open-instrument.drstrm";

        const bool goldenProjectMatch = referenceProjectJson == readTextFile(checkedInProjectPath);
        const bool goldenInstrumentMatch = referenceInstrumentJson == readTextFile(checkedInInstrumentPath);
        const bool goldenStreamMatch = referenceStreamJson == readTextFile(checkedInStreamPath);

        const auto tempProjectJson = drs::engine::serializeRuntimeProjectManifest(tempCompile.project,
                                                                                  tempPlan.outputProjectPath);
        const auto tempInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(tempCompile.instrument,
                                                                                       tempPlan.outputInstrumentPath);
        const auto tempStreamJson = drs::engine::serializePrototypeStreamContainer(tempCompile,
                                                                                   tempPlan.outputStreamPath);

        writeTextFile(fs::path(tempPlan.outputProjectPath), tempProjectJson);
        writeTextFile(fs::path(tempPlan.outputInstrumentPath), tempInstrumentJson);
        writeTextFile(fs::path(tempPlan.outputStreamPath), tempStreamJson);

        const auto tempLoadedProject = drs::engine::loadRuntimeProjectManifest(tempPlan.outputProjectPath);
        const auto tempLoadedInstrument = drs::engine::loadRuntimeInstrumentManifest(tempPlan.outputInstrumentPath);

        ordered_json compileSection = buildCompileEntry(referenceCompile);
        compileSection["deterministicProject"] = deterministicProject;
        compileSection["deterministicInstrument"] = deterministicInstrument;
        compileSection["deterministicStream"] = deterministicStream;
        compileSection["goldenProjectMatch"] = goldenProjectMatch;
        compileSection["goldenInstrumentMatch"] = goldenInstrumentMatch;
        compileSection["goldenStreamMatch"] = goldenStreamMatch;
        compileSection["tempProjectLoaded"] = tempLoadedProject.loaded;
        compileSection["tempInstrumentLoaded"] = tempLoadedInstrument.loaded;
        const bool compilePassed = referenceCompile.compiled
            && secondReferenceCompile.compiled
            && tempCompile.compiled
            && deterministicProject
            && deterministicInstrument
            && deterministicStream
            && goldenProjectMatch
            && goldenInstrumentMatch
            && goldenStreamMatch
            && tempLoadedProject.loaded
            && tempLoadedInstrument.loaded;
        compileSection["passed"] = compilePassed;
        report["compilePath"] = std::move(compileSection);

        ordered_json corruptionSection;
        const auto missingDefaultManifest = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "negative-corpus"
            / "missing-default-articulation"
            / "missing-default-articulation.drinst";
        const auto missingSampleManifest = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "negative-corpus"
            / "missing-sample-file"
            / "missing-sample-file.drinst";
        const auto malformedJsonManifest = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "negative-corpus"
            / "malformed-json"
            / "malformed-json.drinst";

        const auto missingDefaultResult = drs::engine::loadRuntimeInstrumentManifest(missingDefaultManifest.generic_string());
        const auto missingSampleResult = drs::engine::loadRuntimeInstrumentManifest(missingSampleManifest.generic_string());
        const auto malformedJsonResult = drs::engine::loadRuntimeInstrumentManifest(malformedJsonManifest.generic_string());

        fs::remove(fs::path(tempPlan.outputStreamPath));
        const auto missingGeneratedStreamResult = drs::engine::loadRuntimeInstrumentManifest(tempPlan.outputInstrumentPath);

        const auto malformedGeneratedInstrumentPath = tempDirectory / "tiny-open-instrument-corrupt.drinst";
        writeTextFile(malformedGeneratedInstrumentPath, "{ invalid json\n");
        const auto malformedGeneratedResult = drs::engine::loadRuntimeInstrumentManifest(malformedGeneratedInstrumentPath.generic_string());

        const bool missingDefaultRejected = !missingDefaultResult.loaded
            && containsText(missingDefaultResult.issues, "default articulation");
        const bool missingSampleRejected = !missingSampleResult.loaded
            && containsText(missingSampleResult.issues, "Zone sample does not exist");
        const bool malformedFixtureRejected = !malformedJsonResult.loaded
            && containsText(malformedJsonResult.issues, "JSON parse failed");
        const bool missingGeneratedStreamRejected = !missingGeneratedStreamResult.loaded
            && containsText(missingGeneratedStreamResult.issues, "Compiled stream asset must exist");
        const bool malformedGeneratedRejected = !malformedGeneratedResult.loaded
            && containsText(malformedGeneratedResult.issues, "JSON parse failed");

        corruptionSection["missingDefaultRejected"] = missingDefaultRejected;
        corruptionSection["missingSampleRejected"] = missingSampleRejected;
        corruptionSection["malformedFixtureRejected"] = malformedFixtureRejected;
        corruptionSection["missingGeneratedStreamRejected"] = missingGeneratedStreamRejected;
        corruptionSection["malformedGeneratedRejected"] = malformedGeneratedRejected;
        corruptionSection["passed"] = missingDefaultRejected
            && missingSampleRejected
            && malformedFixtureRejected
            && missingGeneratedStreamRejected
            && malformedGeneratedRejected;
        report["corruptionChecks"] = std::move(corruptionSection);

        const auto presetRoot = fs::path(drs::engine::getPhase1RuntimeRootPath()) / "preset-state";
        const auto leadPresetPath = presetRoot / "reference" / "lead-performance-state.drpreset.json";
        const auto negativePresetPath = presetRoot / "negative" / "transient-diagnostics-leak.drpreset.json";
        const auto leadPresetJson = readTextFile(leadPresetPath);
        const auto negativePresetJson = readTextFile(negativePresetPath);

        ordered_json stateRecallSection;
        bool standaloneFixtureRestored = false;
        bool standaloneExportMatchesFixture = false;
        bool standaloneReloadMatchesFixture = false;
        bool pluginFixtureRestored = false;
        bool pluginExportMatchesFixture = false;
        bool pluginReloadMatchesFixture = false;
        bool pluginParameterSurfaceMatchesMacros = false;
        bool macroStateComparePassed = false;
        bool invalidRestorePreservedLastGoodState = false;

        drs::standalone::MainComponent standaloneSource;
        const auto standaloneRestore = standaloneSource.restoreStateJson(leadPresetJson);
        serviceRestore(standaloneSource.getProcessor());
        standaloneFixtureRestored = standaloneRestore.restored;

        const auto standaloneExportedState = standaloneSource.exportStateJson();
        standaloneExportMatchesFixture = legacyPresetMatchesLeadPerformance(standaloneExportedState);

        drs::standalone::MainComponent standaloneReloaded;
        const auto standaloneReload = standaloneReloaded.restoreStateJson(standaloneExportedState);
        serviceRestore(standaloneReloaded.getProcessor());
        standaloneReloadMatchesFixture = standaloneReload.restored
            && sessionMatchesLeadPerformance(standaloneReloaded.getEngineFacade().getCurrentSessionState());

        drs::plugin::Processor sourceProcessor;
        sourceProcessor.setStateInformation(leadPresetJson.data(), static_cast<int>(leadPresetJson.size()));
        serviceRestore(sourceProcessor);
        pluginFixtureRestored = sessionMatchesLeadPerformance(sourceProcessor.getEngineFacade().getCurrentSessionState());

        juce::MemoryBlock pluginState;
        sourceProcessor.getStateInformation(pluginState);
        const auto pluginStateJson = std::string(static_cast<const char*>(pluginState.getData()), pluginState.getSize());
        pluginExportMatchesFixture = legacyPresetMatchesLeadPerformance(pluginStateJson);

        drs::plugin::Processor restoredProcessor;
        restoredProcessor.setStateInformation(pluginState.getData(), static_cast<int>(pluginState.getSize()));
        serviceRestore(restoredProcessor);
        pluginReloadMatchesFixture = sessionMatchesLeadPerformance(restoredProcessor.getEngineFacade().getCurrentSessionState());

        const auto restoredToneValue = findMacroValue(restoredProcessor.getEngineFacade(), "tone");
        const auto restoredMotionValue = findMacroValue(restoredProcessor.getEngineFacade(), "motion");
        auto* restoredToneParameter = dynamic_cast<juce::RangedAudioParameter*>(
            restoredProcessor.getParameterState().getParameter("macro.tone"));
        auto* restoredMotionParameter = dynamic_cast<juce::RangedAudioParameter*>(
            restoredProcessor.getParameterState().getParameter("macro.motion"));

        pluginParameterSurfaceMatchesMacros = restoredToneParameter != nullptr
            && restoredMotionParameter != nullptr
            && restoredProcessor.getParameterState().getRawParameterValue("macro.tone") != nullptr
            && restoredProcessor.getParameterState().getRawParameterValue("macro.motion") != nullptr
            && nearlyEqual(static_cast<double>(restoredProcessor.getParameterState().getRawParameterValue("macro.tone")->load()),
                           0.62)
            && nearlyEqual(static_cast<double>(restoredProcessor.getParameterState().getRawParameterValue("macro.motion")->load()),
                           0.78);
        macroStateComparePassed = restoredToneValue.has_value()
            && restoredMotionValue.has_value()
            && nearlyEqual(*restoredToneValue, 0.62)
            && nearlyEqual(*restoredMotionValue, 0.78)
            && pluginParameterSurfaceMatchesMacros;

        const auto previousPluginState = restoredProcessor.getEngineFacade().exportPresetStateJson();
        restoredProcessor.setStateInformation(negativePresetJson.data(), static_cast<int>(negativePresetJson.size()));
        serviceRestore(restoredProcessor);
        const auto rejectedRestore = restoredProcessor.getProjectRestoreSnapshot();
        invalidRestorePreservedLastGoodState = restoredProcessor.getEngineFacade().exportPresetStateJson() == previousPluginState
            && rejectedRestore != nullptr
            && rejectedRestore->state == drs::engine::ProjectRestoreState::failed;

        stateRecallSection["standaloneFixtureRestored"] = standaloneFixtureRestored;
        stateRecallSection["standaloneExportMatchesFixture"] = standaloneExportMatchesFixture;
        stateRecallSection["standaloneReloadMatchesFixture"] = standaloneReloadMatchesFixture;
        stateRecallSection["pluginFixtureRestored"] = pluginFixtureRestored;
        stateRecallSection["pluginExportMatchesFixture"] = pluginExportMatchesFixture;
        stateRecallSection["pluginReloadMatchesFixture"] = pluginReloadMatchesFixture;
        stateRecallSection["pluginParameterSurfaceMatchesMacros"] = pluginParameterSurfaceMatchesMacros;
        stateRecallSection["macroStateComparePassed"] = macroStateComparePassed;
        stateRecallSection["invalidRestorePreservedLastGoodState"] = invalidRestorePreservedLastGoodState;
        const bool stateRecallPassed = standaloneFixtureRestored
            && standaloneExportMatchesFixture
            && standaloneReloadMatchesFixture
            && pluginFixtureRestored
            && pluginExportMatchesFixture
            && pluginReloadMatchesFixture
            && macroStateComparePassed
            && invalidRestorePreservedLastGoodState;
        stateRecallSection["passed"] = stateRecallPassed;
        report["stateRecall"] = std::move(stateRecallSection);

        ordered_json errorHandlingSection;
        drs::engine::EngineFacade errorFacade;
        const auto restoreLeadResult = errorFacade.restorePresetStateJson(leadPresetJson);
        const bool baselineSessionLoaded = restoreLeadResult.restored
            && sessionMatchesLeadPerformance(errorFacade.getCurrentSessionState());

        const auto missingPackProbe = errorFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::missingContent);
        const bool missingPackHandledGracefully = missingPackProbe.attempted
            && missingPackProbe.failedGracefully
            && containsText(missingPackProbe.issues, "Zone sample does not exist")
            && sessionMatchesLeadPerformance(errorFacade.getCurrentSessionState());

        const auto checksumProbe = errorFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::badChecksum);
        const bool checksumHandledGracefully = checksumProbe.attempted
            && checksumProbe.failedGracefully
            && containsText(checksumProbe.issues, "checksum mismatch")
            && sessionMatchesLeadPerformance(errorFacade.getCurrentSessionState());

        const auto schemaProbe = errorFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::schemaMismatch);
        const bool schemaHandledGracefully = schemaProbe.attempted
            && schemaProbe.failedGracefully
            && containsText(schemaProbe.issues, "schemaName")
            && sessionMatchesLeadPerformance(errorFacade.getCurrentSessionState());

        const auto partialProbe = errorFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::partialCompiledArtifact);
        const bool partialArtifactHandledGracefully = partialProbe.attempted
            && partialProbe.failedGracefully
            && containsText(partialProbe.issues, "Compiled stream asset must exist")
            && sessionMatchesLeadPerformance(errorFacade.getCurrentSessionState());

        const auto diagnosticsAfterProbe = errorFacade.getDiagnosticsSnapshot();
        const bool diagnosticsSurfacedFailure = diagnosticsAfterProbe.lastContentProbeCategory == "partial-compiled-artifact"
            && diagnosticsAfterProbe.lastContentProbeFailedGracefully
            && !diagnosticsAfterProbe.failureState.empty();

        errorFacade.clearContentFailureProbe();
        const auto diagnosticsAfterClear = errorFacade.getDiagnosticsSnapshot();
        const bool clearedVisibleFailure = diagnosticsAfterClear.lastContentProbeCategory.empty()
            && diagnosticsAfterClear.failureState.empty();

        errorHandlingSection["baselineSessionLoaded"] = baselineSessionLoaded;
        errorHandlingSection["missingPackHandledGracefully"] = missingPackHandledGracefully;
        errorHandlingSection["checksumHandledGracefully"] = checksumHandledGracefully;
        errorHandlingSection["schemaHandledGracefully"] = schemaHandledGracefully;
        errorHandlingSection["partialArtifactHandledGracefully"] = partialArtifactHandledGracefully;
        errorHandlingSection["diagnosticsSurfacedFailure"] = diagnosticsSurfacedFailure;
        errorHandlingSection["clearedVisibleFailure"] = clearedVisibleFailure;
        const bool errorHandlingPassed = baselineSessionLoaded
            && missingPackHandledGracefully
            && checksumHandledGracefully
            && schemaHandledGracefully
            && partialArtifactHandledGracefully
            && diagnosticsSurfacedFailure
            && clearedVisibleFailure
            && report["corruptionChecks"].at("passed").get<bool>();
        errorHandlingSection["passed"] = errorHandlingPassed;
        report["errorHandling"] = std::move(errorHandlingSection);

        const bool loadValidationPassed = loaderPassed
            && streamReaderPassed
            && importerPassed
            && compilePassed;
        const bool playValidationPassed = schedulerPassed
            && voicePassed
            && noteRoutingPassed
            && loadProfilePassed
            && runtimeCountersPassed;

        ordered_json nightlyValidationSection;
        nightlyValidationSection["load"] = {
            { "passed", loadValidationPassed },
            { "sources", ordered_json::array({ "loader", "streamReader", "importer", "compilePath" }) }
        };
        nightlyValidationSection["play"] = {
            { "passed", playValidationPassed },
            { "sources", ordered_json::array({ "streamScheduler", "voiceRuntime", "noteRouting", "loadProfile", "runtimeCounters" }) }
        };
        nightlyValidationSection["stateRecall"] = {
            { "passed", stateRecallPassed },
            { "sources", ordered_json::array({ "stateRecall" }) }
        };
        nightlyValidationSection["errorHandling"] = {
            { "passed", errorHandlingPassed },
            { "sources", ordered_json::array({ "errorHandling", "corruptionChecks" }) }
        };
        nightlyValidationSection["passed"] = loadValidationPassed
            && playValidationPassed
            && stateRecallPassed
            && errorHandlingPassed;
        report["nightlyValidation"] = std::move(nightlyValidationSection);

        const bool overallPassed = loadValidationPassed
            && playValidationPassed
            && stateRecallPassed
            && errorHandlingPassed;
        report["passed"] = overallPassed;
        writeTextFile(outputPath, report.dump(2) + "\n");
        std::cout << report.dump(2) << std::endl;

        require(overallPassed, "Phase 1 pipeline report detected one or more failing sections.");
        return 0;
    }
    catch (const std::exception& exception)
    {
        report["passed"] = false;
        report["fatalError"] = exception.what();
        try
        {
            writeTextFile(outputPath, report.dump(2) + "\n");
        }
        catch (const std::exception&)
        {
        }

        std::cerr << "Phase 1 pipeline report failed: " << exception.what() << std::endl;
        return 1;
    }
}
