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

double normalizeMacroValue(double value)
{
    constexpr auto precisionScale = 1000000.0;
    return std::round(value * precisionScale) / precisionScale;
}

void syncSessionSelectionsIntoDiagnostics(const RuntimeSessionStateSnapshot& sessionState,
                                          EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.presetId = sessionState.presetId;
    diagnosticsSnapshot.loadProfileId = sessionState.loadProfileId;
    diagnosticsSnapshot.selectedArticulationId = sessionState.selectedArticulationId;
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
    }
    else
    {
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
    }

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
    return referenceManifest;
}

RuntimeStreamLoadResult EngineFacade::loadPhase1ReferenceStream() const
{
    return referenceStream;
}

std::vector<EngineMacroDescriptor> EngineFacade::getMacroDescriptors() const
{
    std::vector<EngineMacroDescriptor> descriptors;

    if (!referenceManifest.loaded)
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
            currentValue
        });
    }

    return descriptors;
}

bool EngineFacade::setMacroValue(const std::string& macroId, double value)
{
    if (!referenceManifest.loaded)
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
        currentSessionState = {};
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
        lastContentFailureProbe = {};
        refreshDiagnosticsSnapshot();
        return;
    }

    currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
    currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
    currentSessionState.transientMetrics.lastFailure.clear();
    lastContentFailureProbe = {};
    refreshDiagnosticsSnapshot();
}

void EngineFacade::refreshDiagnosticsSnapshot()
{
    diagnosticsSnapshot = {};
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);

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
