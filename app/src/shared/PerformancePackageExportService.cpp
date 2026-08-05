#include "shared/PerformancePackageExportService.h"

#include "drs/engine/PackageReader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace drs::app
{
namespace
{
namespace fs = std::filesystem;

constexpr double validationProgress = 0.05;
constexpr double compileStartProgress = 0.10;
constexpr double compileEndProgress = 0.25;
constexpr double streamStartProgress = 0.25;
constexpr double streamEndProgress = 0.75;
constexpr double packageStartProgress = 0.75;
constexpr double packageEndProgress = 0.95;
constexpr double verifyStartProgress = 0.95;
constexpr double verifyEndProgress = 1.0;

bool isTerminalStage(const PerformancePackageExportStage stage) noexcept
{
    return stage == PerformancePackageExportStage::completed
        || stage == PerformancePackageExportStage::canceled
        || stage == PerformancePackageExportStage::failed
        || stage == PerformancePackageExportStage::consumed;
}

double clampProgress(const double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

double mapPhaseProgress(const double start, const double end, const double localProgress) noexcept
{
    return start + (end - start) * clampProgress(localProgress);
}

double ratio(std::uint64_t numerator, std::uint64_t denominator) noexcept
{
    if (denominator == 0)
        return 0.0;

    return clampProgress(static_cast<double>(numerator) / static_cast<double>(denominator));
}

std::string sanitizeExportStableId(std::string_view source, std::string_view fallback)
{
    std::string result;
    result.reserve(source.size());

    auto separatorPending = false;
    for (const auto character : source)
    {
        const auto unsignedCharacter = static_cast<unsigned char>(character);
        if (std::isalnum(unsignedCharacter))
        {
            if (separatorPending && !result.empty())
                result.push_back('-');
            result.push_back(static_cast<char>(std::tolower(unsignedCharacter)));
            separatorPending = false;
        }
        else
        {
            separatorPending = !result.empty();
        }
    }

    if (result.empty())
        result = std::string(fallback);

    return result;
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return {};

    if (issues.size() == 1)
        return issues.front();

    return issues.front() + " (+" + std::to_string(issues.size() - 1) + " more)";
}

std::string resolveProjectSampleSourcePath(const drs::engine::RuntimeProjectModel& project,
                                           const drs::engine::RuntimeProjectSampleSource& sampleSource)
{
    const fs::path sourcePath(sampleSource.path);
    if (sourcePath.is_absolute() || project.contentRootPath.empty())
        return sourcePath.lexically_normal().generic_string();

    return (fs::path(project.contentRootPath) / sourcePath).lexically_normal().generic_string();
}

void collectPerformancePackageExportCompatibilityIssues(
    const drs::engine::RuntimeProjectModel& project,
    std::vector<std::string>& issues)
{
    for (const auto& macro : project.authoring.macros)
    {
        if (!macro.targets.empty())
        {
            issues.push_back("Macro '" + macro.id
                             + "' carries target mappings that playable package export does not yet preserve.");
        }
    }

    if (!project.authoring.fxSlots.empty())
        issues.push_back("Playable package export does not yet support authored FX slots.");

    if (!project.authoring.routingBuses.empty())
        issues.push_back("Playable package export does not yet support authored routing buses.");

    if (!project.authoring.performanceBanks.empty())
        issues.push_back("Playable package export does not yet support authored performance banks.");

    for (const auto& group : project.authoring.groups)
    {
        if (std::abs(group.pan) > 0.000001)
        {
            issues.push_back("Group '" + group.id
                             + "' uses non-default pan, which playable package export does not yet preserve.");
        }

        if (!group.routingBusId.empty() && group.routingBusId != "master")
        {
            issues.push_back("Group '" + group.id
                             + "' routes to '" + group.routingBusId
                             + "', which playable package export does not yet preserve.");
        }
    }

    for (const auto& zone : project.authoring.zones)
    {
        if (std::abs(zone.pan) > 0.000001)
        {
            issues.push_back("Zone '" + zone.id
                             + "' uses non-default pan, which playable package export does not yet preserve.");
        }

        if (zone.sampleStartFrame != 0)
        {
            issues.push_back("Zone '" + zone.id
                             + "' uses a non-zero sample start offset, which playable package does not yet preserve.");
        }

        if (zone.loopEnabled || zone.loopStartFrame != 0 || zone.loopEndFrame != 0)
        {
            issues.push_back("Zone '" + zone.id
                             + "' uses loop settings, which playable package export does not yet preserve.");
        }
    }
}

struct PerformancePackageExportPreparationResult
{
    bool ready = false;
    std::string state;
    std::vector<std::string> issues;
    drs::engine::RuntimeCompilePlan compilePlan;
    drs::engine::PerformancePackageManifest manifest;
};

PerformancePackageExportPreparationResult preparePerformancePackageExport(
    const drs::engine::RuntimeProjectModel& project,
    const drs::engine::RuntimeSessionStateSnapshot& sessionState,
    const juce::File& targetPackageFile,
    const juce::File& stagingDirectory)
{
    PerformancePackageExportPreparationResult result;
    result.state = "Playable package export validation failed";

    if (targetPackageFile == juce::File())
    {
        result.issues.push_back("Select a valid .drpkg export destination.");
        return result;
    }

    if (project.sampleSources.empty())
        result.issues.push_back("The current project has no sample sources to export.");
    if (project.authoring.zones.empty())
        result.issues.push_back("The current project has no playable zones to export.");

    collectPerformancePackageExportCompatibilityIssues(project, result.issues);

    const auto baseId = sanitizeExportStableId(
        !project.projectId.empty() ? std::string_view(project.projectId)
                                   : std::string_view(targetPackageFile.getFileNameWithoutExtension().toStdString()),
        "playable-package");
    const auto displayName = !project.displayName.empty() && project.displayName != "No Project Loaded"
        ? project.displayName
        : targetPackageFile.getFileNameWithoutExtension().toStdString();

    auto& plan = result.compilePlan;
    plan.outputProjectPath = stagingDirectory.getChildFile("export-runtime-project.drsproj")
                                 .getFullPathName()
                                 .toStdString();
    plan.outputInstrumentPath = stagingDirectory.getChildFile("export-runtime-instrument.drinst")
                                    .getFullPathName()
                                    .toStdString();
    plan.outputStreamPath = stagingDirectory.getChildFile("export-runtime-stream.drstrm")
                                .getFullPathName()
                                .toStdString();
    plan.projectId = !project.projectId.empty() ? project.projectId : baseId;
    plan.projectDisplayName = displayName;
    plan.contentRootPath = project.contentRootPath;
    plan.instrumentId = plan.projectId + ".instrument";
    plan.instrumentDisplayName = displayName;
    plan.defaultLoadProfile = sessionState.loadProfileId.empty() ? "balanced" : sessionState.loadProfileId;
    plan.masterGainDb = project.authoring.masterGainDb;
    plan.pageSizeBytes = 65536;
    plan.projectNotes = project.notes;
    plan.instrumentValidationNotes = {
        "Exported from the current Decent Rhapsody Studio authoring project."
    };
    plan.streamNotes = {
        "Generated for sealed playable package export."
    };

    for (const auto& projectMacro : project.authoring.macros)
    {
        drs::engine::RuntimeMacroDefinition macro;
        macro.id = projectMacro.id;
        macro.name = projectMacro.name;
        macro.defaultValue = projectMacro.defaultValue;
        macro.minValue = projectMacro.minValue;
        macro.maxValue = projectMacro.maxValue;
        plan.macros.push_back(std::move(macro));
    }

    std::unordered_map<std::string, const drs::engine::RuntimeProjectSampleSource*> sampleSourcesById;
    sampleSourcesById.reserve(project.sampleSources.size());
    for (const auto& sampleSource : project.sampleSources)
    {
        if (sampleSource.id.empty())
        {
            result.issues.push_back("Every sample source must have a stable id before export.");
            continue;
        }

        if (sampleSource.path.empty())
        {
            result.issues.push_back("Sample source '" + sampleSource.id + "' does not point to a source file.");
            continue;
        }

        if (!sampleSourcesById.emplace(sampleSource.id, &sampleSource).second)
        {
            result.issues.push_back("Sample source id '" + sampleSource.id + "' is duplicated.");
            continue;
        }

        const auto resolvedPath = resolveProjectSampleSourcePath(project, sampleSource);
        const auto inspection = drs::engine::inspectSampleFile(resolvedPath);
        if (!inspection.accepted)
        {
            auto issue = "Sample source '" + sampleSource.id + "' could not be prepared for export from '"
                + resolvedPath + "'.";
            if (!inspection.state.empty())
                issue += " " + inspection.state + ".";
            if (!inspection.issues.empty())
                issue += " " + summarizeIssues(inspection.issues);
            result.issues.push_back(std::move(issue));
            continue;
        }

        drs::engine::RuntimeCompileSourceDefinition compileSource;
        compileSource.id = sampleSource.id;
        compileSource.sourcePath = resolvedPath;
        compileSource.role = sampleSource.role;
        compileSource.metadata = inspection.metadata;
        plan.sampleSources.push_back(std::move(compileSource));
    }

    std::unordered_map<std::string, std::size_t> articulationIndexes;
    std::unordered_map<std::string, std::size_t> groupIndexes;

    plan.groups.reserve(project.authoring.groups.size());
    for (const auto& projectGroup : project.authoring.groups)
    {
        if (projectGroup.id.empty() || groupIndexes.count(projectGroup.id) != 0)
            continue;

        drs::engine::RuntimeGroupDefinition group;
        group.id = projectGroup.id;
        group.name = projectGroup.displayName.empty() ? projectGroup.id : projectGroup.displayName;
        group.gainDb = projectGroup.gainDb;
        groupIndexes.emplace(group.id, plan.groups.size());
        plan.groups.push_back(std::move(group));
    }

    plan.articulations.reserve(project.authoring.articulations.size());
    for (const auto& projectArticulation : project.authoring.articulations)
    {
        if (projectArticulation.id.empty() || articulationIndexes.count(projectArticulation.id) != 0)
            continue;

        drs::engine::RuntimeArticulationDefinition articulation;
        articulation.id = projectArticulation.id;
        articulation.name = projectArticulation.displayName.empty()
            ? projectArticulation.id
            : projectArticulation.displayName;
        articulation.isDefault = projectArticulation.isDefault;
        articulation.activation = projectArticulation.activation;
        articulationIndexes.emplace(articulation.id, plan.articulations.size());
        plan.articulations.push_back(std::move(articulation));
    }

    for (const auto& projectZone : project.authoring.zones)
    {
        if (projectZone.id.empty())
        {
            result.issues.push_back("Every exported zone must have a stable id.");
            continue;
        }

        if (sampleSourcesById.count(projectZone.sampleSourceId) == 0)
        {
            result.issues.push_back("Zone '" + projectZone.id + "' references missing sample source '"
                                    + projectZone.sampleSourceId + "'.");
            continue;
        }

        if (!projectZone.groupId.empty() && groupIndexes.count(projectZone.groupId) == 0)
        {
            drs::engine::RuntimeGroupDefinition group;
            group.id = projectZone.groupId;
            group.name = projectZone.groupId;
            groupIndexes.emplace(group.id, plan.groups.size());
            plan.groups.push_back(std::move(group));
        }

        if (!projectZone.articulationId.empty() && articulationIndexes.count(projectZone.articulationId) == 0)
        {
            drs::engine::RuntimeArticulationDefinition articulation;
            articulation.id = projectZone.articulationId;
            articulation.name = projectZone.articulationId;
            articulation.isDefault = plan.articulations.empty();
            articulationIndexes.emplace(articulation.id, plan.articulations.size());
            plan.articulations.push_back(std::move(articulation));
        }

        if (!projectZone.groupId.empty() && !projectZone.articulationId.empty())
        {
            auto& articulationIds = plan.groups[groupIndexes.at(projectZone.groupId)].articulationIds;
            if (std::find(articulationIds.begin(), articulationIds.end(), projectZone.articulationId)
                == articulationIds.end())
            {
                articulationIds.push_back(projectZone.articulationId);
            }
        }

        drs::engine::RuntimeCompileZoneDefinition zone;
        zone.id = projectZone.id;
        zone.sourceId = projectZone.sampleSourceId;
        zone.groupId = projectZone.groupId;
        zone.articulationId = projectZone.articulationId;
        zone.rootKey = projectZone.rootKey;
        zone.keyLow = projectZone.keyLow;
        zone.keyHigh = projectZone.keyHigh;
        zone.velocityLow = projectZone.velocityLow;
        zone.velocityHigh = projectZone.velocityHigh;
        zone.velocityCrossfade = projectZone.velocityCrossfade;
        zone.gainDb = projectZone.gainDb;
        zone.roundRobin = projectZone.roundRobin;
        zone.roundRobinLength = projectZone.roundRobinLength;
        zone.roundRobinPosition = projectZone.roundRobinPosition;
        zone.triggerMode = projectZone.triggerMode;
        zone.performance = projectZone.performance;
        zone.exclusiveGroupId = projectZone.exclusiveGroupId;
        zone.exclusiveTargetGroupIds = projectZone.exclusiveTargetGroupIds;
        zone.chokeReleaseSeconds = projectZone.chokeReleaseSeconds;
        zone.prefetchBytes = 16384;
        plan.zones.push_back(std::move(zone));
    }

    plan.roundRobinResetRules = project.authoring.roundRobinResetRules;

    result.manifest.packageId = baseId;
    result.manifest.displayName = displayName;
    result.manifest.instrumentId = plan.instrumentId;
    result.manifest.defaultLoadProfile = plan.defaultLoadProfile;
    result.manifest.minimumReaderSchemaVersion = drs::engine::performancePackageSchemaVersion;
    result.manifest.masterGainDb = plan.masterGainDb;
    result.manifest.notes = {
        "Exported from the current Decent Rhapsody Studio authoring project."
    };

    if (!result.issues.empty())
        return result;

    result.ready = true;
    result.state = "Playable package export ready";
    return result;
}

bool isCancellationRequested(const PerformancePackageExportExecutionOptions& options)
{
    return options.cancellationProbe && options.cancellationProbe();
}

void publishProgress(const PerformancePackageExportExecutionOptions& options,
                     const PerformancePackageExportStage stage,
                     const double progress01,
                     std::string status,
                     std::string detail = {},
                     const std::uint64_t bytesProcessed = 0,
                     const std::uint64_t totalBytes = 0,
                     std::string itemId = {})
{
    if (!options.progressSink)
        return;

    options.progressSink(PerformancePackageExportProgress { stage,
                                                            clampProgress(progress01),
                                                            std::move(status),
                                                            std::move(detail),
                                                            bytesProcessed,
                                                            totalBytes,
                                                            std::move(itemId) });
}

juce::String describeStage(const PerformancePackageExportStage stage)
{
    switch (stage)
    {
        case PerformancePackageExportStage::queued: return "Playable package export queued";
        case PerformancePackageExportStage::validating: return "Validating playable package export";
        case PerformancePackageExportStage::compiling: return "Compiling playable instrument";
        case PerformancePackageExportStage::writingStream: return "Writing compiled stream assets";
        case PerformancePackageExportStage::sealingPackage: return "Sealing playable package";
        case PerformancePackageExportStage::verifying: return "Verifying playable package";
        case PerformancePackageExportStage::completed: return "Playable package export complete";
        case PerformancePackageExportStage::canceled: return "Playable package export canceled";
        case PerformancePackageExportStage::failed: return "Playable package export failed";
        case PerformancePackageExportStage::consumed: return "Playable package export consumed";
        case PerformancePackageExportStage::idle: break;
    }

    return "Playable package export idle";
}

juce::String buildProgressDetailText(const PerformancePackageExportSnapshot& snapshot)
{
    juce::String detail = juce::String(snapshot.detail);
    if (snapshot.result != nullptr && snapshot.result->exported)
    {
        if (!detail.isEmpty())
            detail += "  ";
        detail += "Package size: "
            + juce::File::descriptionOfSizeInBytes(static_cast<int64_t>(snapshot.result->packageBytes));
        detail += "  Payloads: " + juce::String(static_cast<int>(snapshot.result->payloadCount));
    }

    return detail;
}
} // namespace

bool isPerformancePackageExportStageTransitionAllowed(const PerformancePackageExportStage from,
                                                      const PerformancePackageExportStage to) noexcept
{
    using Stage = PerformancePackageExportStage;
    switch (from)
    {
        case Stage::idle: return to == Stage::queued;
        case Stage::queued:
            return to == Stage::validating || to == Stage::canceled || to == Stage::failed;
        case Stage::validating:
            return to == Stage::compiling || to == Stage::canceled || to == Stage::failed;
        case Stage::compiling:
            return to == Stage::writingStream || to == Stage::canceled || to == Stage::failed;
        case Stage::writingStream:
            return to == Stage::sealingPackage || to == Stage::canceled || to == Stage::failed;
        case Stage::sealingPackage:
            return to == Stage::verifying || to == Stage::canceled || to == Stage::failed;
        case Stage::verifying:
            return to == Stage::completed || to == Stage::canceled || to == Stage::failed;
        case Stage::completed:
        case Stage::canceled:
        case Stage::failed:
            return to == Stage::consumed || to == Stage::queued;
        case Stage::consumed:
            return to == Stage::idle || to == Stage::queued;
    }

    return false;
}

const char* toString(const PerformancePackageExportStage stage) noexcept
{
    using Stage = PerformancePackageExportStage;
    switch (stage)
    {
        case Stage::idle: return "idle";
        case Stage::queued: return "queued";
        case Stage::validating: return "validating";
        case Stage::compiling: return "compiling";
        case Stage::writingStream: return "writingStream";
        case Stage::sealingPackage: return "sealingPackage";
        case Stage::verifying: return "verifying";
        case Stage::completed: return "completed";
        case Stage::canceled: return "canceled";
        case Stage::failed: return "failed";
        case Stage::consumed: return "consumed";
    }

    return "failed";
}

PerformancePackageExportOperationResult executePerformancePackageExport(
    const PerformancePackageExportRequest& request,
    const PerformancePackageExportExecutionOptions& options)
{
    PerformancePackageExportOperationResult result;
    result.packagePath = request.packagePath;
    result.state = "Playable package export failed";

    const auto targetPackageFile = request.packagePath.empty()
        ? juce::File {}
        : juce::File(request.packagePath)
              .withFileExtension(juce::String::fromUTF8(drs::engine::performancePackageFileExtension));
    if (targetPackageFile == juce::File())
    {
        result.issues.push_back("Select a valid .drpkg export destination.");
        return result;
    }

    publishProgress(options,
                    PerformancePackageExportStage::validating,
                    validationProgress,
                    "Validating playable package export",
                    targetPackageFile.getFileName().toStdString());

    const auto targetDirectory = targetPackageFile.getParentDirectory();
    if ((!targetDirectory.exists() && !targetDirectory.createDirectory()) || !targetDirectory.isDirectory())
    {
        result.issues.push_back("The playable package destination folder could not be created.");
        return result;
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return result;
    }

    const auto stagingDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("drs-package-export-" + juce::Uuid().toDashedString());
    const auto cleanupStagingDirectory = [&stagingDirectory]()
    {
        std::error_code cleanupError;
        fs::remove_all(fs::path(stagingDirectory.getFullPathName().toStdString()), cleanupError);
    };
    if ((!stagingDirectory.exists() && !stagingDirectory.createDirectory()) || !stagingDirectory.isDirectory())
    {
        result.issues.push_back("A temporary export staging directory could not be created.");
        return result;
    }

    const auto cleanupAndReturn = [&](PerformancePackageExportOperationResult terminalResult)
    {
        cleanupStagingDirectory();
        return terminalResult;
    };

    const auto preparation = preparePerformancePackageExport(request.project,
                                                             request.sessionState,
                                                             targetPackageFile,
                                                             stagingDirectory);
    if (!preparation.ready)
    {
        result.state = preparation.state;
        result.issues = preparation.issues;
        if (result.issues.empty())
            result.issues.push_back("The playable package export validation did not succeed.");
        return cleanupAndReturn(std::move(result));
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::compiling,
                    compileStartProgress,
                    "Compiling playable instrument",
                    "Building runtime manifest and stream plan");

    auto compileResult = drs::engine::compileRuntimeInstrument(preparation.compilePlan);
    if (!compileResult.compiled)
    {
        result.state = compileResult.state;
        result.issues = compileResult.issues;
        if (result.issues.empty())
            result.issues.push_back("The project could not be compiled for playable package export.");
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::compiling,
                    compileEndProgress,
                    "Compiling playable instrument",
                    "Runtime compile completed");

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    const auto streamWrite = drs::engine::writeCompiledStreamAssets(
        compileResult,
        drs::engine::RuntimeStreamWriteOptions {
            [options](const drs::engine::RuntimeStreamWriteProgress& progress)
            {
                publishProgress(options,
                                PerformancePackageExportStage::writingStream,
                                mapPhaseProgress(streamStartProgress,
                                                 streamEndProgress,
                                                 ratio(progress.bytesProcessed, progress.totalBytes)),
                                "Writing compiled stream assets",
                                progress.status,
                                progress.bytesProcessed,
                                progress.totalBytes,
                                progress.sampleId);
            },
            options.cancellationProbe,
            16384
        });
    if (!streamWrite.written)
    {
        result.state = streamWrite.state;
        result.issues = streamWrite.issues;
        if (result.issues.empty())
        {
            result.issues.push_back("The compiled runtime stream could not be materialized for playable package export.");
        }
        if (streamWrite.state.find("canceled") != std::string::npos)
            result.canceled = true;
        return cleanupAndReturn(std::move(result));
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    drs::engine::PerformancePackageCompileWritePlan packagePlan;
    packagePlan.manifest = preparation.manifest;
    packagePlan.compiledRuntime = std::move(compileResult);
    packagePlan.outputPackagePath = targetPackageFile.getFullPathName().toStdString();
    packagePlan.minimumCompatibleAppVersion = "0.5.0-internal";

    const auto packageWrite = drs::engine::writePerformancePackage(
        packagePlan,
        drs::engine::getDeterministicPackageCryptoProvider(),
        drs::engine::PerformancePackageWriteOptions {
            [options](const drs::engine::PerformancePackageWriteProgress& progress)
            {
                publishProgress(options,
                                PerformancePackageExportStage::sealingPackage,
                                mapPhaseProgress(packageStartProgress,
                                                 packageEndProgress,
                                                 ratio(progress.bytesProcessed, progress.totalBytes)),
                                "Sealing playable package",
                                progress.status,
                                progress.bytesProcessed,
                                progress.totalBytes,
                                progress.payloadId);
            },
            options.cancellationProbe
        });
    if (!packageWrite.written)
    {
        result.state = packageWrite.state;
        result.issues = packageWrite.issues;
        if (result.issues.empty())
            result.issues.push_back("The playable package file could not be written.");
        if (packageWrite.state.find("canceled") != std::string::npos)
            result.canceled = true;
        return cleanupAndReturn(std::move(result));
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::verifying,
                    verifyStartProgress,
                    "Verifying playable package",
                    juce::File(packagePlan.outputPackagePath).getFileName().toStdString());

    const auto verification = drs::engine::loadPerformancePackage(
        packagePlan.outputPackagePath,
        drs::engine::getDeterministicPackageCryptoProvider(),
        drs::engine::performancePackageSchemaVersion);
    if (!verification.loaded)
    {
        result.state = verification.state.empty()
            ? std::string("Playable package export verification failed")
            : verification.state;
        result.issues = verification.issues;
        if (result.issues.empty())
        {
            result.issues.push_back(
                "The exported playable package was written, but the current reader could not reopen it.");
        }
        return cleanupAndReturn(std::move(result));
    }

    result.exported = true;
    result.state = "Playable package exported";
    result.packagePath = packageWrite.packagePath;
    result.packageBytes = packageWrite.packageBytes;
    result.payloadCount = packageWrite.payloadCount;
    publishProgress(options,
                    PerformancePackageExportStage::verifying,
                    verifyEndProgress,
                    result.state,
                    "Playable package verification completed");
    return cleanupAndReturn(std::move(result));
}

PerformancePackageExportProgressComponent::PerformancePackageExportProgressComponent(
    CancelCallback callback)
    : cancelCallback(std::move(callback))
{
    setComponentID("performancePackageExportProgress");
    statusLabel.setComponentID("performancePackageExportProgressLabel");
    addAndMakeVisible(statusLabel);
    progressBar.setComponentID("performancePackageExportProgressBar");
    addAndMakeVisible(progressBar);
    detailLabel.setComponentID("performancePackageExportProgressDetailLabel");
    detailLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(detailLabel);
    cancelButton.setComponentID("performancePackageExportProgressCancelButton");
    cancelButton.onClick = [this]
    {
        if (cancelCallback)
            cancelCallback();
    };
    addAndMakeVisible(cancelButton);
    setVisible(false);
}

void PerformancePackageExportProgressComponent::setCancelCallback(CancelCallback callback)
{
    cancelCallback = std::move(callback);
}

void PerformancePackageExportProgressComponent::update(const PerformancePackageExportSnapshot& snapshot)
{
    progressValue = snapshot.stage == PerformancePackageExportStage::queued
        ? 0.0
        : clampProgress(snapshot.progress01);
    statusLabel.setText(describeStage(snapshot.stage), juce::dontSendNotification);
    detailLabel.setText(buildProgressDetailText(snapshot), juce::dontSendNotification);
    cancelButton.setEnabled(snapshot.stage == PerformancePackageExportStage::queued
                            || snapshot.stage == PerformancePackageExportStage::validating
                            || snapshot.stage == PerformancePackageExportStage::compiling
                            || snapshot.stage == PerformancePackageExportStage::writingStream
                            || snapshot.stage == PerformancePackageExportStage::sealingPackage
                            || snapshot.stage == PerformancePackageExportStage::verifying);
    setVisible(snapshot.stage != PerformancePackageExportStage::idle
               && snapshot.stage != PerformancePackageExportStage::consumed);
}

void PerformancePackageExportProgressComponent::resized()
{
    auto area = getLocalBounds().reduced(8, 6);
    statusLabel.setBounds(area.removeFromTop(22));
    cancelButton.setBounds(area.removeFromRight(80).removeFromTop(24));
    area.removeFromRight(8);
    progressBar.setBounds(area.removeFromTop(18));
    area.removeFromTop(6);
    detailLabel.setBounds(area.removeFromTop(20));
}

PerformancePackageExportService::Client::Client(PerformancePackageExportService* serviceIn,
                                                const std::uint64_t ownerId) noexcept
    : service(serviceIn), owner(ownerId)
{
}

PerformancePackageExportService::Client::~Client()
{
    reset();
}

PerformancePackageExportService::Client::Client(Client&& other) noexcept
    : service(other.service), owner(other.owner), activeGeneration(other.activeGeneration)
{
    other.service = nullptr;
    other.owner = 0;
    other.activeGeneration = 0;
}

PerformancePackageExportService::Client& PerformancePackageExportService::Client::operator=(
    Client&& other) noexcept
{
    if (this != &other)
    {
        reset();
        service = other.service;
        owner = other.owner;
        activeGeneration = other.activeGeneration;
        other.service = nullptr;
        other.owner = 0;
        other.activeGeneration = 0;
    }
    return *this;
}

void PerformancePackageExportService::Client::reset() noexcept
{
    if (service != nullptr && activeGeneration != 0)
    {
        service->cancel(owner, activeGeneration, "Playable package export owner closed");
        service->waitForTerminal(owner, activeGeneration, std::chrono::milliseconds::max());
    }
    service = nullptr;
    owner = 0;
    activeGeneration = 0;
}

PerformancePackageExportSubmitResult PerformancePackageExportService::Client::submit(
    PerformancePackageExportRequest request)
{
    if (service == nullptr)
        return {};

    const auto result = service->submit(owner, std::move(request));
    if (result.disposition == PerformancePackageExportSubmitDisposition::accepted)
        activeGeneration = result.identity.generation;
    return result;
}

bool PerformancePackageExportService::Client::cancel(std::string reason)
{
    return service != nullptr && activeGeneration != 0
        && service->cancel(owner, activeGeneration, std::move(reason));
}

bool PerformancePackageExportService::Client::waitForTerminal(
    const std::chrono::milliseconds timeout) const
{
    return service != nullptr && activeGeneration != 0
        && service->waitForTerminal(owner, activeGeneration, timeout);
}

bool PerformancePackageExportService::Client::waitForTerminal() const
{
    return waitForTerminal(std::chrono::milliseconds::max());
}

std::shared_ptr<const PerformancePackageExportSnapshot>
PerformancePackageExportService::Client::getSnapshot() const
{
    return service == nullptr ? nullptr : service->getSnapshot(owner, activeGeneration);
}

bool PerformancePackageExportService::Client::consume()
{
    return service != nullptr && activeGeneration != 0
        && service->consume(owner, activeGeneration);
}

PerformancePackageExportService::PerformancePackageExportService(
    PerformancePackageExportServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const PerformancePackageExportSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    metrics.liveWorkerCount = 1;
    worker = std::thread([this] { runWorker(); });
}

PerformancePackageExportService::~PerformancePackageExportService()
{
    shutdown();
}

PerformancePackageExportService::Client PerformancePackageExportService::openClient()
{
    return Client(this, nextOwnerId.fetch_add(1, std::memory_order_relaxed));
}

PerformancePackageExportSubmitResult PerformancePackageExportService::submit(
    const std::uint64_t ownerId,
    PerformancePackageExportRequest request)
{
    PerformancePackageExportSubmitResult result;
    if (request.projectId.empty() || request.packagePath.empty())
    {
        result.disposition = PerformancePackageExportSubmitDisposition::invalid;
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
        {
            result.disposition = PerformancePackageExportSubmitDisposition::shuttingDown;
            return result;
        }
        if (pending.has_value() || active.has_value())
        {
            ++metrics.rejectedBusyCount;
            result.disposition = PerformancePackageExportSubmitDisposition::busy;
            return result;
        }

        result.disposition = PerformancePackageExportSubmitDisposition::accepted;
        result.identity.ownerId = ownerId;
        result.identity.generation = ++nextGeneration;
        result.identity.projectId = request.projectId;
        result.identity.baseRevision = request.baseRevision;
        result.identity.packagePath = request.packagePath;
        pending = PendingRequest { result.identity,
                                   std::move(request),
                                   std::make_shared<std::atomic<bool>>(false),
                                   {} };
        ++metrics.requestedCount;
        metrics.maximumPendingCount = std::max<std::size_t>(metrics.maximumPendingCount, 1);
        const auto queued = std::make_shared<PerformancePackageExportSnapshot>(
            PerformancePackageExportSnapshot { result.identity,
                                              PerformancePackageExportStage::queued,
                                              0.0,
                                              "Playable package export queued",
                                              juce::File(result.identity.packagePath)
                                                  .getFileName()
                                                  .toStdString(),
                                              {} });
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const PerformancePackageExportSnapshot>(queued),
                                   std::memory_order_release);
    }
    condition.notify_one();
    return result;
}

PerformancePackageExportServiceMetrics PerformancePackageExportService::getMetrics() const
{
    std::lock_guard<std::mutex> lock(mutex);
    auto copy = metrics;
    copy.shutdownWaitDuration = std::chrono::microseconds(copy.maximumShutdownWaitMicros);
    copy.shutdownWaitMilliseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(copy.shutdownWaitDuration).count());
    return copy;
}

std::shared_ptr<const PerformancePackageExportSnapshot>
PerformancePackageExportService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

std::shared_ptr<const PerformancePackageExportSnapshot> PerformancePackageExportService::getSnapshot(
    const std::uint64_t ownerId,
    const std::uint64_t generation) const
{
    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    if (!current || current->identity.ownerId != ownerId
        || (generation != 0 && current->identity.generation != generation))
    {
        return nullptr;
    }
    return current;
}

bool PerformancePackageExportService::cancel(const std::uint64_t ownerId,
                                             const std::uint64_t generation,
                                             std::string reason)
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto matches = [&](const auto& request)
    {
        return request.has_value() && request->identity.ownerId == ownerId
            && request->identity.generation == generation;
    };
    if (matches(pending))
    {
        pending->cancellationReason = std::move(reason);
        pending->cancellation->store(true, std::memory_order_release);
        return true;
    }
    if (matches(active))
    {
        active->cancellationReason = std::move(reason);
        active->cancellation->store(true, std::memory_order_release);
        return true;
    }
    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    return current && current->identity.ownerId == ownerId
        && current->identity.generation == generation
        && !isTerminalStage(current->stage);
}

