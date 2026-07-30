#include "drs/engine/ProjectDocument.h"

#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string_view>
#include <utility>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

constexpr std::size_t maximumCheckpointPathBytes = 32u * 1024u;

bool isStructurallyValidPath(const std::string& path)
{
    if (path.empty() || path.size() > maximumCheckpointPathBytes)
        return false;

    if (std::any_of(path.begin(),
                    path.end(),
                    [](const unsigned char character)
                    {
                        return character == 0 || character < 0x20;
                    }))
        return false;

    try
    {
        return !fs::u8path(path).lexically_normal().empty();
    }
    catch (const fs::filesystem_error&)
    {
        return false;
    }
}

bool hasProjectManifestExtension(const std::string& path)
{
    try
    {
        auto extension = fs::u8path(path).extension().u8string();
        std::transform(extension.begin(),
                       extension.end(),
                       extension.begin(),
                       [](const unsigned char character)
                       {
                           return static_cast<char>(std::tolower(character));
                       });
        return extension == ".drsproj";
    }
    catch (const fs::filesystem_error&)
    {
        return false;
    }
}

void validateCheckpointPath(RuntimeProjectDocumentCheckpointValidationResult& result,
                            const std::string& path,
                            const std::string_view label)
{
    if (!isStructurallyValidPath(path))
        result.issues.push_back(std::string(label) + " is not a valid bounded filesystem path.");
}
} // namespace

RuntimeProjectDocumentController::RuntimeProjectDocumentController(RuntimeProjectModel project)
    : currentProject(std::move(project))
{
    syncDocumentState();
}

RuntimeProjectDocumentCheckpoint RuntimeProjectDocumentController::exportCheckpoint() const
{
    RuntimeProjectDocumentCheckpoint checkpoint;
    checkpoint.project = currentProject;
    checkpoint.revision = documentState.revision;
    checkpoint.savedRevision = documentState.savedRevision;
    checkpoint.dirty = documentState.dirty;
    checkpoint.lastChangeLabel = documentState.lastChangeLabel;
    return checkpoint;
}

RuntimeProjectDocumentActionResult RuntimeProjectDocumentController::restoreCheckpoint(
    RuntimeProjectDocumentCheckpoint checkpoint,
    RuntimeProjectDocumentCheckpointConstraints constraints)
{
    const auto validation = validateRuntimeProjectDocumentCheckpoint(checkpoint, constraints);
    if (!validation.valid)
        return makeRejectedResult("Project checkpoint rejected", validation.issues);

    currentProject = std::move(checkpoint.project);
    documentState.revision = checkpoint.revision;
    documentState.savedRevision = checkpoint.savedRevision;
    documentState.lastChangeLabel = std::move(checkpoint.lastChangeLabel);
    undoStack.clear();
    redoStack.clear();
    syncDocumentState();
    return makeAppliedResult("Project checkpoint restored");
}

RuntimeProjectDocumentCheckpointValidationResult validateRuntimeProjectDocumentCheckpoint(
    const RuntimeProjectDocumentCheckpoint& checkpoint,
    const RuntimeProjectDocumentCheckpointConstraints& constraints)
{
    RuntimeProjectDocumentCheckpointValidationResult result;
    const auto projectValidation = validateRuntimeProjectModel(checkpoint.project);
    result.issues = projectValidation.issues;

    if (checkpoint.savedRevision > checkpoint.revision)
        result.issues.push_back("Project checkpoint savedRevision must not exceed revision.");

    if (checkpoint.dirty != (checkpoint.revision != checkpoint.savedRevision))
        result.issues.push_back("Project checkpoint dirty must equal revision != savedRevision.");

    if (!constraints.expectedProjectId.empty()
        && checkpoint.project.projectId != constraints.expectedProjectId)
    {
        result.issues.push_back(
            "Project checkpoint projectId does not match the expected host-state project identity.");
    }

    if (!constraints.manifestPath.empty())
    {
        validateCheckpointPath(result, constraints.manifestPath, "Project checkpoint manifestPath");
        if (!hasProjectManifestExtension(constraints.manifestPath))
            result.issues.push_back("Project checkpoint manifestPath must use the .drsproj extension.");
    }

    validateCheckpointPath(result,
                           checkpoint.project.contentRootPath,
                           "Project checkpoint contentRootPath");
    validateCheckpointPath(result,
                           checkpoint.project.defaultInstrumentManifestPath,
                           "Project checkpoint defaultInstrumentManifestPath");

    for (const auto& sampleSource : checkpoint.project.sampleSources)
    {
        validateCheckpointPath(result,
                               sampleSource.path,
                               "Project checkpoint sample source path");
    }

    for (const auto& bank : checkpoint.project.authoring.performanceBanks)
    {
        for (const auto& phraseAsset : bank.phraseAssets)
        {
            validateCheckpointPath(result,
                                   phraseAsset.sourcePath,
                                   "Project checkpoint phrase sourcePath");
        }
    }

    result.valid = result.issues.empty();
    return result;
}

