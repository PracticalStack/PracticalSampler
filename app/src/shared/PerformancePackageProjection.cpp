#include "shared/PerformancePackageProjection.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace drs::app
{
namespace
{
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

    return result.empty() ? std::string(fallback) : result;
}

void collectCompatibilityIssues(const drs::engine::RuntimeProjectModel& project,
                                std::vector<std::string>& issues)
{
    if (!project.authoring.performanceBanks.empty())
        issues.push_back("Playable package export does not yet support authored performance banks.");

    for (const auto& group : project.authoring.groups)
    {
        if (std::abs(group.pan) > 0.000001)
        {
            issues.push_back("Group '" + group.id
                             + "' uses non-default pan, which playable package export does not yet preserve.");
        }
    }

    for (const auto& zone : project.authoring.zones)
    {
        if (std::abs(zone.pan) > 0.000001)
        {
            issues.push_back("Zone '" + zone.id
                             + "' uses non-default pan, which playable package export does not yet preserve.");
        }

        if (zone.loopEnabled || zone.loopStartFrame != 0 || zone.loopEndFrame != 0)
        {
            issues.push_back("Zone '" + zone.id
                             + "' uses loop settings, which playable package export does not yet preserve.");
        }
    }
}
} // namespace

PerformancePackageProjectionResult projectPerformancePackage(
    const drs::engine::RuntimeProjectModel& project,
    const drs::engine::RuntimeSessionStateSnapshot& sessionState,
    PerformancePackageProjectionContext context)
{
    PerformancePackageProjectionResult result;
    result.state = "Playable package export validation failed";

    if (project.sampleSources.empty())
        result.issues.push_back("The current project has no sample sources to export.");
    if (project.authoring.zones.empty())
        result.issues.push_back("The current project has no playable zones to export.");
    collectCompatibilityIssues(project, result.issues);

    const auto baseId = sanitizeExportStableId(
        !project.projectId.empty() ? std::string_view(project.projectId)
                                   : std::string_view(context.fallbackPackageName),
        "playable-package");
    const auto displayName = !project.displayName.empty() && project.displayName != "No Project Loaded"
        ? project.displayName
        : context.fallbackPackageName;
    const auto hasAuthoredGraph = !project.authoring.fxSlots.empty()
        || !project.authoring.routingBuses.empty();
    const auto hasAuthoredMacroTargets = std::any_of(
        project.authoring.macros.begin(), project.authoring.macros.end(), [](const auto& macro)
        {
            return !macro.targets.empty();
        });
    const auto requiresExtendedPackageRuntime = hasAuthoredGraph || hasAuthoredMacroTargets;

    auto& plan = result.compilePlan;
    plan.outputProjectPath = std::move(context.outputProjectPath);
    plan.outputInstrumentPath = std::move(context.outputInstrumentPath);
    plan.outputStreamPath = std::move(context.outputStreamPath);
    plan.projectId = !project.projectId.empty() ? project.projectId : baseId;
    plan.projectDisplayName = displayName;
    plan.contentRootPath = project.contentRootPath;
    plan.instrumentId = plan.projectId + ".instrument";
    plan.instrumentDisplayName = displayName;
    plan.defaultLoadProfile = sessionState.loadProfileId.empty() ? "balanced" : sessionState.loadProfileId;
    plan.masterGainDb = project.authoring.masterGainDb;
    plan.pageSizeBytes = 65536;
    plan.sampleSources = std::move(context.sampleSources);
    plan.fxSlots = project.authoring.fxSlots;
    plan.routingBuses = project.authoring.routingBuses;
    plan.projectNotes = project.notes;
    plan.instrumentValidationNotes = {
        "Exported from the current Practical Sampler authoring project."
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
        macro.targets = projectMacro.targets;
        macro.exposedInPerformance = projectMacro.exposedInPerformance;
        plan.macros.push_back(std::move(macro));
    }

    std::unordered_set<std::string> sampleSourceIds;
    sampleSourceIds.reserve(project.sampleSources.size());
    for (const auto& sampleSource : project.sampleSources)
    {
        if (sampleSource.id.empty())
        {
            result.issues.push_back("Every sample source must have a stable id before export.");
            continue;
        }
        if (sampleSource.path.empty())
        {
            result.issues.push_back("Sample source '" + sampleSource.id
                                    + "' does not point to a source file.");
            continue;
        }
        if (!sampleSourceIds.insert(sampleSource.id).second)
            result.issues.push_back("Sample source id '" + sampleSource.id + "' is duplicated.");
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
        group.routingBusId = requiresExtendedPackageRuntime
            ? (projectGroup.routingBusId.empty() ? "master" : projectGroup.routingBusId)
            : std::string {};
        if (!hasAuthoredGraph && !projectGroup.routingBusId.empty()
            && projectGroup.routingBusId != "master")
        {
            result.issues.push_back("Group '" + projectGroup.id + "' references routing bus '"
                                    + projectGroup.routingBusId
                                    + "' but the project defines no authored routing graph.");
        }
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
        if (sampleSourceIds.count(projectZone.sampleSourceId) == 0)
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
            group.routingBusId = requiresExtendedPackageRuntime ? "master" : std::string {};
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
        zone.sampleStartFrame = projectZone.sampleStartFrame;
        zone.releaseSeconds = projectZone.releaseSeconds;
        zone.releaseShape = projectZone.releaseShape;
        zone.roundRobin = projectZone.roundRobin;
        zone.roundRobinLength = projectZone.roundRobinLength;
        zone.roundRobinPosition = projectZone.roundRobinPosition;
        zone.triggerMode = projectZone.triggerMode;
        zone.performance = projectZone.performance;
        zone.exclusiveGroupId = projectZone.exclusiveGroupId;
        zone.exclusiveTargetGroupIds = projectZone.exclusiveTargetGroupIds;
        zone.chokeReleaseSeconds = projectZone.chokeReleaseSeconds;
        zone.prefetchBytes = 16384;
        zone.fineTuneCents = projectZone.fineTuneCents;
        zone.amplitudeVelocityTracking = projectZone.amplitudeVelocityTracking;
        zone.controllerConditions = projectZone.controllerConditions;
        zone.damper = projectZone.damper;
        plan.zones.push_back(std::move(zone));
    }

    plan.roundRobinResetRules = project.authoring.roundRobinResetRules;
    plan.controllerDefaults = project.authoring.controllerDefaults;

    result.manifest.schemaVersion = requiresExtendedPackageRuntime
        ? drs::engine::performancePackageFxRoutingSchemaVersion
        : drs::engine::performancePackageLegacySchemaVersion;
    result.manifest.packageId = baseId;
    result.manifest.displayName = displayName;
    result.manifest.instrumentId = plan.instrumentId;
    result.manifest.defaultLoadProfile = plan.defaultLoadProfile;
    result.manifest.minimumReaderSchemaVersion = requiresExtendedPackageRuntime
        ? drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion
        : drs::engine::performancePackageLegacySchemaVersion;
    result.manifest.masterGainDb = plan.masterGainDb;
    result.manifest.notes = {
        "Exported from the current Practical Sampler authoring project."
    };

    if (!result.issues.empty())
        return result;

    result.projected = true;
    result.state = "Playable package export ready";
    return result;
}
} // namespace drs::app