bool PerformancePackageExportService::waitForTerminal(const std::uint64_t ownerId,
                                                      const std::uint64_t generation,
                                                      const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        return shutdownRequested || (current && current->identity.ownerId == ownerId
            && current->identity.generation == generation && isTerminalStage(current->stage));
    });
}

bool PerformancePackageExportService::consume(const std::uint64_t ownerId, const std::uint64_t generation)
{
    std::shared_ptr<const PerformancePackageExportSnapshot> current;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (!currentSnapshot || currentSnapshot->identity.ownerId != ownerId
            || currentSnapshot->identity.generation != generation
            || !isTerminalStage(currentSnapshot->stage)
            || currentSnapshot->stage == PerformancePackageExportStage::consumed)
        {
            return false;
        }
        current = currentSnapshot;
    }
    publish(current->identity,
            PerformancePackageExportStage::consumed,
            current->progress01,
            "Playable package export consumed",
            current->detail,
            current->result);
    return true;
}

void PerformancePackageExportService::shutdown() noexcept
{
    std::lock_guard<std::mutex> shutdownLock(shutdownMutex);
    const auto started = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mutex);
        shutdownRequested = true;
        if (pending)
            pending->cancellation->store(true, std::memory_order_release);
        if (active)
            active->cancellation->store(true, std::memory_order_release);
    }
    condition.notify_all();
    terminalCondition.notify_all();
    if (worker.joinable() && worker.get_id() != std::this_thread::get_id())
        worker.join();
    std::lock_guard<std::mutex> lock(mutex);
    metrics.liveWorkerCount = 0;
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto micros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    metrics.maximumShutdownWaitMicros = std::max(metrics.maximumShutdownWaitMicros, micros);
}