RuntimeProjectDocumentActionResult RuntimeProjectDocumentController::commitSnapshot(
    const RuntimeProjectModel& nextProject,
    const std::string& label,
    const std::vector<std::string>& changedPaths)
{
    if (label.empty())
        return makeRejectedResult("Project transaction rejected", { "Project transaction label must not be empty." });

    const auto validation = validateRuntimeProjectModel(nextProject);
    if (!validation.valid)
        return makeRejectedResult("Project transaction rejected", validation.issues);

    Snapshot snapshot;
    snapshot.project = currentProject;
    snapshot.entry.label = label;
    snapshot.entry.changedPaths = changedPaths;
    snapshot.revision = documentState.revision;
    undoStack.push_back(std::move(snapshot));

    currentProject = nextProject;
    redoStack.clear();
    ++documentState.revision;
    documentState.lastChangeLabel = label;
    syncDocumentState();

    auto result = makeAppliedResult("Project transaction committed");
    result.requiresHostStateRebuild = true;
    result.changedPaths = changedPaths;
    return result;
}

RuntimeProjectDocumentActionResult RuntimeProjectDocumentController::undo()
{
    if (undoStack.empty())
        return makeRejectedResult("Project undo unavailable", { "Project history does not contain an undo checkpoint." });

    auto snapshot = std::move(undoStack.back());
    undoStack.pop_back();

    Snapshot redoSnapshot;
    redoSnapshot.project = currentProject;
    redoSnapshot.entry = snapshot.entry;
    redoSnapshot.revision = documentState.revision;
    redoStack.push_back(std::move(redoSnapshot));

    currentProject = std::move(snapshot.project);
    documentState.revision = snapshot.revision;
    documentState.lastChangeLabel = snapshot.entry.label;
    syncDocumentState();

    auto result = makeAppliedResult("Project undo restored");
    result.requiresHostStateRebuild = true;
    result.changedPaths = snapshot.entry.changedPaths;
    return result;
}

RuntimeProjectDocumentActionResult RuntimeProjectDocumentController::redo()
{
    if (redoStack.empty())
        return makeRejectedResult("Project redo unavailable", { "Project history does not contain a redo checkpoint." });

    auto snapshot = std::move(redoStack.back());
    redoStack.pop_back();

    Snapshot undoSnapshot;
    undoSnapshot.project = currentProject;
    undoSnapshot.entry = snapshot.entry;
    undoSnapshot.revision = documentState.revision;
    undoStack.push_back(std::move(undoSnapshot));

    currentProject = std::move(snapshot.project);
    documentState.revision = snapshot.revision;
    documentState.lastChangeLabel = snapshot.entry.label;
    syncDocumentState();

    auto result = makeAppliedResult("Project redo restored");
    result.requiresHostStateRebuild = true;
    result.changedPaths = snapshot.entry.changedPaths;
    return result;
}

void RuntimeProjectDocumentController::markSaved()
{
    documentState.savedRevision = documentState.revision;
    syncDocumentState();
}

RuntimeProjectDocumentActionResult RuntimeProjectDocumentController::makeRejectedResult(
    const std::string& state,
    const std::vector<std::string>& issues) const
{
    RuntimeProjectDocumentActionResult result;
    result.state = state;
    result.issues = issues;
    result.documentState = documentState;
    return result;
}

RuntimeProjectDocumentActionResult RuntimeProjectDocumentController::makeAppliedResult(const std::string& state) const
{
    RuntimeProjectDocumentActionResult result;
    result.applied = true;
    result.state = state;
    result.documentState = documentState;
    return result;
}

void RuntimeProjectDocumentController::syncDocumentState()
{
    documentState.undoDepth = undoStack.size();
    documentState.redoDepth = redoStack.size();
    documentState.dirty = documentState.revision != documentState.savedRevision;
}
} // namespace drs::engine
