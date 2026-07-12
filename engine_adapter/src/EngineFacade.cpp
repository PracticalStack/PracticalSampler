#include "drs/engine/EngineFacade.h"
#include "drs/engine/HiseFrontendBridge.h"
#include "drs/engine/HiseProjectContent.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/HiseVendorInfo.generated.h"

#include <sstream>

namespace drs::engine
{
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
    }

    if (!runtimeManifest.issues.empty())
    {
        detail << "Runtime manifest issues:\n";

        for (const auto& issue : runtimeManifest.issues)
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
        integrationState,
        detail.str(),
        nextSteps
    };
}

RuntimeManifestLoadResult EngineFacade::loadPhase1ReferenceInstrument() const
{
    return loadPhase1ReferenceInstrumentManifest();
}
} // namespace drs::engine
