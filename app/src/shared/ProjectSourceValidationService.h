#pragma once

#include "drs/engine/RuntimeModel.h"
#include "drs/engine/SampleImport.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace drs::app
{
enum class ProjectSourceValidationStage
{
    idle,
    queued,
    fingerprinting,
    inspecting,
    completed,
    canceled,
    failed
};

bool isProjectSourceValidationStageTransitionAllowed(ProjectSourceValidationStage from,
                                                     ProjectSourceValidationStage to) noexcept;
const char* toString(ProjectSourceValidationStage stage) noexcept;

enum class ProjectSourceValidationItemStage
{
    pending,
    fingerprinting,
    inspecting,
    validated,
    failed,
    canceled
};

const char* toString(ProjectSourceValidationItemStage stage) noexcept;

struct ProjectSourceValidationRequestIdentity
{
    std::uint64_t generation = 0;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string contentRootPath;
};

struct ProjectSourceValidationRequest
{
    std::vector<drs::engine::RuntimeProjectSampleSource> sampleSources;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string contentRootPath;
};

struct ProjectSourceValidationItemSnapshot
{
    std::string sourceId;
    std::string sourcePath;
    ProjectSourceValidationItemStage stage = ProjectSourceValidationItemStage::pending;
    std::string status;
    bool fingerprinted = false;
    bool accepted = false;
    bool canceled = false;
    std::size_t warningCount = 0;
    std::size_t issueCount = 0;
    std::uint64_t bytesProcessed = 0;
    std::uint64_t totalBytes = 0;
    std::uint64_t fingerprintDurationMicros = 0;
    std::uint64_t inspectionDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
};

struct ProjectSourceValidationSnapshot
{
    ProjectSourceValidationRequestIdentity identity;
    ProjectSourceValidationStage stage = ProjectSourceValidationStage::idle;
    std::string status;
    std::size_t totalItemCount = 0;
    std::size_t completedItemCount = 0;
    std::size_t successfulItemCount = 0;
    std::size_t failedItemCount = 0;
    std::size_t canceledItemCount = 0;
    std::size_t warningItemCount = 0;
    std::uint64_t totalBytesProcessed = 0;
    std::uint64_t totalBytesExpected = 0;
    std::uint64_t totalDurationMicros = 0;
    std::string currentSourceId;
    std::string currentSourcePath;
    std::vector<ProjectSourceValidationItemSnapshot> items;
};

struct ProjectSourceValidationSubmitResult
{
    bool accepted = false;
    ProjectSourceValidationRequestIdentity identity;
};

struct ProjectSourceValidationServiceOptions
{
    std::uint64_t fingerprintChunkBytes = 4096;
    const drs::engine::SampleImportHooks* sampleImportHooks = nullptr;
    std::function<void(ProjectSourceValidationStage)> stageObserver;
    std::function<void(ProjectSourceValidationStage)> checkpointObserver;
};

class ProjectSourceValidationService
{
public:
    explicit ProjectSourceValidationService(ProjectSourceValidationServiceOptions options = {});
    ~ProjectSourceValidationService();

    ProjectSourceValidationService(const ProjectSourceValidationService&) = delete;
    ProjectSourceValidationService& operator=(const ProjectSourceValidationService&) = delete;

    ProjectSourceValidationSubmitResult submit(ProjectSourceValidationRequest request);
    bool cancel(std::string reason = "Project source validation canceled");
    std::shared_ptr<const ProjectSourceValidationSnapshot> getSnapshot() const;
    bool waitForTerminal(std::chrono::milliseconds timeout) const;
    void shutdown() noexcept;

private:
    struct PendingRequest
    {
        ProjectSourceValidationRequestIdentity identity;
        ProjectSourceValidationRequest request;
        std::shared_ptr<std::atomic<bool>> cancellation;
        std::string cancellationReason;
    };

    void runWorker();
    void process(PendingRequest pending);
    void publish(ProjectSourceValidationSnapshot snapshot);
    bool isTerminal(ProjectSourceValidationStage stage) const noexcept;
    std::string cancellationReason() const;

    ProjectSourceValidationServiceOptions options;
    std::uint64_t nextGeneration = 0;
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable std::condition_variable terminalCondition;
    std::optional<PendingRequest> pending;
    std::optional<PendingRequest> active;
    std::shared_ptr<const ProjectSourceValidationSnapshot> snapshot;
    bool shutdownRequested = false;
    mutable std::mutex shutdownMutex;
    std::thread worker;
};
} // namespace drs::app
