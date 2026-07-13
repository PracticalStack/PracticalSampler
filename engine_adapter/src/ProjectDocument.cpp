#include "drs/engine/ProjectDocument.h"

#include "drs/engine/RuntimeLoader.h"

#include <utility>

namespace drs::engine
{
RuntimeProjectDocumentController::RuntimeProjectDocumentController(RuntimeProjectModel project)
    : currentProject(std::move(project))
{
    syncDocumentState();
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

    return makeAppliedResult("Project transaction committed");
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

    return makeAppliedResult("Project undo restored");
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

    return makeAppliedResult("Project redo restored");
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
