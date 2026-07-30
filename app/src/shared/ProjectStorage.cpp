#include "shared/ProjectStorage.h"

#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace drs::app
{
namespace
{
std::uint64_t computeFnv1a64(std::string_view text) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    return hash;
}

std::optional<drs::engine::RoundRobinDescriptor> materializeRoundRobinDescriptor(
    const drs::engine::RuntimeProjectZoneDefinition& zone)
{
    if (zone.roundRobin.has_value())
        return zone.roundRobin;

    if (zone.roundRobinLength <= 0 || zone.roundRobinPosition <= 0)
        return std::nullopt;

    std::ostringstream stream;
    stream << zone.groupId
           << "|"
           << zone.articulationId
           << "|"
           << zone.rootKey
           << "|"
           << zone.keyLow
           << "|"
           << zone.keyHigh
           << "|"
           << zone.roundRobinLength
           << "|"
           << static_cast<int>(zone.triggerMode);

    drs::engine::RoundRobinDescriptor roundRobin;
    roundRobin.poolId = "legacy-rr-" + std::to_string(computeFnv1a64(stream.str()));
    roundRobin.slotCount = zone.roundRobinLength;
    roundRobin.slotIndex = zone.roundRobinPosition;
    roundRobin.mode = drs::engine::RoundRobinMode::sequential;
    return roundRobin;
}

std::uint64_t buildCrossfadePairingKey(const drs::engine::RuntimeZoneDefinition& zone)
{
    std::ostringstream stream;
    stream << zone.articulationId
           << "|" << zone.rootKey
           << "|" << zone.keyLow
           << "|" << zone.keyHigh
           << "|" << static_cast<int>(zone.triggerMode);
    return computeFnv1a64(stream.str());
}

void populateCrossfadeRuntimeDescriptors(std::vector<drs::engine::RuntimeZoneDefinition>& zones)
{
    std::vector<drs::engine::VelocityCrossfadeTopologyZoneDefinition> topologyZones;
    topologyZones.reserve(zones.size());

    for (const auto& zone : zones)
    {
        drs::engine::VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = buildCrossfadePairingKey(zone);
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinPoolId = zone.roundRobin.has_value() ? zone.roundRobin->poolId : std::string {};
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        topologyZones.push_back(topologyZone);
    }

    const auto runtimeTopology = drs::engine::buildFirstPassVelocityCrossfadeRuntimeTopology(topologyZones);
    for (std::size_t index = 0; index < zones.size(); ++index)
    {
        auto& zone = zones[index];
        zone.velocityCrossfadeRuntime = {};
        if (!drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            continue;

        const auto& topology = runtimeTopology[index];
        zone.velocityCrossfadeRuntime.effectiveLowVelocity = topology.effectiveLowVelocity;
        zone.velocityCrossfadeRuntime.effectiveHighVelocity = topology.effectiveHighVelocity;
        zone.velocityCrossfadeRuntime.fadeInOverlapLowVelocity = topology.fadeInOverlapLowVelocity;
        zone.velocityCrossfadeRuntime.fadeInOverlapHighVelocity = topology.fadeInOverlapHighVelocity;
        zone.velocityCrossfadeRuntime.fadeOutOverlapLowVelocity = topology.fadeOutOverlapLowVelocity;
        zone.velocityCrossfadeRuntime.fadeOutOverlapHighVelocity = topology.fadeOutOverlapHighVelocity;

        if (topology.fadeInNeighborZoneIndex >= 0)
        {
            zone.velocityCrossfadeRuntime.fadeInNeighborZoneId =
                zones[static_cast<std::size_t>(topology.fadeInNeighborZoneIndex)].id;
        }

        if (topology.fadeOutNeighborZoneIndex >= 0)
        {
            zone.velocityCrossfadeRuntime.fadeOutNeighborZoneId =
                zones[static_cast<std::size_t>(topology.fadeOutNeighborZoneIndex)].id;
        }
    }
}
} // namespace

juce::File ensureProjectFileExtension(juce::File file)
{
    if (file == juce::File())
        return {};

    if (file.hasFileExtension(".drsproj"))
        return file;

    return file.withFileExtension(".drsproj");
}

juce::File getDefaultStudioProjectDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("DecentRhapsodyStudio");
}

