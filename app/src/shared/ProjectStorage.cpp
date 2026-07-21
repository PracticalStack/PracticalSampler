#include "shared/ProjectStorage.h"

#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace drs::app
{
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
    instrument.schemaVersion = 1;
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
        zone.streamOffsetBytes = 0;
        zone.prefetchBytes = 16384;
        zone.releaseSeconds = projectZone.releaseSeconds;
        zone.roundRobinLength = projectZone.roundRobinLength;
        zone.roundRobinPosition = projectZone.roundRobinPosition;
        zone.triggerMode = projectZone.triggerMode;
        instrument.zones.push_back(std::move(zone));
    }

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