bool PerformancePackageExportService::isTerminal(const PerformancePackageExportStage stage) const noexcept
{
    return isTerminalStage(stage);
}

std::string PerformancePackageExportService::cancellationReason(
    const PerformancePackageExportRequestIdentity& identity) const
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto matches = [&identity](const auto& request)
    {
        return request.has_value() && request->identity.ownerId == identity.ownerId
            && request->identity.generation == identity.generation;
    };
    if (matches(active))
        return active->cancellationReason;
    if (matches(pending))
        return pending->cancellationReason;
    return {};
}

void PerformancePackageExportService::publish(
    PerformancePackageExportRequestIdentity identity,
    const PerformancePackageExportStage stage,
    const double progress01,
    std::string status,
    std::string detail,
    std::shared_ptr<const PerformancePackageExportOperationResult> result)
{
    PerformancePackageExportServiceOptions localOptions;
    PerformancePackageExportStage publishedStage = PerformancePackageExportStage::idle;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        const auto previous = current ? current->stage : PerformancePackageExportStage::idle;
        if (current && current->identity.generation == identity.generation && previous != stage
            && !isPerformancePackageExportStageTransitionAllowed(previous, stage))
        {
            return;
        }

        auto next = std::make_shared<PerformancePackageExportSnapshot>();
        next->identity = std::move(identity);
        next->stage = stage;
        next->progress01 = clampProgress(progress01);
        next->status = std::move(status);
        next->detail = std::move(detail);
        next->result = std::move(result);
        publishedStage = next->stage;
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const PerformancePackageExportSnapshot>(std::move(next)),
                                   std::memory_order_release);

        if (stage == PerformancePackageExportStage::completed)
            ++metrics.completedCount;
        else if (stage == PerformancePackageExportStage::canceled)
            ++metrics.canceledCount;
        else if (stage == PerformancePackageExportStage::failed)
            ++metrics.failedCount;
        localOptions = options;
    }
    terminalCondition.notify_all();
    try
    {
        if (localOptions.stageObserver)
            localOptions.stageObserver(publishedStage);
    }
    catch (...)
    {
    }

    try
    {
        if (localOptions.checkpointObserver)
            localOptions.checkpointObserver(publishedStage);
    }
    catch (...)
    {
    }
}