juce::File makeSelfContainedProjectFile(juce::File selectedFile)
{
    selectedFile = ensureProjectFileExtension(std::move(selectedFile));
    if (selectedFile == juce::File())
        return {};

    const auto projectName = selectedFile.getFileNameWithoutExtension();
    if (projectName.trim().isEmpty())
        return {};

    const auto selectedDirectory = selectedFile.getParentDirectory();

    if (selectedDirectory.getFileName() == projectName)
        return selectedFile;

    return selectedDirectory.getChildFile(projectName).getChildFile(selectedFile.getFileName());
}

bool ensureProjectFolderLayout(const juce::File& projectFile)
{
    if (projectFile == juce::File())
        return false;

    const auto projectDirectory = projectFile.getParentDirectory();
    if ((!projectDirectory.exists() && !projectDirectory.createDirectory()) || !projectDirectory.isDirectory())
        return false;

    const auto samplesDirectory = projectDirectory.getChildFile("Samples");
    return (samplesDirectory.exists() || samplesDirectory.createDirectory()) && samplesDirectory.isDirectory();
}

drs::engine::RuntimeInstrumentModel buildInstrumentManifestForProject(
    const drs::engine::RuntimeProjectModel& project,
    const juce::File& projectFile)
{
    drs::engine::RuntimeInstrumentModel instrument;
    instrument.schemaName = "drs.instrument";
    instrument.schemaVersion = 2;
    instrument.instrumentId = project.projectId.empty() ? "instrument" : project.projectId + ".instrument";
    instrument.displayName = project.displayName;
    instrument.sourceProjectPath = projectFile.getFullPathName().toStdString();
    instrument.compiledStreamAssetPath = projectFile.withFileExtension(".drstrm").getFullPathName().toStdString();
    instrument.defaultLoadProfile = "balanced";

    instrument.macros.reserve(project.authoring.macros.size());
    for (const auto& projectMacro : project.authoring.macros)
    {
        drs::engine::RuntimeMacroDefinition macro;
        macro.id = projectMacro.id;
        macro.name = projectMacro.name;
        macro.defaultValue = projectMacro.defaultValue;
        macro.minValue = projectMacro.minValue;
        macro.maxValue = projectMacro.maxValue;
        instrument.macros.push_back(std::move(macro));
    }

    std::unordered_map<std::string, const drs::engine::RuntimeProjectSampleSource*> sampleSources;
    sampleSources.reserve(project.sampleSources.size());
    for (const auto& sampleSource : project.sampleSources)
        sampleSources.emplace(sampleSource.id, &sampleSource);

    std::unordered_map<std::string, std::size_t> articulationIndexes;
    std::unordered_map<std::string, std::size_t> groupIndexes;

    instrument.groups.reserve(project.authoring.groups.size());
    for (const auto& projectGroup : project.authoring.groups)
    {
        if (projectGroup.id.empty() || groupIndexes.count(projectGroup.id))
            continue;

        drs::engine::RuntimeGroupDefinition group;
        group.id = projectGroup.id;
        group.name = projectGroup.displayName.empty() ? projectGroup.id : projectGroup.displayName;
        groupIndexes.emplace(group.id, instrument.groups.size());
        instrument.groups.push_back(std::move(group));
    }

    for (const auto& projectZone : project.authoring.zones)
    {
        if (!projectZone.articulationId.empty() && !articulationIndexes.count(projectZone.articulationId))
        {
            drs::engine::RuntimeArticulationDefinition articulation;
            articulation.id = projectZone.articulationId;
            articulation.name = projectZone.articulationId;
            articulation.isDefault = instrument.articulations.empty();
            articulationIndexes.emplace(articulation.id, instrument.articulations.size());
            instrument.articulations.push_back(std::move(articulation));
        }

        if (!projectZone.groupId.empty() && !groupIndexes.count(projectZone.groupId))
        {
            drs::engine::RuntimeGroupDefinition group;
            group.id = projectZone.groupId;
            group.name = projectZone.groupId;
            groupIndexes.emplace(group.id, instrument.groups.size());
            instrument.groups.push_back(std::move(group));
        }

        if (!projectZone.groupId.empty() && !projectZone.articulationId.empty())
        {
            auto& articulationIds = instrument.groups[groupIndexes.at(projectZone.groupId)].articulationIds;
            if (std::find(articulationIds.begin(), articulationIds.end(), projectZone.articulationId)
                == articulationIds.end())
                articulationIds.push_back(projectZone.articulationId);
        }

        const auto source = sampleSources.find(projectZone.sampleSourceId);
        if (source == sampleSources.end())
            continue;

        drs::engine::RuntimeZoneDefinition zone;
        zone.id = projectZone.id;
        zone.groupId = projectZone.groupId;
        zone.articulationId = projectZone.articulationId;
        zone.samplePath = source->second->path;
        zone.streamAssetPath = instrument.compiledStreamAssetPath;
        zone.rootKey = projectZone.rootKey;
        zone.keyLow = projectZone.keyLow;
        zone.keyHigh = projectZone.keyHigh;
        zone.velocityLow = projectZone.velocityLow;
        zone.velocityHigh = projectZone.velocityHigh;
        zone.velocityCrossfade = projectZone.velocityCrossfade;
        zone.streamOffsetBytes = 0;
        zone.prefetchBytes = 16384;
        zone.releaseSeconds = projectZone.releaseSeconds;
        if (const auto roundRobin = materializeRoundRobinDescriptor(projectZone))
        {
            zone.roundRobin = *roundRobin;
            zone.roundRobinLength = roundRobin->slotCount;
            zone.roundRobinPosition = roundRobin->slotIndex;
        }
        else
        {
            zone.roundRobinLength = projectZone.roundRobinLength;
            zone.roundRobinPosition = projectZone.roundRobinPosition;
        }
        zone.triggerMode = projectZone.triggerMode;
        instrument.zones.push_back(std::move(zone));
    }

    populateCrossfadeRuntimeDescriptors(instrument.zones);

    instrument.validationNotes = {
        "Generated from the current Decent Rhapsody Studio authoring project when the project was saved."
    };
    return instrument;
}

