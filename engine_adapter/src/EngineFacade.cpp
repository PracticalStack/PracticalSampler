#include "drs/engine/EngineFacade.h"
#include "drs/engine/HiseFrontendBridge.h"
#include "drs/engine/HiseProjectContent.h"
#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimePresetState.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStreamingService.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeVoice.h"
#include "drs/engine/HiseVendorInfo.generated.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace drs::engine
{
namespace
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return {};

    if (issues.size() == 1)
        return issues.front();

    return issues.front() + " (+" + std::to_string(issues.size() - 1) + " more)";
}

std::string summarizeSnapshotFindings(const std::vector<PlaybackSnapshotFinding>& findings)
{
    if (findings.empty())
        return {};

    if (findings.size() == 1)
        return findings.front().message;

    return findings.front().message + " (+" + std::to_string(findings.size() - 1) + " more)";
}

std::string summarizeDigest(const std::string& digest)
{
    if (digest.empty())
        return "none";

    constexpr std::size_t prefixLength = 18;
    if (digest.size() <= prefixLength)
        return digest;

    return digest.substr(0, prefixLength) + "...";
}

std::string buildMacroSummary(const RuntimeSessionStateSnapshot& sessionState)
{
    if (sessionState.macroValues.empty())
        return "none";

    std::ostringstream stream;

    for (std::size_t index = 0; index < sessionState.macroValues.size(); ++index)
    {
        if (index != 0)
            stream << ", ";

        stream << sessionState.macroValues[index].id << "=" << sessionState.macroValues[index].value;
    }

    return stream.str();
}

std::optional<double> findMacroValue(const RuntimeSessionStateSnapshot& sessionState, const std::string& macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const RuntimePresetMacroValue& macroValue)
                                       {
                                           return macroValue.id == macroId;
                                       });
    if (iterator == sessionState.macroValues.end())
        return std::nullopt;

    return iterator->value;
}

double normalizeMacroValue(double value)
{
    constexpr auto precisionScale = 1000000.0;
    return std::round(value * precisionScale) / precisionScale;
}

int clampMidiValue(int value)
{
    return std::clamp(value, 0, 127);
}

int computeTonePreviewVelocity(const RuntimeSessionStateSnapshot& sessionState, int fallbackVelocity)
{
    const auto toneValue = findMacroValue(sessionState, "tone").value_or(0.35);
    const auto effectiveVelocity = static_cast<int>(std::lround(32.0 + toneValue * 95.0));
    return std::clamp(effectiveVelocity, 1, 127);
}

int computeMotionPreviewNote(const RuntimeSessionStateSnapshot& sessionState, int playedNote)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.15);
    const auto semitoneOffset = static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
    return clampMidiValue(playedNote + semitoneOffset);
}

std::string buildToneCurrentEffect(const RuntimeSessionStateSnapshot& sessionState)
{
    const auto toneValue = findMacroValue(sessionState, "tone").value_or(0.35);
    if (toneValue >= 0.75)
        return "Accent attack";
    if (toneValue >= 0.4)
        return "Balanced attack";
    return "Soft attack";
}

std::string buildMotionCurrentEffect(const RuntimeSessionStateSnapshot& sessionState)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.15);
    const auto semitoneOffset = static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
    if (semitoneOffset == 0)
        return "Centered pitch";

    const auto direction = semitoneOffset > 0 ? "+" : "";
    return direction + std::to_string(semitoneOffset) + " st";
}

std::string buildAppliedMacroSummary(const RuntimeSessionStateSnapshot& sessionState)
{
    return "Tone: " + buildToneCurrentEffect(sessionState) + " | Motion: " + buildMotionCurrentEffect(sessionState);
}

std::string resolveDraftSurfaceSource(const DraftPlaybackStatus& status)
{
    if (status.performance.available)
        return "published draft";

    if (status.preview.available)
        return "preview fallback";

    return "default fallback";
}

std::string resolveRendererMode(bool referenceInstrumentActive)
{
    return referenceInstrumentActive ? "reference-backed" : "inactive";
}

void syncDraftPlaybackIntoDiagnostics(const DraftPlaybackStatus& status,
                                      EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.draftRevision = status.draftRevision;
    diagnosticsSnapshot.previewRevision = status.preview.revision;
    diagnosticsSnapshot.publishedRevision = status.performance.revision;
    diagnosticsSnapshot.previewBuildId = status.preview.buildId;
    diagnosticsSnapshot.publishedBuildId = status.performance.buildId;
    diagnosticsSnapshot.previewPreparedBuildId = status.preview.preparedBuildId;
    diagnosticsSnapshot.publishedPreparedBuildId = status.performance.preparedBuildId;
    diagnosticsSnapshot.previewPending = status.pendingPreview.active;
    diagnosticsSnapshot.publishedPending = status.pendingPerformance.active;
    diagnosticsSnapshot.previewActivationEligible = status.preview.activationEligible;
    diagnosticsSnapshot.publishedActivationEligible = status.performance.activationEligible;
    diagnosticsSnapshot.previewRevisionState = status.preview.state;
    diagnosticsSnapshot.publishedRevisionState = status.performance.state;
    diagnosticsSnapshot.previewContentDigest = status.preview.contentDigest;
    diagnosticsSnapshot.publishedContentDigest = status.performance.contentDigest;
    diagnosticsSnapshot.previewPreparedContentDigest = status.preview.preparedContentDigest;
    diagnosticsSnapshot.publishedPreparedContentDigest = status.performance.preparedContentDigest;
    diagnosticsSnapshot.previewPreparedSampleCount = status.preview.preparedSampleCount;
    diagnosticsSnapshot.previewPreparedStreamCount = status.preview.preparedStreamCount;
    diagnosticsSnapshot.previewPreparedZoneCount = status.preview.preparedZoneCount;
    diagnosticsSnapshot.previewPreparedOwnershipRecordCount = status.preview.preparedOwnershipRecordCount;
    diagnosticsSnapshot.publishedPreparedSampleCount = status.performance.preparedSampleCount;
    diagnosticsSnapshot.publishedPreparedStreamCount = status.performance.preparedStreamCount;
    diagnosticsSnapshot.publishedPreparedZoneCount = status.performance.preparedZoneCount;
    diagnosticsSnapshot.publishedPreparedOwnershipRecordCount = status.performance.preparedOwnershipRecordCount;
    diagnosticsSnapshot.previewPreparedBytes = status.preview.preparedBytes;
    diagnosticsSnapshot.publishedPreparedBytes = status.performance.preparedBytes;
    diagnosticsSnapshot.previewPreparedOwnershipBytes = status.preview.preparedOwnershipBytes;
    diagnosticsSnapshot.publishedPreparedOwnershipBytes = status.performance.preparedOwnershipBytes;
    diagnosticsSnapshot.previewPreparationCacheHits = status.preview.preparationCacheHitCount;
    diagnosticsSnapshot.previewPreparationCacheMisses = status.preview.preparationCacheMissCount;
    diagnosticsSnapshot.publishedPreparationCacheHits = status.performance.preparationCacheHitCount;
    diagnosticsSnapshot.publishedPreparationCacheMisses = status.performance.preparationCacheMissCount;
    diagnosticsSnapshot.previewFindings = status.preview.findings;
    diagnosticsSnapshot.publishedFindings = status.performance.findings;
    if (status.performance.playableRangeAvailable && status.performance.available)
    {
        diagnosticsSnapshot.playableRangeAvailable = true;
        diagnosticsSnapshot.lowestPlayableNote = status.performance.lowestPlayableNote;
        diagnosticsSnapshot.highestPlayableNote = status.performance.highestPlayableNote;
        diagnosticsSnapshot.playableRangeSource = "published";
    }
    else if (status.preview.playableRangeAvailable && status.preview.available)
    {
        diagnosticsSnapshot.playableRangeAvailable = true;
        diagnosticsSnapshot.lowestPlayableNote = status.preview.lowestPlayableNote;
        diagnosticsSnapshot.highestPlayableNote = status.preview.highestPlayableNote;
        diagnosticsSnapshot.playableRangeSource = "preview";
    }
    else
    {
        diagnosticsSnapshot.playableRangeSource = "default";
    }
    diagnosticsSnapshot.surfaceStateSource = resolveDraftSurfaceSource(status);
    diagnosticsSnapshot.draftPlaybackEvent = status.lastEvent;
}

void syncPreparedPlaybackWorkerIntoDiagnostics(const PreparedPlaybackWorkerStatus& workerStatus,
                                               EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.preparedWorkerPendingCount = workerStatus.pendingWorkCount;
    diagnosticsSnapshot.preparedWorkerConfiguredMaxPendingCount = workerStatus.configuredMaxPendingWorkCount;
    diagnosticsSnapshot.preparedWorkerConfiguredMaxInFlightCount = workerStatus.configuredMaxInFlightWorkCount;
    diagnosticsSnapshot.preparedWorkerCancellationCount = workerStatus.cancellationCount;
    diagnosticsSnapshot.preparedWorkerSupersededCount = workerStatus.supersededCount;
    diagnosticsSnapshot.preparedWorkerFailureCount = workerStatus.failureCount;
    diagnosticsSnapshot.preparedWorkerMaxPendingCount = workerStatus.maxPendingWorkCount;
    diagnosticsSnapshot.preparedWorkerActiveOwnershipRecordCount = workerStatus.activeOwnershipRecordCount;
    diagnosticsSnapshot.preparedWorkerRetiredOwnershipRecordCount = workerStatus.retiredOwnershipRecordCount;
    diagnosticsSnapshot.preparedWorkerRetiredBytes = workerStatus.retiredBytesAwaitingCleanup;
    diagnosticsSnapshot.preparedWorkerEvent = workerStatus.lastEvent;
    diagnosticsSnapshot.preparedWorkerLastCancellationLane = workerStatus.lastCancellationLane;
    diagnosticsSnapshot.preparedWorkerLastCancellationReason = workerStatus.lastCancellationReason;
    diagnosticsSnapshot.preparedWorkerLastSupersededLane = workerStatus.lastSupersededLane;
    diagnosticsSnapshot.preparedWorkerLastSupersededReason = workerStatus.lastSupersededReason;
}