void PerformancePackageExportService::runWorker()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        metrics.liveWorkerCount = 1;
    }

    while (true)
    {
        std::optional<PendingRequest> request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return shutdownRequested || pending.has_value(); });
            if (shutdownRequested && !pending.has_value())
                break;
            request = std::move(pending);
            pending.reset();
            active = request;
            metrics.maximumInFlightCount = std::max<std::size_t>(metrics.maximumInFlightCount, 1);
        }

        if (request)
        {
            try
            {
                process(std::move(*request));
            }
            catch (const std::exception& exception)
            {
                auto result = std::make_shared<PerformancePackageExportOperationResult>();
                result->state = exception.what();
                result->issues.push_back(exception.what());
                publish(request->identity,
                        request->cancellation->load(std::memory_order_acquire)
                            ? PerformancePackageExportStage::canceled
                            : PerformancePackageExportStage::failed,
                        1.0,
                        request->cancellation->load(std::memory_order_acquire)
                            ? "Playable package export canceled"
                            : "Playable package export failed",
                        exception.what(),
                        result);
            }
            catch (...)
            {
                auto result = std::make_shared<PerformancePackageExportOperationResult>();
                result->state = "Unexpected playable package export worker failure";
                result->issues.push_back(result->state);
                publish(request->identity,
                        PerformancePackageExportStage::failed,
                        1.0,
                        "Playable package export failed",
                        result->state,
                        result);
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            active.reset();
        }
    }

    std::lock_guard<std::mutex> lock(mutex);
    metrics.liveWorkerCount = 0;
}

