#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstddef>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimeProjectHistoryEntry
{
    std::string label;
    std::vector<std::string> changedPaths;
};

struct RuntimeProjectDocumentState
{
    std::size_t revision = 0;
    std::size_t savedRevision = 0;
    bool dirty = false;
    std::size_t undoDepth = 0;
    std::size_t redoDepth = 0;
    std::string lastChangeLabel;
};

struct RuntimeProjectDocumentCheckpoint
{
    RuntimeProjectModel project;
    std::size_t revision = 0;
    std::size_t savedRevision = 0;
    bool dirty = false;
    std::string lastChangeLabel;
};

struct RuntimeProjectDocumentCheckpointConstraints
{
    std::string expectedProjectId;
    std::string manifestPath;
};

struct RuntimeProjectDocumentCheckpointValidationResult
{
    bool valid = false;
    std::vector<std::string> issues;
};

struct RuntimeProjectDocumentActionResult
{
    bool applied = false;
    bool requiresHostStateRebuild = false;
    std::string state;
    std::vector<std::string> issues;
    std::vector<std::string> changedPaths;
    RuntimeProjectDocumentState documentState;
};

class RuntimeProjectDocumentController
{
public:
    explicit RuntimeProjectDocumentController(RuntimeProjectModel project);

    const RuntimeProjectModel& getProject() const { return currentProject; }
    const RuntimeProjectDocumentState& getDocumentState() const { return documentState; }
    RuntimeProjectDocumentCheckpoint exportCheckpoint() const;
    RuntimeProjectDocumentActionResult restoreCheckpoint(
        RuntimeProjectDocumentCheckpoint checkpoint,
        RuntimeProjectDocumentCheckpointConstraints constraints = {});

    RuntimeProjectDocumentActionResult commitSnapshot(const RuntimeProjectModel& nextProject,
                                                      const std::string& label,
                                                      const std::vector<std::string>& changedPaths = {});
    RuntimeProjectDocumentActionResult undo();
    RuntimeProjectDocumentActionResult redo();
    void markSaved();

private:
    struct Snapshot
    {
        RuntimeProjectModel project;
        RuntimeProjectHistoryEntry entry;
        std::size_t revision = 0;
    };

    RuntimeProjectDocumentActionResult makeRejectedResult(const std::string& state,
                                                          const std::vector<std::string>& issues) const;
    RuntimeProjectDocumentActionResult makeAppliedResult(const std::string& state) const;
    void syncDocumentState();

    RuntimeProjectModel currentProject;
    RuntimeProjectDocumentState documentState;
    std::vector<Snapshot> undoStack;
    std::vector<Snapshot> redoStack;
};

RuntimeProjectDocumentCheckpointValidationResult validateRuntimeProjectDocumentCheckpoint(
    const RuntimeProjectDocumentCheckpoint& checkpoint,
    const RuntimeProjectDocumentCheckpointConstraints& constraints = {});
} // namespace drs::engine
