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

void syncDraftPlaybackIntoDiagnostics(const DraftPlaybackStatus& status,
                                      EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.draftRevision = status.draftRevision;
    diagnosticsSnapshot.previewRevision = status.preview.revision;
    diagnosticsSnapshot.publishedRevision = status.performance.revision;
    diagnosticsSnapshot.previewPending = status.pendingPreview.active;
    diagnosticsSnapshot.publishedPending = status.pendingPerformance.active;
    diagnosticsSnapshot.previewRevisionState = status.preview.state;
    diagnosticsSnapshot.publishedRevisionState = status.performance.state;
    diagnosticsSnapshot.draftPlaybackEvent = status.lastEvent;
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
    : referenceManifest(loadPhase1ReferenceInstrumentManifest())
{
    if (referenceManifest.loaded)
    {
        currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
        currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
        referenceStream = loadPhase1ReferenceStreamContainer();
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

EnginePerformanceSnapshot EngineFacade::getPerformanceSnapshot() const
{
    EnginePerformanceSnapshot snapshot;
    const auto& draftStatus = draftPlaybackContract.getStatus();
    snapshot.loaded = referenceInstrumentActive && referenceManifest.loaded && referenceStream.loaded
        && draftStatus.performance.available;
    snapshot.draftRevision = draftStatus.draftRevision;
    snapshot.previewRevision = draftStatus.preview.revision;
    snapshot.publishedRevision = draftStatus.performance.revision;
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
    snapshot.previewRevisionState = draftStatus.preview.state;
    snapshot.publishedRevisionState = draftStatus.performance.state;
    snapshot.draftPlaybackEvent = draftStatus.lastEvent;
    snapshot.loadIndicator = referenceInstrumentActive
        ? buildLoadIndicator(referenceManifest, referenceStream, currentSessionState)
        : "Click Load Default or Load Lead Demo";

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

    currentSessionState.selectedArticulationId = articulationId;
    currentSessionState.transientMetrics.integrationState = "Performance surface articulation updated";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    previewPlaybackSnapshot.articulationId = articulationId;
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
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
        currentIterator->value = clampedValue;
    }
    else
    {
        currentSessionState.macroValues.push_back({ macroId, clampedValue });
    }

    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    return true;
}

bool EngineFacade::stageDraftRevision(std::size_t revision)
{
    if (!referenceInstrumentActive || !draftPlaybackContract.getStatus().projectOpen)
        return false;

    if (!draftPlaybackContract.setDraftRevision(revision))
        return false;

    currentSessionState.transientMetrics.integrationState = "Draft revision staged";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    return true;
}

bool EngineFacade::refreshPreviewToCurrentDraft()
{
    if (!referenceInstrumentActive || !referenceManifest.loaded || !referenceStream.loaded)
        return false;

    const auto request = draftPlaybackContract.requestPreviewBuild();
    if (!request.accepted)
        return false;

    if (!draftPlaybackContract.completePreviewBuild(request.requestId))
        return false;

    currentSessionState.transientMetrics.integrationState = "Preview revision prepared";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    return true;
}

bool EngineFacade::publishCurrentDraft()
{
    if (!referenceInstrumentActive || !referenceManifest.loaded || !referenceStream.loaded)
        return false;

    const auto request = draftPlaybackContract.requestPerformanceBuild();
    if (!request.accepted)
        return false;

    if (!draftPlaybackContract.completePerformanceBuild(request.requestId))
        return false;

    currentSessionState.transientMetrics.integrationState = "Published revision activated";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    return true;
}

void EngineFacade::closeDraftPlaybackProject()
{
    draftPlaybackContract.closeProject();
    referenceInstrumentActive = false;
    currentSessionState.transientMetrics.integrationState = "Draft playback project closed";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
}

bool EngineFacade::reopenDraftPlaybackProject(std::size_t revision)
{
    if (!referenceManifest.loaded)
        return false;

    draftPlaybackContract.reopenProject(revision);
    referenceInstrumentActive = true;
    currentSessionState.transientMetrics.integrationState = "Draft playback project reopened";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    return true;
}

bool EngineFacade::beginDraftPlaybackDeviceRestart()
{
    if (!draftPlaybackContract.beginDeviceRestart())
        return false;

    currentSessionState.transientMetrics.integrationState = "Device restart in progress";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    return true;
}

bool EngineFacade::completeDraftPlaybackDeviceRestart(bool restored)
{
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
    return true;
}

EnginePreviewPlaybackSnapshot EngineFacade::auditionPreviewNote(int midiNote, int velocity)
{
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
        return restoreResult;
    }

    if (!referenceManifest.loaded)
    {
        restoreResult.issues.push_back("Reference instrument manifest is unavailable, so preset state cannot be restored.");
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = restoreResult.issues.front();
        refreshDiagnosticsSnapshot();
        return restoreResult;
    }

    const auto validation = validateRuntimePresetState(parsedState.preset, referenceManifest.instrument);
    if (!validation.valid)
    {
        restoreResult.issues = validation.issues;
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = summarizeIssues(validation.issues);
        refreshDiagnosticsSnapshot();
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
    return probeResult;
}

void EngineFacade::clearContentFailureProbe()
{
    lastContentFailureProbe = {};
    refreshDiagnosticsSnapshot();
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
}

void EngineFacade::initializeDraftPlaybackContract(bool activatePerformanceRevision)
{
    draftPlaybackContract.reopenProject(0);

    if (!referenceManifest.loaded || !referenceStream.loaded)
    {
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        return;
    }

    if (const auto previewRequest = draftPlaybackContract.requestPreviewBuild(); previewRequest.accepted)
        draftPlaybackContract.completePreviewBuild(previewRequest.requestId);

    if (activatePerformanceRevision)
    {
        if (const auto publishRequest = draftPlaybackContract.requestPerformanceBuild(); publishRequest.accepted)
            draftPlaybackContract.completePerformanceBuild(publishRequest.requestId);
    }

    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
}

void EngineFacade::refreshDiagnosticsSnapshot()
{
    diagnosticsSnapshot = {};
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    syncDraftPlaybackIntoDiagnostics(draftPlaybackContract.getStatus(), diagnosticsSnapshot);

    if (!referenceInstrumentActive)
    {
        diagnosticsSnapshot.headline = "No instrument loaded";
        diagnosticsSnapshot.failureState = "Reference instrument is available but inactive.";
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
    diagnosticsSnapshot.headline = "Reference runtime diagnostics ready";
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