void PerformancePackageExportService::process(PendingRequest pendingRequest)
{
    const auto identity = pendingRequest.identity;
    const auto progressCallback = [this, identity](const PerformancePackageExportProgress& progress)
    {
        publish(identity,
                progress.stage,
                progress.progress01,
                progress.status,
                progress.detail);
    };

    const auto result = std::make_shared<PerformancePackageExportOperationResult>(
        executePerformancePackageExport(
            pendingRequest.request,
            PerformancePackageExportExecutionOptions {
                progressCallback,
                [flag = pendingRequest.cancellation]
                {
                    return flag->load(std::memory_order_acquire);
                } }));

    if (result->exported)
    {
        publish(identity,
                PerformancePackageExportStage::completed,
                1.0,
                result->state,
                juce::File(result->packagePath).getFileName().toStdString(),
                result);
        return;
    }

    if (result->canceled)
    {
        const auto reason = cancellationReason(identity);
        publish(identity,
                PerformancePackageExportStage::canceled,
                1.0,
                "Playable package export canceled",
                reason.empty() ? result->state : reason,
                result);
        return;
    }

    publish(identity,
            PerformancePackageExportStage::failed,
            1.0,
            "Playable package export failed",
            !result->issues.empty() ? result->issues.front() : result->state,
            result);
}
} // namespace drs::app