void syncSessionSelectionsIntoDiagnostics(const RuntimeSessionStateSnapshot& sessionState,
                                          EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.presetId = sessionState.presetId;
    diagnosticsSnapshot.loadProfileId = sessionState.loadProfileId;
    diagnosticsSnapshot.selectedArticulationId = sessionState.selectedArticulationId;
}

const RuntimeArticulationDefinition* findArticulationDefinition(const RuntimeInstrumentModel& instrument,
                                                                const std::string& articulationId)
{
    const auto iterator = std::find_if(instrument.articulations.begin(),
                                       instrument.articulations.end(),
                                       [&](const RuntimeArticulationDefinition& articulation)
                                       {
                                           return articulation.id == articulationId;
                                       });
    return iterator != instrument.articulations.end() ? &(*iterator) : nullptr;
}

std::string resolveArticulationName(const RuntimeManifestLoadResult& manifest,
                                    const RuntimeSessionStateSnapshot& sessionState)
{
    if (!manifest.loaded)
        return {};

    if (const auto* articulation = findArticulationDefinition(manifest.instrument, sessionState.selectedArticulationId))
        return articulation->name;

    return {};
}

std::string buildLoadIndicator(const RuntimeManifestLoadResult& manifest,
                               const RuntimeStreamLoadResult& stream,
                               const RuntimeSessionStateSnapshot& sessionState)
{
    if (!manifest.loaded)
        return "Manifest unavailable";

    if (!stream.loaded)
        return "Stream unavailable";

    if (!sessionState.transientMetrics.lastFailure.empty())
        return "Attention required";

    return "Reference instrument ready";
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

void drainVoice(RuntimeVoice& voice, RuntimeStreamingService& service)
{
    const auto drained = waitUntil(
        [&]
        {
            const auto advance = voice.advanceFrames(8192, service);
            return advance.voiceFinished
                || voice.getSnapshot().state == RuntimeVoiceLifecycleState::finished;
        },
        std::chrono::milliseconds(1500));

    if (!drained)
        throw std::runtime_error("Diagnostics voice did not finish draining in time.");
}

std::vector<RuntimeMacroValueSnapshot> toVoiceMacroValues(const RuntimeSessionStateSnapshot& sessionState)
{
    std::vector<RuntimeMacroValueSnapshot> macroValues;
    macroValues.reserve(sessionState.macroValues.size());

    for (const auto& macroValue : sessionState.macroValues)
        macroValues.push_back({ macroValue.id, macroValue.value });

    return macroValues;
}

std::string readTextFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

std::string getFailureCategoryId(const EngineContentFailureCategory category)
{
    switch (category)
    {
    case EngineContentFailureCategory::missingContent:
        return "missing-content";
    case EngineContentFailureCategory::badChecksum:
        return "bad-checksum";
    case EngineContentFailureCategory::schemaMismatch:
        return "schema-mismatch";
    case EngineContentFailureCategory::partialCompiledArtifact:
        return "partial-compiled-artifact";
    }

    return "unknown";
}

fs::path getFailureFixturePath(const EngineContentFailureCategory category)
{
    const auto runtimeRoot = fs::path(getPhase1RuntimeRootPath());

    switch (category)
    {
    case EngineContentFailureCategory::missingContent:
        return runtimeRoot / "negative-corpus" / "missing-sample-file" / "missing-sample-file.drinst";
    case EngineContentFailureCategory::schemaMismatch:
        return runtimeRoot / "negative-corpus" / "schema-mismatch" / "schema-mismatch.drinst";
    case EngineContentFailureCategory::partialCompiledArtifact:
        return runtimeRoot / "negative-corpus" / "partial-compiled-artifact" / "partial-compiled-artifact.drinst";
    case EngineContentFailureCategory::badChecksum:
        break;
    }

    return {};
}

fs::path buildChecksumMismatchFixture()
{
    const auto referencePath = fs::path(getPhase1ReferenceStreamContainerPath());
    const auto tempDirectory = fs::temp_directory_path() / "drs-phase1-failure-probes";
    const auto outputPath = tempDirectory / "checksum-mismatch.drstrm";

    auto checksumCorruptJson = ordered_json::parse(readTextFile(referencePath.generic_string()));
    for (auto& sample : checksumCorruptJson["samples"])
    {
        const fs::path sourcePath(sample["sourcePath"].get<std::string>());
        sample["sourcePath"] = (referencePath.parent_path() / sourcePath).lexically_normal().generic_string();
    }
    checksumCorruptJson["samples"][0]["sourceChecksumHex"] = "deadbeefdeadbeef";
    writeTextFile(outputPath, checksumCorruptJson.dump(2) + "\n");
    return outputPath;
}
} // namespace

