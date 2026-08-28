#pragma once

#include "drs/engine/HostSessionState.h"
#include "drs/engine/ProjectDocument.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace drs::engine
{
struct PreparedPerformancePackageActivationResult;
struct PerformancePackageV3ActivationSecurityContext;
enum class ProjectRestoreState
{
    idle,
    parsing,
    resolving,
    needsLocation,
    loading,
    preparing,
    ready,
    active,
    degraded,
    failed
};

enum class ProjectRestoreFinding
{
    none,
    legacyUnboundProject,
    requestSuperseded,
    requestCanceled,
    invalidHostState,
    candidateMissing,
    identityMismatch,
    contentChanged,
    projectLoadFailed,
    checkpointInvalid,
    projectBindingInvalid,
    performancePackageInvalid,
    presetStateInvalid,
    articulationMismatch,
    draftPlaybackFailed,
    publishSchedulingFailed,
    publishedIdentityMismatch,
    preparationFailed,
    shutdown
};

struct ProjectRestoreRequest
{
    std::string serializedHostState;
    std::string trustedBaseDirectory;
    std::string locatedManifestPath;
};

struct ProjectRestoreSnapshot
{
    std::uint64_t generation = 0;
    ProjectRestoreState state = ProjectRestoreState::idle;
    ProjectRestoreFinding finding = ProjectRestoreFinding::none;
    std::string expectedProjectId;
    std::string resolvedManifestPath;
    std::string message;
    bool restoredFromEmbeddedSnapshot = false;
    bool legacyPresetOnly = false;
    bool performancePackageOnly = false;
    std::optional<HostSessionState> hostState;
    std::optional<RuntimePresetState> legacyPreset;
    std::optional<RuntimeProjectDocumentCheckpoint> checkpoint;
    std::shared_ptr<PreparedPerformancePackageActivationResult> packageActivation;
};

struct ProjectRestoreCoordinatorOptions
{
    std::size_t maximumSiblingEntries = 128;
    std::function<void(std::uint64_t, ProjectRestoreState)> stageObserver;
    std::function<std::shared_ptr<const PerformancePackageV3ActivationSecurityContext>()>
        v3SecurityContextProvider;
};

class ProjectRestoreCoordinator
{
public:
    explicit ProjectRestoreCoordinator(ProjectRestoreCoordinatorOptions options = {});
    ~ProjectRestoreCoordinator();

    ProjectRestoreCoordinator(const ProjectRestoreCoordinator&) = delete;
    ProjectRestoreCoordinator& operator=(const ProjectRestoreCoordinator&) = delete;

    std::uint64_t submit(ProjectRestoreRequest request);
    std::shared_ptr<const ProjectRestoreSnapshot> getSnapshot() const;

    bool publishLifecycleState(std::uint64_t generation,
                               ProjectRestoreState state,
                               ProjectRestoreFinding finding = ProjectRestoreFinding::none,
                               std::string message = {});

    void shutdown();
    bool isWorkerRunning() const noexcept;
    std::uint64_t latestGeneration() const noexcept;

private:
    struct PendingRequest
    {
        std::uint64_t generation = 0;
        ProjectRestoreRequest request;
    };

    void runWorker();
    void processRequest(const PendingRequest& pending);
    bool isCurrent(std::uint64_t generation) const noexcept;
    bool publishSnapshot(ProjectRestoreSnapshot snapshot);

    ProjectRestoreCoordinatorOptions options;
    std::atomic<std::uint64_t> generationCounter { 0 };
    std::atomic<std::uint64_t> currentGeneration { 0 };
    std::atomic<bool> shutdownRequested { false };
    std::atomic<bool> workerRunning { false };
    mutable std::shared_ptr<const ProjectRestoreSnapshot> publishedSnapshot;
    mutable std::mutex requestMutex;
    std::condition_variable requestCondition;
    std::optional<PendingRequest> pendingRequest;
    std::thread worker;
};

const char* toString(ProjectRestoreState state) noexcept;
const char* toString(ProjectRestoreFinding finding) noexcept;
} // namespace drs::engine