ProjectFilesSaveResult saveProjectFiles(const drs::engine::RuntimeProjectModel& project,
                                        const juce::File& projectFile)
{
    ProjectFilesSaveResult result;
    if (!ensureProjectFolderLayout(projectFile))
    {
        result.errorMessage = "The project folder and its Samples directory could not be created at:\n"
            + projectFile.getParentDirectory().getFullPathName();
        return result;
    }

    const auto instrumentFile = projectFile.withFileExtension(".drinst");
    const auto instrument = buildInstrumentManifestForProject(project, projectFile);
    const auto serializedProject = drs::engine::serializeRuntimeProjectManifest(
        project, projectFile.getFullPathName().toStdString());
    const auto serializedInstrument = drs::engine::serializeRuntimeInstrumentManifest(
        instrument, instrumentFile.getFullPathName().toStdString());

    juce::TemporaryFile temporaryProject(projectFile);
    juce::TemporaryFile temporaryInstrument(instrumentFile);
    if (!temporaryProject.getFile().replaceWithText(
            juce::String::fromUTF8(serializedProject.c_str()), false, false, "\n"))
    {
        result.errorMessage = "The project could not be written to:\n" + projectFile.getFullPathName();
        return result;
    }

    if (!temporaryInstrument.getFile().replaceWithText(
            juce::String::fromUTF8(serializedInstrument.c_str()), false, false, "\n"))
    {
        result.errorMessage = "The instrument could not be written to:\n" + instrumentFile.getFullPathName();
        return result;
    }

    if (!temporaryInstrument.overwriteTargetFileWithTemporary())
    {
        result.errorMessage = "The instrument could not be saved to:\n" + instrumentFile.getFullPathName();
        return result;
    }

    if (!temporaryProject.overwriteTargetFileWithTemporary())
    {
        result.errorMessage = "The project could not be saved to:\n" + projectFile.getFullPathName();
        return result;
    }

    result.saved = true;
    return result;
}
} // namespace drs::app
