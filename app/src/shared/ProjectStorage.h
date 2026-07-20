#pragma once

#include "drs/engine/RuntimeModel.h"

#include <juce_core/juce_core.h>

namespace drs::app
{
juce::File ensureProjectFileExtension(juce::File file);
juce::File getDefaultStudioProjectDirectory();
juce::File makeSelfContainedProjectFile(juce::File selectedFile);
bool ensureProjectFolderLayout(const juce::File& projectFile);

struct ProjectFilesSaveResult
{
    bool saved = false;
    juce::String errorMessage;
};

drs::engine::RuntimeInstrumentModel buildInstrumentManifestForProject(
    const drs::engine::RuntimeProjectModel& project,
    const juce::File& projectFile);
ProjectFilesSaveResult saveProjectFiles(const drs::engine::RuntimeProjectModel& project,
                                        const juce::File& projectFile);
} // namespace drs::app
