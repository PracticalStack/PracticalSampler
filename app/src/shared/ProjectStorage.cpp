#include "shared/ProjectStorage.h"

#include "drs/engine/RuntimeLoader.h"

#include <juce_graphics/juce_graphics.h>

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

struct ProjectTransactionFiles
{
    juce::File project;
    juce::File instrument;
    juce::File journal;
    juce::File projectBackup;
    juce::File instrumentBackup;
};

ProjectTransactionFiles makeProjectTransactionFiles(const juce::File& projectFile)
{
    const auto instrumentFile = projectFile.withFileExtension(".drinst");
    return {
        projectFile,
        instrumentFile,
        projectFile.getSiblingFile(projectFile.getFileName() + ".save-journal"),
        projectFile.getSiblingFile(projectFile.getFileName() + ".save-backup"),
        instrumentFile.getSiblingFile(instrumentFile.getFileName() + ".save-backup")
    };
}

bool removeFileIfPresent(const juce::File& file)
{
    return !file.exists() || file.deleteFile();
}

bool restoreTransactionTarget(const juce::File& target,
                              const juce::File& backup,
                              const bool targetPreviouslyExisted,
                              juce::String& errorMessage)
{
    if (!removeFileIfPresent(target))
    {
        errorMessage = "Could not remove the interrupted transaction target:\n"
            + target.getFullPathName();
        return false;
    }

    if (!targetPreviouslyExisted)
        return true;

    if (!backup.existsAsFile() || !backup.copyFileTo(target))
    {
        errorMessage = "Could not restore the transaction backup:\n"
            + backup.getFullPathName();
        return false;
    }

    return true;
}

bool writeTransactionJournal(const ProjectTransactionFiles& files,
                             const bool projectExisted,
                             const bool instrumentExisted)
{
    const juce::String journalText = "drs-project-save-transaction=1\nproject-existed="
        + juce::String(projectExisted ? 1 : 0)
        + "\ninstrument-existed=" + juce::String(instrumentExisted ? 1 : 0) + "\n";
    juce::TemporaryFile temporaryJournal(files.journal);
    return temporaryJournal.getFile().replaceWithText(journalText, false, false, "\n")
        && temporaryJournal.overwriteTargetFileWithTemporary();
}

bool checkpointAllowed(const ProjectFilesSaveOptions& options,
                       const ProjectFilesSaveCheckpoint checkpoint)
{
    return !options.allowCommitAtCheckpoint || options.allowCommitAtCheckpoint(checkpoint);
}

ProjectBackgroundImageImportResult buildBackgroundImageImportError(const juce::String& message)
{
    ProjectBackgroundImageImportResult result;
    result.errorMessage = message;
    return result;
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
    instrument.schemaVersion = project.schemaVersion >= 6 && project.authoring.schemaVersion >= 5 ? 3 : 2;
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

    if (project.schemaVersion >= 6 && project.authoring.schemaVersion >= 5)
    {
        instrument.articulations.reserve(project.authoring.articulations.size());
        for (const auto& projectArticulation : project.authoring.articulations)
        {
            if (projectArticulation.id.empty() || articulationIndexes.count(projectArticulation.id))
                continue;
            drs::engine::RuntimeArticulationDefinition articulation;
            articulation.id = projectArticulation.id;
            articulation.name = projectArticulation.displayName;
            articulation.isDefault = projectArticulation.isDefault;
            articulation.activation = projectArticulation.activation;
            articulationIndexes.emplace(articulation.id, instrument.articulations.size());
            instrument.articulations.push_back(std::move(articulation));
        }
    }

    for (const auto& projectZone : project.authoring.zones)
    {
        if (project.schemaVersion < 6 && !projectZone.articulationId.empty()
            && !articulationIndexes.count(projectZone.articulationId))
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
        zone.performance = projectZone.performance;
        zone.exclusiveGroupId = projectZone.exclusiveGroupId;
        zone.exclusiveTargetGroupIds = projectZone.exclusiveTargetGroupIds;
        zone.chokeReleaseSeconds = projectZone.chokeReleaseSeconds;
        instrument.zones.push_back(std::move(zone));
    }

    if (instrument.schemaVersion >= 3)
        instrument.roundRobinResetRules = project.authoring.roundRobinResetRules;

    populateCrossfadeRuntimeDescriptors(instrument.zones);

    instrument.validationNotes = {
        "Generated from the current Decent Rhapsody Studio authoring project when the project was saved."
    };
    return instrument;
}