EngineFacade::EngineFacade()
    : referenceManifest(loadPhase1ReferenceInstrumentManifest()),
      preparedPlaybackService("phase1-prepared-playback-v2", 2, true)
{
    authoringProject = loadPhase2ReferenceProjectManifest();

    if (referenceManifest.loaded)
    {
        currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
        currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
        referenceStream = loadPhase1ReferenceStreamContainer();
        preparedPlaybackService.setBackgroundWorkerStream(referenceStream);
        referenceInstrumentActive = referenceStream.loaded;
    }
    else
    {
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
        referenceInstrumentActive = false;
    }

    initializeDraftPlaybackContract(false);
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

std::vector<HiseFrontendExportProfile> EngineFacade::getFrontendExportProfiles() const
{
    return {
        {
            "HISE frontend plugin",
            HiseFrontendTargetKind::plugin,
            true,
            false,
            true,
            false,
            false,
            true,
            "hi_backend/backend/ProjectTemplate.cpp",
            "Selected first integration target. Uses USE_FRONTEND, disables IS_STANDALONE_APP, and expects VST SDK inputs for exporter workflows."
        },
        {
            "HISE frontend standalone",
            HiseFrontendTargetKind::standalone,
            true,
            true,
            false,
            true,
            true,
            false,
            "hi_backend/backend/StandaloneProjectTemplate.cpp",
            "Frontend export for a standalone app target. Enables IS_STANDALONE_APP and typically needs the ASIO SDK for Windows low-latency device support."
        }
    };
}

bool EngineFacade::serviceBackgroundWork()
{
    return pumpPreparedPlaybackWorkerCompletions();
}

EngineStatusSnapshot EngineFacade::getStatusSnapshot() const
{
    using namespace generated;

    std::ostringstream detail;
    const auto profiles = getFrontendExportProfiles();
    const auto linkedFrontend = getLinkedHiseFrontendSnapshot();
    const auto contentSnapshot = getHiseProjectContentSnapshot();
    const auto runtimeManifest = loadPhase1ReferenceInstrument();
    const auto runtimeStream = loadPhase1ReferenceStream();
    const auto diagnostics = getDiagnosticsSnapshot();

    detail << "HISE root: " << hiseVendorRoot << "\n";
    detail << "Pinned HISE commit: " << hiseCurrentGitHash << "\n";
    detail << "hi_core module version: " << hiseHiCoreVersion << "\n";
    detail << "REST API version macro: " << hiseRestApiVersion << "\n";
    detail << "Nested HISE JUCE snapshot: " << (hiseNestedJucePresent ? "present" : "missing") << "\n";
    detail << "Projucer Windows binary: " << (hiseProjucerWindowsPresent ? "present" : "missing") << "\n";
    detail << "HISE SDK zip: " << (hiseSdkZipPresent ? "present" : "missing") << "\n";
    detail << "HISE SDK extracted: " << (hiseSdkExtracted ? "yes" : "no") << "\n";
    detail << "Linked frontend bridge: " << (linkedFrontend.linked ? "yes" : "no") << "\n";

    if (linkedFrontend.linked)
    {
        detail << "Linked plugin name: " << linkedFrontend.pluginName << "\n";
        detail << "Linked manufacturer: " << linkedFrontend.manufacturer << "\n";
        detail << "Linked HISE build sub-version: " << linkedFrontend.buildSubVersion << "\n";
        detail << "Linked macro profile: USE_BACKEND=" << (linkedFrontend.useBackend ? "1" : "0")
               << ", USE_FRONTEND=" << (linkedFrontend.useFrontend ? "1" : "0")
               << ", FRONTEND_IS_PLUGIN=" << (linkedFrontend.frontendIsPlugin ? "1" : "0")
               << ", IS_STANDALONE_APP=" << (linkedFrontend.isStandaloneApp ? "1" : "0")
               << ", IS_STANDALONE_FRONTEND=" << (linkedFrontend.isStandaloneFrontend ? "1" : "0") << "\n";
    }

    detail << "\nProject content seam:\n";
    detail << "Repo root: " << contentSnapshot.repoRoot << "\n";
    detail << "Repo HISE content root: " << contentSnapshot.repoContentRoot
           << " (" << (contentSnapshot.repoContentRootExists ? "present" : "missing") << ")\n";
    detail << "Runtime AppData root: "
           << (contentSnapshot.runtimeAppDataRoot.empty() ? "unavailable" : contentSnapshot.runtimeAppDataRoot) << "\n";
    detail << "Discovered repo user presets: " << contentSnapshot.presetFileCount << "\n";
    detail << "Discovered repo sample maps: " << contentSnapshot.sampleMapFileCount << "\n";
    detail << "Repo content directories:\n";

    for (const auto& directory : contentSnapshot.repoDirectories)
    {
        detail << "- " << directory.name
               << ": " << (directory.exists ? "present" : "missing")
               << ", matching files=" << directory.matchingFileCount
               << ", path=" << directory.absolutePath << "\n";
    }

    detail << "Runtime content directories:\n";

    for (const auto& directory : contentSnapshot.runtimeDirectories)
    {
        detail << "- " << directory.name
               << ": " << (directory.exists ? "present" : "missing")
               << ", matching files=" << directory.matchingFileCount
               << ", path=" << directory.absolutePath << "\n";
    }

    detail << "\nConcrete HISE frontend target profiles:\n";

    for (const auto& profile : profiles)
    {
        std::string sdkSummary;

        if (profile.requiresAsioSdk)
            sdkSummary += "ASIO";

        if (profile.requiresVst3Sdk)
        {
            if (!sdkSummary.empty())
                sdkSummary += ", ";

            sdkSummary += "VST3";
        }

        if (sdkSummary.empty())
            sdkSummary = "none";

        detail << "- " << profile.name << "\n";
        detail << "  template: " << profile.sourceTemplate << "\n";
        detail << "  USE_FRONTEND=1"
               << ", IS_STANDALONE_APP=" << (profile.isStandaloneApp ? "1" : "0")
               << ", FRONTEND_IS_PLUGIN=" << (profile.frontendIsPlugin ? "1" : "0")
               << ", IS_STANDALONE_FRONTEND=" << (profile.isStandaloneFrontend ? "1" : "0") << "\n";
        detail << "  requires SDKs: " << sdkSummary << "\n";
        detail << "  summary: " << profile.summary << "\n";
    }

    detail << "\nPhase 1 runtime bootstrap:\n";
    detail << "Runtime fixture root: " << getPhase1RuntimeRootPath() << "\n";
    detail << "Reference corpus index: " << getPhase1ReferenceCorpusIndexPath() << "\n";
    detail << "Reference manifest: " << runtimeManifest.manifestPath
           << " (" << (runtimeManifest.manifestFound ? "present" : "missing") << ")\n";
    detail << "Reference load state: " << runtimeManifest.state << "\n";

    if (runtimeManifest.loaded)
    {
        detail << "Loaded instrument: " << runtimeManifest.instrument.displayName
               << " [" << runtimeManifest.instrument.instrumentId << "]\n";
        detail << "Schema: " << runtimeManifest.instrument.schemaName
               << " v" << runtimeManifest.instrument.schemaVersion << "\n";
        detail << "Source project: " << runtimeManifest.instrument.sourceProjectPath << "\n";
        detail << "Compiled stream asset: " << runtimeManifest.instrument.compiledStreamAssetPath << "\n";
        detail << "Load profile: " << runtimeManifest.instrument.defaultLoadProfile << "\n";
        detail << "Counts: macros=" << runtimeManifest.metrics.macroCount
               << ", articulations=" << runtimeManifest.metrics.articulationCount
               << ", groups=" << runtimeManifest.metrics.groupCount
               << ", zones=" << runtimeManifest.metrics.zoneCount << "\n";
        detail << "Streaming seam: " << (runtimeManifest.metrics.usesStreaming ? "present" : "missing")
               << ", total prefetch bytes=" << runtimeManifest.metrics.totalPrefetchBytes << "\n";
        detail << "Baseline metrics: manifest bytes=" << runtimeManifest.metrics.manifestSizeBytes
               << ", load micros=" << runtimeManifest.metrics.loadDurationMicros
               << ", source project resolved=" << (runtimeManifest.metrics.sourceProjectResolved ? "yes" : "no")
               << ", stream asset resolved=" << (runtimeManifest.metrics.compiledStreamAssetResolved ? "yes" : "no") << "\n";
        detail << "Stream reader state: " << runtimeStream.state << "\n";

        if (runtimeStream.loaded)
        {
            detail << "Stream container: " << runtimeStream.container.containerId
                   << ", samples=" << runtimeStream.metrics.sampleCount
                   << ", pages=" << runtimeStream.metrics.pageCount
                   << ", checksum validations=" << runtimeStream.metrics.checksumValidatedCount << "\n";
        }
    }

    detail << "\nCurrent session state:\n";
    detail << "Preset id: " << (currentSessionState.presetId.empty() ? "unavailable" : currentSessionState.presetId) << "\n";
    detail << "Target instrument: "
           << (currentSessionState.targetInstrumentId.empty() ? "unavailable" : currentSessionState.targetInstrumentId)
           << " [" << (currentSessionState.targetInstrumentSchemaName.empty() ? "n/a" : currentSessionState.targetInstrumentSchemaName)
           << " v" << currentSessionState.targetInstrumentSchemaVersion << "]\n";
    detail << "Selected load profile: "
           << (currentSessionState.loadProfileId.empty() ? "unavailable" : currentSessionState.loadProfileId) << "\n";
    detail << "Selected articulation: "
           << (currentSessionState.selectedArticulationId.empty() ? "unavailable" : currentSessionState.selectedArticulationId) << "\n";
    detail << "Macro values: " << buildMacroSummary(currentSessionState) << "\n";
    detail << "Draft playback: draft=" << diagnostics.draftRevision
           << ", preview=" << diagnostics.previewRevision
           << " (" << (diagnostics.previewRevisionState.empty() ? "unavailable" : diagnostics.previewRevisionState) << ")"
           << ", published=" << diagnostics.publishedRevision
           << " (" << (diagnostics.publishedRevisionState.empty() ? "unavailable" : diagnostics.publishedRevisionState) << ")\n";
    detail << "Draft playback event: "
           << (diagnostics.draftPlaybackEvent.empty() ? "not reported" : diagnostics.draftPlaybackEvent) << "\n";
    detail << "Snapshot ids: previewBuild=" << diagnostics.previewBuildId
           << ", publishBuild=" << diagnostics.publishedBuildId << "\n";
    detail << "Snapshot digests: preview=" << summarizeDigest(diagnostics.previewContentDigest)
           << ", publish=" << summarizeDigest(diagnostics.publishedContentDigest) << "\n";
    detail << "Snapshot findings: preview=" << diagnostics.previewFindings.size()
           << ", publish=" << diagnostics.publishedFindings.size() << "\n";
    detail << "Prepared playback ids: preview=" << diagnostics.previewPreparedBuildId
           << ", publish=" << diagnostics.publishedPreparedBuildId << "\n";
    detail << "Prepared playback digests: preview=" << summarizeDigest(diagnostics.previewPreparedContentDigest)
           << ", publish=" << summarizeDigest(diagnostics.publishedPreparedContentDigest) << "\n";
    detail << "Prepared playback assets: preview samples=" << diagnostics.previewPreparedSampleCount
           << ", preview streams=" << diagnostics.previewPreparedStreamCount
           << ", preview ownership=" << diagnostics.previewPreparedOwnershipRecordCount
           << ", publish samples=" << diagnostics.publishedPreparedSampleCount
           << ", publish streams=" << diagnostics.publishedPreparedStreamCount
           << ", publish ownership=" << diagnostics.publishedPreparedOwnershipRecordCount << "\n";
    detail << "Playable range: source=" << (diagnostics.playableRangeSource.empty() ? "unavailable" : diagnostics.playableRangeSource)
           << ", available=" << (diagnostics.playableRangeAvailable ? "yes" : "no")
           << ", low=" << diagnostics.lowestPlayableNote
           << ", high=" << diagnostics.highestPlayableNote << "\n";
    detail << "Surface provenance: source=" << (diagnostics.surfaceStateSource.empty() ? "unavailable" : diagnostics.surfaceStateSource)
           << ", renderer=" << (diagnostics.rendererMode.empty() ? "unavailable" : diagnostics.rendererMode) << "\n";
    detail << "Prepared playback bytes: preview=" << diagnostics.previewPreparedBytes
           << ", publish=" << diagnostics.publishedPreparedBytes
           << ", previewOwnership=" << diagnostics.previewPreparedOwnershipBytes
           << ", publishOwnership=" << diagnostics.publishedPreparedOwnershipBytes << "\n";
    detail << "Prepared worker: pending=" << diagnostics.preparedWorkerPendingCount
           << ", queueLimit=" << diagnostics.preparedWorkerConfiguredMaxPendingCount
           << ", inFlightLimit=" << diagnostics.preparedWorkerConfiguredMaxInFlightCount
           << ", canceled=" << diagnostics.preparedWorkerCancellationCount
           << ", superseded=" << diagnostics.preparedWorkerSupersededCount
           << ", failures=" << diagnostics.preparedWorkerFailureCount
           << ", maxPending=" << diagnostics.preparedWorkerMaxPendingCount
           << ", activeOwnership=" << diagnostics.preparedWorkerActiveOwnershipRecordCount
           << ", retiredOwnership=" << diagnostics.preparedWorkerRetiredOwnershipRecordCount
           << ", retiredBytes=" << diagnostics.preparedWorkerRetiredBytes << "\n";
    detail << "Prepared worker event: "
           << (diagnostics.preparedWorkerEvent.empty() ? "not reported" : diagnostics.preparedWorkerEvent) << "\n";
    detail << "Prepared worker queue reasons: cancel["
           << (diagnostics.preparedWorkerLastCancellationLane.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastCancellationLane)
           << "]="
           << (diagnostics.preparedWorkerLastCancellationReason.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastCancellationReason)
           << ", supersede["
           << (diagnostics.preparedWorkerLastSupersededLane.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastSupersededLane)
           << "]="
           << (diagnostics.preparedWorkerLastSupersededReason.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastSupersededReason)
           << "\n";
    detail << "State recall status: "
           << (currentSessionState.transientMetrics.integrationState.empty()
                   ? "not reported"
                   : currentSessionState.transientMetrics.integrationState)
           << "\n";

    if (!currentSessionState.transientMetrics.lastFailure.empty())
        detail << "Last state recall failure: " << currentSessionState.transientMetrics.lastFailure << "\n";

    detail << "\nRuntime diagnostics:\n";
    detail << "Headline: " << (diagnostics.headline.empty() ? "unavailable" : diagnostics.headline) << "\n";
    detail << "Load profile: " << (diagnostics.loadProfileId.empty() ? "unavailable" : diagnostics.loadProfileId)
           << ", articulation: "
           << (diagnostics.selectedArticulationId.empty() ? "unavailable" : diagnostics.selectedArticulationId) << "\n";
    detail << "Cache budget: " << diagnostics.configuredMaxCachedPages
           << " resident pages, max prefetch per voice=" << diagnostics.maxPrefetchBytesPerVoice << " bytes\n";
    detail << "Voices: active=" << diagnostics.activeVoiceCount
           << ", peak=" << diagnostics.peakActiveVoiceCount
           << ", routed zones=" << diagnostics.routedZones.size() << "\n";
    detail << "Stream counters: pageMisses=" << diagnostics.pageMissCount
           << ", cacheHits=" << diagnostics.cacheHitCount
           << ", cacheMisses=" << diagnostics.cacheMissCount
           << ", backgroundReads=" << diagnostics.backgroundReadCount << "\n";
    detail << "Cache residency: resident=" << diagnostics.residentPageCount
           << ", pending=" << diagnostics.pendingPageCount
           << ", purgePasses=" << diagnostics.purgePassCount
           << ", dormantPurges=" << diagnostics.dormantPurgeCount
           << ", cumulativeEvictions=" << diagnostics.evictedPageCount
           << ", lastPurgeEvictions=" << diagnostics.lastPurgeEvictedPageCount << "\n";
    detail << "Read latency micros: average=" << diagnostics.averageReadLatencyMicros
           << ", max=" << diagnostics.maxReadLatencyMicros << "\n";

    if (!diagnostics.failureState.empty())
        detail << "Diagnostics failure state: " << diagnostics.failureState << "\n";

    if (!diagnostics.lastContentProbeCategory.empty())
    {
        detail << "Last content probe: " << diagnostics.lastContentProbeCategory
               << " (" << (diagnostics.lastContentProbeFailedGracefully ? "failed gracefully" : "did not fail gracefully")
               << ")\n";
        detail << "Last content probe state: " << diagnostics.lastContentProbeState << "\n";
    }

    if (!diagnostics.previewFindings.empty())
    {
        detail << "Preview snapshot findings:\n";
        for (const auto& finding : diagnostics.previewFindings)
            detail << "- [" << toString(finding.severity) << "] " << finding.code << ": " << finding.message << "\n";
    }

    if (!diagnostics.publishedFindings.empty())
    {
        detail << "Publish snapshot findings:\n";
        for (const auto& finding : diagnostics.publishedFindings)
            detail << "- [" << toString(finding.severity) << "] " << finding.code << ": " << finding.message << "\n";
    }

    if (!diagnostics.routedZones.empty())
    {
        detail << "Routed zones:\n";
        for (const auto& zone : diagnostics.routedZones)
            detail << "- " << zone << "\n";
    }

    if (!diagnostics.lastContentProbeIssues.empty())
    {
        detail << "Last content probe issues:\n";
        for (const auto& issue : diagnostics.lastContentProbeIssues)
            detail << "- " << issue << "\n";
    }

    if (!diagnostics.issues.empty())
    {
        detail << "Diagnostics issues:\n";
        for (const auto& issue : diagnostics.issues)
            detail << "- " << issue << "\n";
    }

    if (!runtimeManifest.issues.empty())
    {
        detail << "Runtime manifest issues:\n";

        for (const auto& issue : runtimeManifest.issues)
            detail << "- " << issue << "\n";
    }

    if (runtimeManifest.loaded && !runtimeStream.issues.empty())
    {
        detail << "Runtime stream issues:\n";

        for (const auto& issue : runtimeStream.issues)
            detail << "- " << issue << "\n";
    }

    std::vector<std::string> nextSteps;

    if (!hiseProjucerWindowsPresent)
        nextSteps.emplace_back("Decide how Windows developers obtain Projucer, because the vendored HISE tree does not currently include a Windows Projucer binary.");

    if (hiseVendorPresent && hiseNestedJucePresent && linkedFrontend.linked)
        nextSteps.emplace_back("Use the linked frontend-profile and content seam as the hand-off point for the next runtime service, such as processor construction boundaries or preset loading orchestration.");
    else if (hiseVendorPresent && hiseNestedJucePresent)
        nextSteps.emplace_back("Promote the selected HISE plugin frontend profile from a compile-only probe to a minimal linked runtime seam.");

    if (contentSnapshot.repoContentRootExists && contentSnapshot.presetFileCount == 0 && contentSnapshot.sampleMapFileCount == 0)
        nextSteps.emplace_back("Populate content/hise_project/UserPresets and content/hise_project/SampleMaps with the first Decent Rhapsody authoring assets so the adapter can validate real content, not just empty folders.");
    else if (!contentSnapshot.repoContentRootExists)
        nextSteps.emplace_back("Create the product-owned content/hise_project layout so HISE authoring assets have a stable location outside third_party.");

    if (hiseSdkExtracted)
        nextSteps.emplace_back("The bundled HISE SDK inputs are extracted. Validate which parts are still needed versus optional for Decent Rhapsody Studio's Windows workflow.");
    else if (hiseSdkZipPresent)
        nextSteps.emplace_back("Extract third_party/hise/tools/SDK/sdk.zip so HISE's ASIO and VST3 SDK inputs are available.");

    if (hiseProjectTemplatePresent)
        nextSteps.emplace_back("Compare the generated product-owned AppConfig against HISE's frontend export templates to close any remaining macro or include-path gaps.");

    if (!runtimeManifest.manifestFound)
        nextSteps.emplace_back("Create and commit the Phase 1 reference instrument manifest under content/runtime/phase1/reference-corpus so the runtime model has a product-owned load target.");
    else if (!runtimeManifest.loaded)
        nextSteps.emplace_back("Fix the Phase 1 reference instrument manifest issues so the minimal loader can become the hand-off point for the import compiler in Sprint 2.");
    else
        nextSteps.emplace_back("Promote the Phase 1 reference instrument loader from fixture-backed manifest parsing to compiled content emitted by the Sprint 2 import pipeline.");

    nextSteps.emplace_back("Prepare the medium internal streaming case and synthetic stress manifest described by the Phase 1 reference corpus plan before Sprint 3 streaming work begins.");

    if (nextSteps.empty())
        nextSteps.emplace_back("Promote the adapter from metadata probe to a minimal compiled HISE-backed runtime object.");

    const auto integrationState = (hiseVendorPresent && hiseNestedJucePresent && linkedFrontend.linked)
        ? "Plugin frontend profile bridge linked"
        : (hiseVendorPresent && hiseNestedJucePresent)
            ? "Plugin frontend compile probe established"
        : "HISE vendor snapshot incomplete";

    return {
        "HISE vendor handshake",
        diagnostics.hasFailure ? diagnostics.failureState : integrationState,
        diagnostics,
        detail.str(),
        nextSteps
    };
}

RuntimeManifestLoadResult EngineFacade::loadPhase1ReferenceInstrument() const
{
    return referenceInstrumentActive ? referenceManifest : RuntimeManifestLoadResult {};
}

RuntimeStreamLoadResult EngineFacade::loadPhase1ReferenceStream() const
{
    return referenceInstrumentActive ? referenceStream : RuntimeStreamLoadResult {};
}

EngineDiagnosticsSnapshot EngineFacade::getDiagnosticsSnapshot() const
{
    return diagnosticsSnapshot;
}

EnginePerformanceSnapshot EngineFacade::getPerformanceSnapshot() const
{
    EnginePerformanceSnapshot snapshot;
    const auto& draftStatus = draftPlaybackContract.getStatus();
    snapshot.loaded = referenceInstrumentActive && referenceManifest.loaded && referenceStream.loaded
        && draftStatus.performance.available;
    snapshot.draftRevision = draftStatus.draftRevision;
    snapshot.previewRevision = draftStatus.preview.revision;
    snapshot.publishedRevision = draftStatus.performance.revision;
    snapshot.previewBuildId = draftStatus.preview.buildId;
    snapshot.publishedBuildId = draftStatus.performance.buildId;
    snapshot.previewPreparedBuildId = draftStatus.preview.preparedBuildId;
    snapshot.publishedPreparedBuildId = draftStatus.performance.preparedBuildId;
    snapshot.instrumentDisplayName = (referenceInstrumentActive && referenceManifest.loaded)
        ? referenceManifest.instrument.displayName
        : "No instrument loaded";
    snapshot.presetId = referenceInstrumentActive ? currentSessionState.presetId : "none";
    snapshot.loadProfileId = referenceInstrumentActive ? currentSessionState.loadProfileId : "none";
    snapshot.selectedArticulationId = referenceInstrumentActive ? currentSessionState.selectedArticulationId : std::string {};
    snapshot.selectedArticulationName = referenceInstrumentActive
        ? resolveArticulationName(referenceManifest, currentSessionState)
        : std::string {};
    snapshot.previewPending = draftStatus.pendingPreview.active;
    snapshot.publishedPending = draftStatus.pendingPerformance.active;
    snapshot.previewActivationEligible = draftStatus.preview.activationEligible;
    snapshot.publishedActivationEligible = draftStatus.performance.activationEligible;
    snapshot.previewRevisionState = draftStatus.preview.state;
    snapshot.publishedRevisionState = draftStatus.performance.state;
    snapshot.previewContentDigest = draftStatus.preview.contentDigest;
    snapshot.publishedContentDigest = draftStatus.performance.contentDigest;
    snapshot.previewPreparedContentDigest = draftStatus.preview.preparedContentDigest;
    snapshot.publishedPreparedContentDigest = draftStatus.performance.preparedContentDigest;
    snapshot.surfaceStateSource = resolveDraftSurfaceSource(draftStatus);
    snapshot.rendererMode = resolveRendererMode(referenceInstrumentActive);
    if (draftStatus.performance.playableRangeAvailable && draftStatus.performance.available)
    {
        snapshot.playableRangeAvailable = true;
        snapshot.lowestPlayableNote = draftStatus.performance.lowestPlayableNote;
        snapshot.highestPlayableNote = draftStatus.performance.highestPlayableNote;
        snapshot.playableRangeSource = "published";
    }
    else if (draftStatus.preview.playableRangeAvailable && draftStatus.preview.available)
    {
        snapshot.playableRangeAvailable = true;
        snapshot.lowestPlayableNote = draftStatus.preview.lowestPlayableNote;
        snapshot.highestPlayableNote = draftStatus.preview.highestPlayableNote;
        snapshot.playableRangeSource = "preview";
    }
    else
    {
        snapshot.playableRangeSource = "default";
    }
    snapshot.draftPlaybackEvent = draftStatus.lastEvent;
    snapshot.loadIndicator = referenceInstrumentActive
        ? buildLoadIndicator(referenceManifest, referenceStream, currentSessionState)
        : "Click Load Default or Load Lead Demo";
    const auto& workerStatus = preparedPlaybackService.getWorkerStatus();
    snapshot.preparedWorkerPendingCount = workerStatus.pendingWorkCount;
    snapshot.preparedWorkerConfiguredMaxPendingCount = workerStatus.configuredMaxPendingWorkCount;
    snapshot.preparedWorkerConfiguredMaxInFlightCount = workerStatus.configuredMaxInFlightWorkCount;
    snapshot.preparedWorkerCancellationCount = workerStatus.cancellationCount;
    snapshot.preparedWorkerSupersededCount = workerStatus.supersededCount;
    snapshot.preparedWorkerFailureCount = workerStatus.failureCount;
    snapshot.preparedWorkerActiveOwnershipRecordCount = workerStatus.activeOwnershipRecordCount;
    snapshot.preparedWorkerRetiredOwnershipRecordCount = workerStatus.retiredOwnershipRecordCount;
    snapshot.preparedWorkerRetiredBytes = workerStatus.retiredBytesAwaitingCleanup;
    snapshot.preparedWorkerEvent = workerStatus.lastEvent;
    snapshot.preparedWorkerLastCancellationLane = workerStatus.lastCancellationLane;
    snapshot.preparedWorkerLastCancellationReason = workerStatus.lastCancellationReason;
    snapshot.preparedWorkerLastSupersededLane = workerStatus.lastSupersededLane;
    snapshot.preparedWorkerLastSupersededReason = workerStatus.lastSupersededReason;
    snapshot.previewPreparedSampleCount = draftStatus.preview.preparedSampleCount;
    snapshot.previewPreparedStreamCount = draftStatus.preview.preparedStreamCount;
    snapshot.previewPreparedOwnershipRecordCount = draftStatus.preview.preparedOwnershipRecordCount;
    snapshot.publishedPreparedSampleCount = draftStatus.performance.preparedSampleCount;
    snapshot.publishedPreparedStreamCount = draftStatus.performance.preparedStreamCount;
    snapshot.publishedPreparedOwnershipRecordCount = draftStatus.performance.preparedOwnershipRecordCount;
    snapshot.previewPreparedBytes = draftStatus.preview.preparedBytes;
    snapshot.publishedPreparedBytes = draftStatus.performance.preparedBytes;
    snapshot.previewPreparedOwnershipBytes = draftStatus.preview.preparedOwnershipBytes;
    snapshot.publishedPreparedOwnershipBytes = draftStatus.performance.preparedOwnershipBytes;
    snapshot.previewPreparationCacheHits = draftStatus.preview.preparationCacheHitCount;
    snapshot.previewPreparationCacheMisses = draftStatus.preview.preparationCacheMissCount;
    snapshot.publishedPreparationCacheHits = draftStatus.performance.preparationCacheHitCount;
    snapshot.publishedPreparationCacheMisses = draftStatus.performance.preparationCacheMissCount;
    snapshot.previewFindings = draftStatus.preview.findings;
    snapshot.publishedFindings = draftStatus.performance.findings;

    if (referenceInstrumentActive)
    {
        snapshot.previewPlayback = previewPlaybackSnapshot;
        snapshot.previewPlayback.ready = snapshot.loaded;
        snapshot.previewPlayback.appliedMacroSummary = buildAppliedMacroSummary(currentSessionState);
        if (snapshot.previewPlayback.state.empty())
            snapshot.previewPlayback.state = snapshot.loaded ? "Ready to audition" : snapshot.loadIndicator;
    }
    else
    {
        snapshot.previewPlayback.state = snapshot.loadIndicator;
        snapshot.previewPlayback.appliedMacroSummary = "No instrument loaded";
    }

    return snapshot;
}

std::vector<EngineArticulationDescriptor> EngineFacade::getArticulationDescriptors() const
{
    std::vector<EngineArticulationDescriptor> descriptors;

    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return descriptors;

    descriptors.reserve(referenceManifest.instrument.articulations.size());
    for (const auto& articulation : referenceManifest.instrument.articulations)
    {
        descriptors.push_back({
            articulation.id,
            articulation.name,
            articulation.isDefault,
            articulation.id == currentSessionState.selectedArticulationId
        });
    }

    return descriptors;
}

std::vector<EngineMacroDescriptor> EngineFacade::getMacroDescriptors() const
{
    std::vector<EngineMacroDescriptor> descriptors;

    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return descriptors;

    descriptors.reserve(referenceManifest.instrument.macros.size());

    for (const auto& macro : referenceManifest.instrument.macros)
    {
        auto currentValue = macro.defaultValue;
        const auto currentIterator = std::find_if(currentSessionState.macroValues.begin(),
                                                  currentSessionState.macroValues.end(),
                                                  [&](const RuntimePresetMacroValue& value)
                                                  {
                                                      return value.id == macro.id;
                                                  });
        if (currentIterator != currentSessionState.macroValues.end())
            currentValue = currentIterator->value;

        descriptors.push_back({
            macro.id,
            macro.name,
            macro.minValue,
            macro.maxValue,
            macro.defaultValue,
            currentValue,
            macro.id == "tone" ? "preview.triggerVelocity" : "preview.noteTravel",
            macro.id == "tone"
                ? "Biases the reference preview from softer attacks into accent territory."
                : "Offsets the previewed note pitch to add movement across the reference range.",
            macro.id == "tone" ? buildToneCurrentEffect(currentSessionState) : buildMotionCurrentEffect(currentSessionState)
        });
    }

    return descriptors;
}

bool EngineFacade::setSelectedArticulation(const std::string& articulationId)
{
    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return false;

    if (findArticulationDefinition(referenceManifest.instrument, articulationId) == nullptr)
        return false;

    if (currentSessionState.selectedArticulationId == articulationId)
        return true;

    currentSessionState.selectedArticulationId = articulationId;
    currentSessionState.transientMetrics.integrationState = "Performance surface articulation updated";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    previewPlaybackSnapshot.articulationId = articulationId;
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    markStateChanged();
    return true;
}

bool EngineFacade::setMacroValue(const std::string& macroId, double value)
{
    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return false;

    const auto definitionIterator = std::find_if(referenceManifest.instrument.macros.begin(),
                                                 referenceManifest.instrument.macros.end(),
                                                 [&](const RuntimeMacroDefinition& macro)
                                                 {
                                                     return macro.id == macroId;
                                                 });
    if (definitionIterator == referenceManifest.instrument.macros.end())
        return false;

    const auto clampedValue = normalizeMacroValue(std::clamp(value, definitionIterator->minValue, definitionIterator->maxValue));
    const auto currentIterator = std::find_if(currentSessionState.macroValues.begin(),
                                              currentSessionState.macroValues.end(),
                                              [&](const RuntimePresetMacroValue& currentValue)
                                              {
                                                  return currentValue.id == macroId;
                                              });

    if (currentIterator != currentSessionState.macroValues.end())
    {
        if (currentIterator->value == clampedValue)
            return true;

        currentIterator->value = clampedValue;
    }
    else
    {
        currentSessionState.macroValues.push_back({ macroId, clampedValue });
    }

    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    markStateChanged();
    return true;
}

bool EngineFacade::stageDraftRevision(std::size_t revision)
{
    pumpPreparedPlaybackWorkerCompletions();

    if (!referenceInstrumentActive || !draftPlaybackContract.getStatus().projectOpen)
        return false;

    if (!draftPlaybackContract.setDraftRevision(revision))
        return false;

    currentSessionState.transientMetrics.integrationState = "Draft revision staged";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::refreshPreviewToCurrentDraft()
{
    pumpPreparedPlaybackWorkerCompletions();

    if (!referenceInstrumentActive || !referenceManifest.loaded || !referenceStream.loaded || !authoringProject.loaded)
        return false;

    const auto request = draftPlaybackContract.requestPreviewBuild();
    if (!request.accepted)
        return false;

    const auto buildResult = buildCurrentPlaybackSnapshot(false);
    if (!buildResult.built || !buildResult.activationEligible)
    {
        const auto preparedResult = buildRejectedPreparedPlayback(buildResult);
        const auto applied = draftPlaybackContract.completePreviewBuild(request.requestId, buildResult, preparedResult);
        if (!applied)
            return false;

        currentSessionState.transientMetrics.integrationState = "Preview revision failed";
        currentSessionState.transientMetrics.lastFailure = summarizeSnapshotFindings(draftPlaybackContract.getStatus().preview.findings);
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    if (!enqueuePreparedPlaybackBuild(request.requestId, buildResult, PreparedPlaybackWorkLane::preview))
    {
        PreparedPlaybackBuildResult queueRejected;
        queueRejected.snapshotBuildId = buildResult.buildId;
        queueRejected.requestedDraftRevision = buildResult.requestedDraftRevision;
        queueRejected.activationRequested = buildResult.activationRequested;
        queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        queueRejected.state = "Prepared playback queue is full";
        queueRejected.metrics.failureCount = 1;
        queueRejected.findings.push_back({
            PlaybackSnapshotFindingSeverity::error,
            "prepared-queue-full",
            "preparedWorker",
            "Prepared playback queue is full."
        });
        draftPlaybackContract.completePreviewBuild(request.requestId, buildResult, queueRejected);
        currentSessionState.transientMetrics.integrationState = "Preview revision failed";
        currentSessionState.transientMetrics.lastFailure = "Prepared playback queue is full.";
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    currentSessionState.transientMetrics.integrationState = "Preview revision preparing";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::publishCurrentDraft()
{
    pumpPreparedPlaybackWorkerCompletions();

    if (!referenceInstrumentActive || !referenceManifest.loaded || !referenceStream.loaded || !authoringProject.loaded)
        return false;

    const auto request = draftPlaybackContract.requestPerformanceBuild();
    if (!request.accepted)
        return false;

    const auto buildResult = buildCurrentPlaybackSnapshot(true);
    if (!buildResult.built || !buildResult.activationEligible)
    {
        const auto preparedResult = buildRejectedPreparedPlayback(buildResult);
        const auto applied = draftPlaybackContract.completePerformanceBuild(request.requestId, buildResult, preparedResult);
        if (!applied)
            return false;

        currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
        currentSessionState.transientMetrics.lastFailure = summarizeSnapshotFindings(draftPlaybackContract.getStatus().performance.findings);
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    if (!enqueuePreparedPlaybackBuild(request.requestId, buildResult, PreparedPlaybackWorkLane::performance))
    {
        PreparedPlaybackBuildResult queueRejected;
        queueRejected.snapshotBuildId = buildResult.buildId;
        queueRejected.requestedDraftRevision = buildResult.requestedDraftRevision;
        queueRejected.activationRequested = buildResult.activationRequested;
        queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        queueRejected.state = "Prepared playback queue is full";
        queueRejected.metrics.failureCount = 1;
        queueRejected.findings.push_back({
            PlaybackSnapshotFindingSeverity::error,
            "prepared-queue-full",
            "preparedWorker",
            "Prepared playback queue is full."
        });
        draftPlaybackContract.completePerformanceBuild(request.requestId, buildResult, queueRejected);
        currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
        currentSessionState.transientMetrics.lastFailure = "Prepared playback queue is full.";
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    currentSessionState.transientMetrics.integrationState = "Publish preparation queued";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

void EngineFacade::closeDraftPlaybackProject()
{
    clearPendingPreparedCompletions();
    draftPlaybackContract.closeProject();
    referenceInstrumentActive = false;
    currentSessionState.transientMetrics.integrationState = "Draft playback project closed";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

bool EngineFacade::reopenDraftPlaybackProject(std::size_t revision)
{
    clearPendingPreparedCompletions();
    if (!referenceManifest.loaded)
        return false;

    draftPlaybackContract.reopenProject(revision);
    referenceInstrumentActive = true;
    currentSessionState.transientMetrics.integrationState = "Draft playback project reopened";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::replaceDraftPlaybackAuthoringProject(RuntimeProjectModel project)
{
    const auto validation = validateRuntimeProjectModel(project);
    if (!validation.valid)
        return false;

    clearPendingPreparedCompletions();
    authoringProject = {};
    authoringProject.manifestFound = true;
    authoringProject.loaded = true;
    authoringProject.state = "Draft playback authoring project replaced";
    authoringProject.project = std::move(project);
    currentSessionState.transientMetrics.integrationState = "Draft playback authoring project replaced";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

PlaybackSnapshotBuildResult EngineFacade::buildCurrentPlaybackSnapshot(bool activationRequested)
{
    if (!authoringProject.loaded)
    {
        PlaybackSnapshotBuildResult result;
        result.state = "Authoring project unavailable";
        result.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        result.findings.push_back({
            PlaybackSnapshotFindingSeverity::error,
            "missing-authoring-project",
            "authoringProject",
            "Phase 2 authoring reference project is unavailable for snapshot construction."
        });
        return result;
    }

    const auto request = playbackSnapshotBuilder.requestBuild(draftPlaybackContract.getStatus().draftRevision,
                                                              activationRequested);
    return playbackSnapshotBuilder.buildSnapshot(request, authoringProject.project);
}

PreparedPlaybackBuildResult EngineFacade::buildRejectedPreparedPlayback(const PlaybackSnapshotBuildResult& snapshotResult)
{
    const auto request = preparedPlaybackService.requestBuild(snapshotResult, referenceStream);
    return preparedPlaybackService.prepare(request, snapshotResult, referenceStream);
}

bool EngineFacade::enqueuePreparedPlaybackBuild(std::uint64_t contractRequestId,
                                                const PlaybackSnapshotBuildResult& snapshotResult,
                                                PreparedPlaybackWorkLane lane)
{
    auto submitResult = lane == PreparedPlaybackWorkLane::performance
        ? preparedPlaybackService.enqueuePublishBuild(snapshotResult)
        : preparedPlaybackService.enqueuePreviewBuild(snapshotResult);

    for (const auto& displacedResult : submitResult.displacedResults)
        pendingPreparedCompletions.erase(displacedResult.buildId);

    if (!submitResult.accepted)
        return false;

    if (lane == PreparedPlaybackWorkLane::preview)
        discardSupersededPreviewPendingPreparedCompletions(submitResult.request.buildId);

    pendingPreparedCompletions[submitResult.request.buildId] = {
        lane,
        contractRequestId,
        snapshotResult
    };
    return true;
}

bool EngineFacade::pumpPreparedPlaybackWorkerCompletions()
{
    auto completedResults = preparedPlaybackService.drainCompletedBuilds();
    bool appliedCompletion = false;

    for (const auto& stepResult : completedResults)
    {
        const auto pendingIterator = pendingPreparedCompletions.find(stepResult.result.buildId);
        if (pendingIterator == pendingPreparedCompletions.end())
            continue;

        const auto pendingCompletion = pendingIterator->second;
        bool applied = false;

        if (pendingCompletion.lane == PreparedPlaybackWorkLane::preview)
        {
            applied = draftPlaybackContract.completePreviewBuild(
                pendingCompletion.contractRequestId,
                pendingCompletion.snapshotResult,
                stepResult.result);

            if (applied)
            {
                if (stepResult.result.built && stepResult.result.activationEligible)
                {
                    currentSessionState.transientMetrics.integrationState = "Preview revision prepared";
                    currentSessionState.transientMetrics.lastFailure.clear();
                }
                else
                {
                    currentSessionState.transientMetrics.integrationState = "Preview revision failed";
                    currentSessionState.transientMetrics.lastFailure =
                        summarizeSnapshotFindings(draftPlaybackContract.getStatus().preview.findings);
                }
            }
        }
        else
        {
            applied = draftPlaybackContract.completePerformanceBuild(
                pendingCompletion.contractRequestId,
                pendingCompletion.snapshotResult,
                stepResult.result);

            if (applied)
            {
                if (stepResult.result.built && stepResult.result.activationEligible)
                {
                    currentSessionState.transientMetrics.integrationState = "Published revision activated";
                    currentSessionState.transientMetrics.lastFailure.clear();
                }
                else
                {
                    currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
                    currentSessionState.transientMetrics.lastFailure =
                        summarizeSnapshotFindings(draftPlaybackContract.getStatus().performance.findings);
                }
            }
        }

        pendingPreparedCompletions.erase(pendingIterator);
        appliedCompletion = appliedCompletion || applied;
    }

    if (appliedCompletion)
    {
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
    }

    return appliedCompletion;
}

void EngineFacade::markStateChanged()
{
    ++stateRevision;
}

void EngineFacade::clearPendingPreparedCompletions()
{
    const auto canceledPreview = preparedPlaybackService.cancelQueuedPreviewBuilds(
        "Prepared playback preview build canceled before worker execution");
    const auto canceledPerformance = preparedPlaybackService.cancelQueuedPublishBuilds(
        "Prepared playback publish build canceled before worker execution");

    for (const auto& result : canceledPreview)
        pendingPreparedCompletions.erase(result.buildId);

    for (const auto& result : canceledPerformance)
        pendingPreparedCompletions.erase(result.buildId);

    pendingPreparedCompletions.clear();
    preparedPlaybackService.drainCompletedBuilds();
}

void EngineFacade::discardSupersededPreviewPendingPreparedCompletions(const std::uint64_t newestBuildId)
{
    for (auto iterator = pendingPreparedCompletions.begin(); iterator != pendingPreparedCompletions.end();)
    {
        if (iterator->second.lane == PreparedPlaybackWorkLane::preview
            && iterator->first != newestBuildId)
        {
            iterator = pendingPreparedCompletions.erase(iterator);
            continue;
        }

        ++iterator;
    }
}

bool EngineFacade::beginDraftPlaybackDeviceRestart()
{
    clearPendingPreparedCompletions();
    if (!draftPlaybackContract.beginDeviceRestart())
        return false;

    currentSessionState.transientMetrics.integrationState = "Device restart in progress";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::completeDraftPlaybackDeviceRestart(bool restored)
{
    pumpPreparedPlaybackWorkerCompletions();
    if (!draftPlaybackContract.completeDeviceRestart(restored))
        return false;

    currentSessionState.transientMetrics.integrationState = restored
        ? "Device restart recovered"
        : "Device restart failed";
    if (!restored)
        currentSessionState.transientMetrics.lastFailure = "Device restart failed to restore the published revision.";
    else
        currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::waitForPreparedPlaybackIdle(std::chrono::milliseconds timeout)
{
    const auto deadline = Clock::now() + timeout;

    while (Clock::now() <= deadline)
    {
        serviceBackgroundWork();
        if (pendingPreparedCompletions.empty()
            && !draftPlaybackContract.getStatus().pendingPreview.active
            && !draftPlaybackContract.getStatus().pendingPerformance.active
            && preparedPlaybackService.getWorkerStatus().pendingWorkCount == 0
            && preparedPlaybackService.getWorkerStatus().inFlightWorkCount == 0)
        {
            return true;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
        if (remaining.count() > 0)
            preparedPlaybackService.waitForWorkerIdle(remaining.count() > 25 ? 25 : static_cast<std::uint64_t>(remaining.count()));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    serviceBackgroundWork();
    return pendingPreparedCompletions.empty()
        && !draftPlaybackContract.getStatus().pendingPreview.active
        && !draftPlaybackContract.getStatus().pendingPerformance.active
        && preparedPlaybackService.getWorkerStatus().pendingWorkCount == 0
        && preparedPlaybackService.getWorkerStatus().inFlightWorkCount == 0;
}

EnginePreviewPlaybackSnapshot EngineFacade::auditionPreviewNote(int midiNote, int velocity)
{
    pumpPreparedPlaybackWorkerCompletions();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    previewPlaybackSnapshot.midiNote = midiNote;
    previewPlaybackSnapshot.velocity = velocity;
    previewPlaybackSnapshot.effectiveMidiNote = midiNote;
    previewPlaybackSnapshot.effectiveVelocity = velocity;
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    previewPlaybackSnapshot.appliedMacroSummary = buildAppliedMacroSummary(currentSessionState);

    if (!referenceInstrumentActive)
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = "No instrument is loaded. Use Load Default or Load Lead Demo first.";
        return previewPlaybackSnapshot;
    }

    if (!referenceManifest.loaded)
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = referenceManifest.state;
        return previewPlaybackSnapshot;
    }

    if (!referenceStream.loaded)
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = referenceStream.state;
        return previewPlaybackSnapshot;
    }

    if ((!draftPlaybackContract.getStatus().preview.available
         || draftPlaybackContract.getStatus().preview.revision != draftPlaybackContract.getStatus().draftRevision)
        && !refreshPreviewToCurrentDraft())
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = "Preview revision could not be prepared for the current draft.";
        syncPreviewSnapshotFromDraftPlayback();
        return previewPlaybackSnapshot;
    }

    syncPreviewSnapshotFromDraftPlayback();
    const auto loadProfile = findPhase1RuntimeLoadProfile(currentSessionState.loadProfileId);
    if (!loadProfile.has_value())
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = "Unknown load profile '" + currentSessionState.loadProfileId + "'.";
        return previewPlaybackSnapshot;
    }

    previewPlaybackSnapshot.ready = true;
    previewPlaybackSnapshot.attempted = true;

    const auto effectiveMidiNote = computeMotionPreviewNote(currentSessionState, midiNote);
    const auto effectiveVelocity = computeTonePreviewVelocity(currentSessionState, velocity);
    previewPlaybackSnapshot.effectiveMidiNote = effectiveMidiNote;
    previewPlaybackSnapshot.effectiveVelocity = effectiveVelocity;

    RuntimeStreamingService service(
        referenceStream.container,
        buildRuntimeStreamingServiceOptions(*loadProfile, 2500));
    RuntimeVoice previewVoice;
    std::string errorMessage;

    const auto allocated = previewVoice.allocate(referenceManifest.instrument,
                                                 referenceStream.container,
                                                 {
                                                     nextPreviewVoiceId++,
                                                     "",
                                                     effectiveMidiNote,
                                                     effectiveVelocity,
                                                     toVoiceMacroValues(currentSessionState),
                                                     currentSessionState.selectedArticulationId
                                                 },
                                                 errorMessage);
    if (!allocated)
    {
        previewPlaybackSnapshot.state = "Preview allocation failed";
        previewPlaybackSnapshot.errorMessage = errorMessage;
        return previewPlaybackSnapshot;
    }

    previewVoice.advanceFrames(4096, service);
    const auto boundaryAdvance = previewVoice.advanceFrames(64, service);
    previewPlaybackSnapshot.waitedForPage = boundaryAdvance.waitingForPage;

    if (previewPlaybackSnapshot.waitedForPage)
    {
        const auto pageReady = waitUntil(
            [&]
            {
                const auto snapshot = previewVoice.getSnapshot();
                return !snapshot.sampleId.empty()
                    && service.isPageReady({ snapshot.sampleId, 0 });
            },
            std::chrono::milliseconds(500));

        if (pageReady)
        {
            const auto resumedAdvance = previewVoice.advanceFrames(64, service);
            previewPlaybackSnapshot.acquiredPageLease = resumedAdvance.acquiredPageLease;
        }
    }

    previewPlaybackSnapshot.zoneId = previewVoice.getSnapshot().zoneId;
    previewVoice.beginRelease();
    previewPlaybackSnapshot.voiceFinished = waitUntil(
        [&]
        {
            const auto advance = previewVoice.advanceFrames(8192, service);
            return advance.voiceFinished
                || previewVoice.getSnapshot().state == RuntimeVoiceLifecycleState::finished;
        },
        std::chrono::milliseconds(1500));
    previewPlaybackSnapshot.succeeded = previewPlaybackSnapshot.voiceFinished;
    previewPlaybackSnapshot.state = previewPlaybackSnapshot.succeeded
        ? "Preview played"
        : "Preview did not finish cleanly";
    syncPreviewSnapshotFromDraftPlayback();
    markStateChanged();

    return previewPlaybackSnapshot;
}

std::string EngineFacade::exportPresetStateJson() const
{
    return serializeRuntimePresetState(captureRuntimePresetState(currentSessionState));
}

EnginePresetStateRestoreResult EngineFacade::restorePresetStateJson(const std::string& presetStateJson)
{
    EnginePresetStateRestoreResult restoreResult;
    restoreResult.state = "Preset state restore failed";

    const auto parsedState = parseRuntimePresetState(presetStateJson);
    if (!parsedState.loaded)
    {
        restoreResult.issues = parsedState.issues;
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = summarizeIssues(parsedState.issues);
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    if (!referenceManifest.loaded)
    {
        restoreResult.issues.push_back("Reference instrument manifest is unavailable, so preset state cannot be restored.");
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = restoreResult.issues.front();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    const auto validation = validateRuntimePresetState(parsedState.preset, referenceManifest.instrument);
    if (!validation.valid)
    {
        restoreResult.issues = validation.issues;
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = summarizeIssues(validation.issues);
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    currentSessionState.presetId = parsedState.preset.presetId;
    currentSessionState.targetInstrumentId = parsedState.preset.targetInstrumentId;
    currentSessionState.targetInstrumentSchemaName = parsedState.preset.targetInstrumentSchemaName;
    currentSessionState.targetInstrumentSchemaVersion = parsedState.preset.targetInstrumentSchemaVersion;
    currentSessionState.loadProfileId = parsedState.preset.loadProfileId;
    currentSessionState.selectedArticulationId = parsedState.preset.selectedArticulationId;
    currentSessionState.macroValues = parsedState.preset.macroValues;
    currentSessionState.notes = parsedState.preset.notes;
    currentSessionState.transientMetrics.integrationState = "Preset state restored";
    currentSessionState.transientMetrics.lastFailure.clear();
    referenceInstrumentActive = true;
    previewPlaybackSnapshot = {};
    initializeDraftPlaybackContract(true);
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    refreshDiagnosticsSnapshot();
    markStateChanged();

    restoreResult.restored = true;
    restoreResult.state = "Preset state restored";
    return restoreResult;
}

EnginePresetStateRestoreResult EngineFacade::restorePresetStateFile(const std::string& presetStatePath)
{
    return restorePresetStateJson(readTextFile(presetStatePath));
}

EngineContentFailureProbeResult EngineFacade::probeContentFailure(const EngineContentFailureCategory category)
{
    EngineContentFailureProbeResult probeResult;
    probeResult.attempted = true;
    probeResult.categoryId = getFailureCategoryId(category);
    probeResult.state = "Content failure probe did not run";

    switch (category)
    {
    case EngineContentFailureCategory::missingContent:
    case EngineContentFailureCategory::schemaMismatch:
    case EngineContentFailureCategory::partialCompiledArtifact:
    {
        const auto manifestResult = loadRuntimeInstrumentManifest(getFailureFixturePath(category).generic_string());
        probeResult.failedGracefully = !manifestResult.loaded && !manifestResult.issues.empty();
        probeResult.state = manifestResult.state;
        probeResult.issues = manifestResult.issues;
        break;
    }
    case EngineContentFailureCategory::badChecksum:
    {
        const auto corruptStreamPath = buildChecksumMismatchFixture();
        const auto streamResult = loadRuntimeStreamContainer(corruptStreamPath.generic_string());
        probeResult.failedGracefully = !streamResult.loaded && !streamResult.issues.empty();
        probeResult.state = streamResult.state;
        probeResult.issues = streamResult.issues;
        break;
    }
    }

    lastContentFailureProbe = probeResult;
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return probeResult;
}

void EngineFacade::clearContentFailureProbe()
{
    lastContentFailureProbe = {};
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

void EngineFacade::resetSessionStateToDefault()
{
    if (!referenceManifest.loaded)
    {
        referenceInstrumentActive = false;
        currentSessionState = {};
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
        lastContentFailureProbe = {};
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return;
    }

    referenceInstrumentActive = true;
    currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
    currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    initializeDraftPlaybackContract(true);
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    lastContentFailureProbe = {};
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

void EngineFacade::syncPreviewSnapshotFromDraftPlayback()
{
    const auto& draftStatus = draftPlaybackContract.getStatus();
    previewPlaybackSnapshot.ready = draftStatus.preview.available;
    previewPlaybackSnapshot.draftRevision = draftStatus.draftRevision;
    previewPlaybackSnapshot.preparedRevision = draftStatus.preview.revision;
    previewPlaybackSnapshot.pendingBuild = draftStatus.pendingPreview.active;
    previewPlaybackSnapshot.revisionState = draftStatus.preview.state;

    if (previewPlaybackSnapshot.articulationId.empty())
        previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;

    if (previewPlaybackSnapshot.state.empty())
    {
        previewPlaybackSnapshot.state = draftStatus.preview.state.empty()
            ? buildLoadIndicator(referenceManifest, referenceStream, currentSessionState)
            : draftStatus.preview.state;
    }

    if (previewPlaybackSnapshot.errorMessage.empty() && !draftStatus.preview.findings.empty())
        previewPlaybackSnapshot.errorMessage = summarizeSnapshotFindings(draftStatus.preview.findings);
}

void EngineFacade::initializeDraftPlaybackContract(bool activatePerformanceRevision)
{
    clearPendingPreparedCompletions();
    preparedPlaybackService.setBackgroundWorkerStream(referenceStream);
    draftPlaybackContract.reopenProject(0);

    if (!referenceManifest.loaded || !referenceStream.loaded)
    {
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        return;
    }

    if (authoringProject.loaded)
    {
        if (const auto previewRequest = draftPlaybackContract.requestPreviewBuild(); previewRequest.accepted)
        {
            const auto previewBuild = buildCurrentPlaybackSnapshot(false);
            if (!previewBuild.built || !previewBuild.activationEligible)
            {
                const auto preparedPreview = buildRejectedPreparedPlayback(previewBuild);
                draftPlaybackContract.completePreviewBuild(previewRequest.requestId, previewBuild, preparedPreview);
            }
            else if (!enqueuePreparedPlaybackBuild(previewRequest.requestId,
                                                   previewBuild,
                                                   PreparedPlaybackWorkLane::preview))
            {
                PreparedPlaybackBuildResult queueRejected;
                queueRejected.snapshotBuildId = previewBuild.buildId;
                queueRejected.requestedDraftRevision = previewBuild.requestedDraftRevision;
                queueRejected.activationRequested = previewBuild.activationRequested;
                queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
                queueRejected.state = "Prepared playback queue is full";
                queueRejected.metrics.failureCount = 1;
                queueRejected.findings.push_back({
                    PlaybackSnapshotFindingSeverity::error,
                    "prepared-queue-full",
                    "preparedWorker",
                    "Prepared playback queue is full."
                });
                draftPlaybackContract.completePreviewBuild(previewRequest.requestId, previewBuild, queueRejected);
            }
        }

        if (activatePerformanceRevision)
        {
            if (const auto publishRequest = draftPlaybackContract.requestPerformanceBuild(); publishRequest.accepted)
            {
                const auto publishBuild = buildCurrentPlaybackSnapshot(true);
                if (!publishBuild.built || !publishBuild.activationEligible)
                {
                    const auto preparedPublish = buildRejectedPreparedPlayback(publishBuild);
                    draftPlaybackContract.completePerformanceBuild(publishRequest.requestId, publishBuild, preparedPublish);
                }
                else if (!enqueuePreparedPlaybackBuild(publishRequest.requestId,
                                                       publishBuild,
                                                       PreparedPlaybackWorkLane::performance))
                {
                    PreparedPlaybackBuildResult queueRejected;
                    queueRejected.snapshotBuildId = publishBuild.buildId;
                    queueRejected.requestedDraftRevision = publishBuild.requestedDraftRevision;
                    queueRejected.activationRequested = publishBuild.activationRequested;
                    queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
                    queueRejected.state = "Prepared playback queue is full";
                    queueRejected.metrics.failureCount = 1;
                    queueRejected.findings.push_back({
                        PlaybackSnapshotFindingSeverity::error,
                        "prepared-queue-full",
                        "preparedWorker",
                        "Prepared playback queue is full."
                    });
                    draftPlaybackContract.completePerformanceBuild(publishRequest.requestId, publishBuild, queueRejected);
                }
            }
        }

        waitForPreparedPlaybackIdle(std::chrono::milliseconds(1000));
    }
    else
    {
        if (const auto previewRequest = draftPlaybackContract.requestPreviewBuild(); previewRequest.accepted)
            draftPlaybackContract.completePreviewBuild(previewRequest.requestId);

        if (activatePerformanceRevision)
        {
            if (const auto publishRequest = draftPlaybackContract.requestPerformanceBuild(); publishRequest.accepted)
                draftPlaybackContract.completePerformanceBuild(publishRequest.requestId);
        }
    }

    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
}

void EngineFacade::refreshDiagnosticsSnapshot()
{
    diagnosticsSnapshot = {};
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    syncDraftPlaybackIntoDiagnostics(draftPlaybackContract.getStatus(), diagnosticsSnapshot);
    syncPreparedPlaybackWorkerIntoDiagnostics(preparedPlaybackService.getWorkerStatus(), diagnosticsSnapshot);
    diagnosticsSnapshot.rendererMode = resolveRendererMode(referenceInstrumentActive);

    if (!referenceInstrumentActive)
    {
        diagnosticsSnapshot.headline = "No instrument loaded";
        diagnosticsSnapshot.failureState = "Reference-backed renderer is available but inactive.";
        return;
    }

    if (!referenceManifest.loaded)
    {
        diagnosticsSnapshot.headline = "Reference manifest unavailable";
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = referenceManifest.state;
        diagnosticsSnapshot.issues = referenceManifest.issues;
        return;
    }

    if (!referenceStream.loaded)
    {
        diagnosticsSnapshot.headline = "Reference stream unavailable";
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = referenceStream.state;
        diagnosticsSnapshot.issues = referenceStream.issues;
        return;
    }

    const auto loadProfile = findPhase1RuntimeLoadProfile(currentSessionState.loadProfileId);
    if (!loadProfile.has_value())
    {
        diagnosticsSnapshot.headline = "Diagnostics unavailable";
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = "Unknown current load profile '" + currentSessionState.loadProfileId + "'.";
        diagnosticsSnapshot.issues.push_back(diagnosticsSnapshot.failureState);
        return;
    }

    diagnosticsSnapshot.available = true;
    diagnosticsSnapshot.headline = "Reference-backed runtime diagnostics ready";

    for (const auto& finding : draftPlaybackContract.getStatus().preview.findings)
        diagnosticsSnapshot.issues.push_back("Preview snapshot: " + finding.message);

    for (const auto& finding : draftPlaybackContract.getStatus().performance.findings)
        diagnosticsSnapshot.issues.push_back("Publish snapshot: " + finding.message);
    diagnosticsSnapshot.configuredMaxCachedPages = loadProfile->maxCachedPages;
    diagnosticsSnapshot.maxPrefetchBytesPerVoice = loadProfile->maxPrefetchBytesPerVoice;

    try
    {
        RuntimeStreamingService service(
            referenceStream.container,
            buildRuntimeStreamingServiceOptions(*loadProfile, 1500));

        const auto macroValues = toVoiceMacroValues(currentSessionState);
        RuntimeVoice sustainVoiceA;
        RuntimeVoice sustainVoiceB;
        RuntimeVoice leadVoice;
        std::string errorMessage;

        if (!sustainVoiceA.allocate(referenceManifest.instrument,
                                    referenceStream.container,
                                    { 2101, "", 57, 64, macroValues, "" },
                                    errorMessage))
        {
            throw std::runtime_error("Diagnostics sustain voice A failed to allocate: " + errorMessage);
        }

        if (!sustainVoiceB.allocate(referenceManifest.instrument,
                                    referenceStream.container,
                                    { 2102, "", 57, 120, macroValues, "" },
                                    errorMessage))
        {
            throw std::runtime_error("Diagnostics sustain voice B failed to allocate: " + errorMessage);
        }

        if (!leadVoice.allocate(referenceManifest.instrument,
                                referenceStream.container,
                                { 2103, "", 69, 120, macroValues, "lead" },
                                errorMessage))
        {
            throw std::runtime_error("Diagnostics lead voice failed to allocate: " + errorMessage);
        }

        diagnosticsSnapshot.routedZones.push_back(sustainVoiceA.getSnapshot().zoneId);
        diagnosticsSnapshot.routedZones.push_back(sustainVoiceB.getSnapshot().zoneId);
        diagnosticsSnapshot.routedZones.push_back(leadVoice.getSnapshot().zoneId);

        sustainVoiceA.advanceFrames(4096, service);
        sustainVoiceB.advanceFrames(4096, service);
        leadVoice.advanceFrames(2048, service);
        sustainVoiceA.advanceFrames(64, service);
        sustainVoiceB.advanceFrames(64, service);
        leadVoice.advanceFrames(64, service);

        const auto firstPagesReady = waitUntil(
            [&]
            {
                return service.isPageReady({ "sine-a3", 0 })
                    && service.isPageReady({ "triangle-a4", 0 });
            },
            std::chrono::milliseconds(500));

        if (!firstPagesReady)
            throw std::runtime_error("Diagnostics service did not make the first streamed pages ready in time.");

        sustainVoiceA.advanceFrames(64, service);
        sustainVoiceB.advanceFrames(64, service);
        leadVoice.advanceFrames(64, service);

        std::vector<RuntimeStreamPageRequest> followUpRequests;
        for (const auto& sample : referenceStream.container.samples)
        {
            for (const auto& page : sample.pages)
            {
                if (page.pageIndex == 0)
                    continue;

                followUpRequests.push_back({ sample.sampleId, page.pageIndex });
            }
        }

        for (const auto& request : followUpRequests)
            service.enqueuePageRead(request);

        const auto expectedBackgroundReads = static_cast<std::size_t>(2 + followUpRequests.size());
        const auto queuedReadsReady = waitUntil(
            [&]
            {
                return service.getMetrics().backgroundReadCount >= expectedBackgroundReads;
            },
            std::chrono::milliseconds(1000));

        if (!queuedReadsReady)
            throw std::runtime_error("Diagnostics service did not finish the queued background reads in time.");

        sustainVoiceA.beginRelease();
        sustainVoiceB.beginRelease();
        leadVoice.beginRelease();
        drainVoice(sustainVoiceA, service);
        drainVoice(sustainVoiceB, service);
        drainVoice(leadVoice, service);
        service.purgeDormantPages();

        const auto metrics = service.getMetrics();
        diagnosticsSnapshot.cacheHitCount = metrics.cacheHitCount;
        diagnosticsSnapshot.cacheMissCount = metrics.cacheMissCount;
        diagnosticsSnapshot.pageMissCount = metrics.pageMissCount;
        diagnosticsSnapshot.backgroundReadCount = metrics.backgroundReadCount;
        diagnosticsSnapshot.residentPageCount = metrics.residentPageCount;
        diagnosticsSnapshot.pendingPageCount = metrics.pendingPageCount;
        diagnosticsSnapshot.activeVoiceCount = metrics.activeVoiceCount;
        diagnosticsSnapshot.peakActiveVoiceCount = metrics.peakActiveVoiceCount;
        diagnosticsSnapshot.purgePassCount = metrics.purgePassCount;
        diagnosticsSnapshot.dormantPurgeCount = metrics.dormantPurgeCount;
        diagnosticsSnapshot.evictedPageCount = metrics.evictedPageCount;
        diagnosticsSnapshot.lastPurgeEvictedPageCount = metrics.lastPurgeEvictedPageCount;
        diagnosticsSnapshot.averageReadLatencyMicros = metrics.averageReadLatencyMicros;
        diagnosticsSnapshot.maxReadLatencyMicros = metrics.maxReadLatencyMicros;
        diagnosticsSnapshot.headFramesRead = metrics.headFramesRead;
        diagnosticsSnapshot.headBytesRead = metrics.headBytesRead;
    }
    catch (const std::exception& exception)
    {
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = exception.what();
        diagnosticsSnapshot.issues.push_back(exception.what());
        return;
    }

    diagnosticsSnapshot.lastContentProbeCategory = lastContentFailureProbe.categoryId;
    diagnosticsSnapshot.lastContentProbeFailedGracefully = lastContentFailureProbe.failedGracefully;
    diagnosticsSnapshot.lastContentProbeState = lastContentFailureProbe.state;
    diagnosticsSnapshot.lastContentProbeIssues = lastContentFailureProbe.issues;

    if (lastContentFailureProbe.attempted && !lastContentFailureProbe.state.empty())
        diagnosticsSnapshot.failureState = lastContentFailureProbe.state;
    else if (!currentSessionState.transientMetrics.lastFailure.empty())
        diagnosticsSnapshot.failureState = currentSessionState.transientMetrics.lastFailure;

    diagnosticsSnapshot.hasFailure = !diagnosticsSnapshot.failureState.empty() || !diagnosticsSnapshot.issues.empty();
}
} // namespace drs::engine
