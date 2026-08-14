#pragma once

#include "drs/engine/RuntimeModel.h"

#include <juce_core/juce_core.h>

#include <functional>

namespace drs::app
{
juce::File ensureProjectFileExtension(juce::File file);
juce::File getDefaultStudioProjectDirectory();
juce::File makeSelfContainedProjectFile(juce::File selectedFile);
bool ensureProjectFolderLayout(const juce::File& projectFile);

struct ProjectFilesSaveResult
{
    bool saved = false;
    bool recoveredPreviousGeneration = false;
    juce::String errorMessage;
};

enum class ProjectFilesSaveCheckpoint
{
    beforeInstrumentCommit,
    beforeProjectCommit
};

struct ProjectFilesSaveOptions
{
    std::function<bool(ProjectFilesSaveCheckpoint)> allowCommitAtCheckpoint;
};

struct ProjectFilesRecoveryResult
{
    bool recoveryNeeded = false;
    bool recovered = false;
    juce::String errorMessage;
};

struct ProjectBackgroundImageImportResult
{
    bool imported = false;
    juce::File targetFile;
    juce::String errorMessage;
};

struct ProjectLicenseFileImportResult
{
    bool imported = false;
    juce::File targetFile;
    juce::String errorMessage;
};

drs::engine::RuntimeInstrumentModel buildInstrumentManifestForProject(
    const drs::engine::RuntimeProjectModel& project,
    const juce::File& projectFile);
ProjectFilesSaveResult saveProjectFiles(const drs::engine::RuntimeProjectModel& project,
                                        const juce::File& projectFile);
ProjectFilesSaveResult saveProjectFiles(const drs::engine::RuntimeProjectModel& project,
                                        const juce::File& projectFile,
                                        const ProjectFilesSaveOptions& options);
ProjectFilesRecoveryResult recoverProjectFilesTransaction(const juce::File& projectFile);
ProjectBackgroundImageImportResult importProjectBackgroundImage(const juce::File& sourceImageFile,
                                                               const juce::File& projectFile);
ProjectLicenseFileImportResult importProjectLicenseFile(const juce::File& sourceTextFile,
                                                        const juce::File& projectFile);
} // namespace drs::app