ProjectFilesRecoveryResult recoverProjectFilesTransaction(const juce::File& projectFile)
{
    ProjectFilesRecoveryResult result;
    const auto files = makeProjectTransactionFiles(projectFile);
    if (!files.journal.existsAsFile())
        return result;

    result.recoveryNeeded = true;
    const auto journalText = files.journal.loadFileAsString();
    if (!journalText.contains("drs-project-save-transaction=1"))
    {
        result.errorMessage = "The project save recovery journal is not recognized:\n"
            + files.journal.getFullPathName();
        return result;
    }

    const auto projectExisted = journalText.contains("project-existed=1");
    const auto instrumentExisted = journalText.contains("instrument-existed=1");
    if (!restoreTransactionTarget(files.project,
                                  files.projectBackup,
                                  projectExisted,
                                  result.errorMessage)
        || !restoreTransactionTarget(files.instrument,
                                     files.instrumentBackup,
                                     instrumentExisted,
                                     result.errorMessage))
    {
        return result;
    }

    if (!removeFileIfPresent(files.journal))
    {
        result.errorMessage = "The recovered project save journal could not be removed:\n"
            + files.journal.getFullPathName();
        return result;
    }

    removeFileIfPresent(files.projectBackup);
    removeFileIfPresent(files.instrumentBackup);
    result.recovered = true;
    return result;
}

ProjectBackgroundImageImportResult importProjectBackgroundImage(const juce::File& sourceImageFile,
                                                               const juce::File& projectFile)
{
    const auto targetProjectFile = ensureProjectFileExtension(projectFile);
    if (targetProjectFile == juce::File())
    {
        return buildBackgroundImageImportError(
            "Save the project before importing a background image.");
    }

    if (!sourceImageFile.existsAsFile())
    {
        return buildBackgroundImageImportError(
            "The selected background image file does not exist.");
    }

    if (!sourceImageFile.hasFileExtension(".jpg;.jpeg"))
    {
        return buildBackgroundImageImportError(
            "The selected file must be a JPG image.");
    }

    auto inputStream = sourceImageFile.createInputStream();
    if (inputStream == nullptr)
    {
        return buildBackgroundImageImportError(
            "The selected background image could not be opened.");
    }

    auto* imageFormat = juce::ImageFileFormat::findImageFormatForStream(*inputStream);
    if (imageFormat == nullptr || dynamic_cast<juce::JPEGImageFormat*>(imageFormat) == nullptr)
    {
        return buildBackgroundImageImportError(
            "The selected file is not a valid JPG image.");
    }

    inputStream->setPosition(0);
    const auto image = imageFormat->decodeImage(*inputStream);
    if (!image.isValid())
    {
        return buildBackgroundImageImportError(
            "The selected file is not a valid JPG image.");
    }

    const auto projectDirectory = targetProjectFile.getParentDirectory();
    if ((!projectDirectory.exists() && !projectDirectory.createDirectory()) || !projectDirectory.isDirectory())
    {
        return buildBackgroundImageImportError(
            "The project directory could not be created.");
    }

    const auto imagesDirectory = projectDirectory.getChildFile("Images");
    if ((!imagesDirectory.exists() && !imagesDirectory.createDirectory()) || !imagesDirectory.isDirectory())
    {
        return buildBackgroundImageImportError(
            "The project Images directory could not be created.");
    }

    ProjectBackgroundImageImportResult result;
    result.targetFile = imagesDirectory.getChildFile("background.jpg");

    if (sourceImageFile.getFullPathName() != result.targetFile.getFullPathName()
        && result.targetFile.exists()
        && !result.targetFile.deleteFile())
    {
        return buildBackgroundImageImportError(
            "The existing background image could not be replaced.");
    }

    if (sourceImageFile.getFullPathName() != result.targetFile.getFullPathName()
        && !sourceImageFile.copyFileTo(result.targetFile))
    {
        return buildBackgroundImageImportError(
            "The background image could not be copied into the project.");
    }

    result.imported = true;
    return result;
}

ProjectFilesSaveResult saveProjectFiles(const drs::engine::RuntimeProjectModel& project,
                                        const juce::File& projectFile)
{
    return saveProjectFiles(project, projectFile, {});
}

ProjectFilesSaveResult saveProjectFiles(const drs::engine::RuntimeProjectModel& project,
                                        const juce::File& projectFile,
                                        const ProjectFilesSaveOptions& options)
{
    ProjectFilesSaveResult result;
    if (!ensureProjectFolderLayout(projectFile))
    {
        result.errorMessage = "The project folder and its Samples directory could not be created at:\n"
            + projectFile.getParentDirectory().getFullPathName();
        return result;
    }

    const auto files = makeProjectTransactionFiles(projectFile);
    const auto recovery = recoverProjectFilesTransaction(projectFile);
    if (recovery.recoveryNeeded && !recovery.recovered)
    {
        result.errorMessage = recovery.errorMessage;
        return result;
    }
    result.recoveredPreviousGeneration = recovery.recovered;

    const auto instrumentFile = files.instrument;
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

    removeFileIfPresent(files.projectBackup);
    removeFileIfPresent(files.instrumentBackup);
    const auto projectExisted = files.project.existsAsFile();
    const auto instrumentExisted = files.instrument.existsAsFile();
    if ((projectExisted && !files.project.copyFileTo(files.projectBackup))
        || (instrumentExisted && !files.instrument.copyFileTo(files.instrumentBackup)))
    {
        removeFileIfPresent(files.projectBackup);
        removeFileIfPresent(files.instrumentBackup);
        result.errorMessage = "The previous project generation could not be backed up before saving.";
        return result;
    }

    if (!writeTransactionJournal(files, projectExisted, instrumentExisted))
    {
        removeFileIfPresent(files.projectBackup);
        removeFileIfPresent(files.instrumentBackup);
        result.errorMessage = "The recoverable project save journal could not be written to:\n"
            + files.journal.getFullPathName();
        return result;
    }

    const auto rollback = [&result, &files](const juce::String& commitError)
    {
        const auto rollbackResult = recoverProjectFilesTransaction(files.project);
        result.recoveredPreviousGeneration = rollbackResult.recovered;
        result.errorMessage = commitError;
        if (!rollbackResult.recovered)
            result.errorMessage += "\n\nAutomatic recovery failed:\n" + rollbackResult.errorMessage;
    };

    if (!checkpointAllowed(options, ProjectFilesSaveCheckpoint::beforeInstrumentCommit))
    {
        rollback("The project save was interrupted before the instrument commit.");
        return result;
    }

    if (!temporaryInstrument.overwriteTargetFileWithTemporary())
    {
        rollback("The instrument could not be saved to:\n" + instrumentFile.getFullPathName());
        return result;
    }

    if (!checkpointAllowed(options, ProjectFilesSaveCheckpoint::beforeProjectCommit))
    {
        rollback("The project save was interrupted after the instrument commit.");
        return result;
    }

    if (!temporaryProject.overwriteTargetFileWithTemporary())
    {
        rollback("The project could not be saved to:\n" + projectFile.getFullPathName());
        return result;
    }

    if (!removeFileIfPresent(files.journal))
    {
        rollback("The project pair was written, but the save transaction could not be finalized.");
        return result;
    }

    removeFileIfPresent(files.projectBackup);
    removeFileIfPresent(files.instrumentBackup);

    result.saved = true;
    return result;
}
} // namespace drs::app
