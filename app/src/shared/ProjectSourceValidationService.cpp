#include "shared/ProjectSourceValidationService.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace drs::app
{
namespace
{
using Clock = std::chrono::steady_clock;

std::uint64_t elapsedMicros(const Clock::time_point startedAt) noexcept
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startedAt).count());
}
} // namespace

bool isProjectSourceValidationStageTransitionAllowed(const ProjectSourceValidationStage from,
                                                     const ProjectSourceValidationStage to) noexcept
{
    using Stage = ProjectSourceValidationStage;
    switch (from)
    {
        case Stage::idle:
            return to == Stage::queued;
        case Stage::queued:
            return to == Stage::fingerprinting || to == Stage::canceled || to == Stage::failed;
        case Stage::fingerprinting:
            return to == Stage::fingerprinting || to == Stage::inspecting
                || to == Stage::completed || to == Stage::canceled || to == Stage::failed;
        case Stage::inspecting:
            return to == Stage::fingerprinting || to == Stage::inspecting
                || to == Stage::completed || to == Stage::canceled || to == Stage::failed;
        case Stage::completed:
        case Stage::canceled:
        case Stage::failed:
            return to == Stage::idle || to == Stage::queued;
    }
    return false;
}

const char* toString(const ProjectSourceValidationStage stage) noexcept
{
    using Stage = ProjectSourceValidationStage;
    switch (stage)
    {
        case Stage::idle: return "idle";
        case Stage::queued: return "queued";
        case Stage::fingerprinting: return "fingerprinting";
        case Stage::inspecting: return "inspecting";
        case Stage::completed: return "completed";
        case Stage::canceled: return "canceled";
        case Stage::failed: return "failed";
    }
    return "failed";
}

const char* toString(const ProjectSourceValidationItemStage stage) noexcept
{
    using Stage = ProjectSourceValidationItemStage;
    switch (stage)
    {
        case Stage::pending: return "pending";
        case Stage::fingerprinting: return "fingerprinting";
        case Stage::inspecting: return "inspecting";
        case Stage::validated: return "validated";
        case Stage::failed: return "failed";
        case Stage::canceled: return "canceled";
    }
    return "failed";
}

ProjectSourceValidationService::ProjectSourceValidationService(
    ProjectSourceValidationServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const ProjectSourceValidationSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    worker = std::thread([this] { runWorker(); });
}

ProjectSourceValidationService::~ProjectSourceValidationService()
{
    shutdown();
}

ProjectSourceValidationSubmitResult ProjectSourceValidationService::submit(
    ProjectSourceValidationRequest request)
{
    ProjectSourceValidationSubmitResult result;
    if (request.projectId.empty())
        return result;

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested || request.sampleSources.empty())
            return result;
        if (pending.has_value() || active.has_value())
            return result;

        result.accepted = true;
        result.identity.generation = ++nextGeneration;
        result.identity.projectId = request.projectId;
        result.identity.baseRevision = request.baseRevision;
        result.identity.contentRootPath = request.contentRootPath;
        pending = PendingRequest { result.identity,
                                   std::move(request),
                                   std::make_shared<std::atomic<bool>>(false),
                                   {} };
    }

    ProjectSourceValidationSnapshot queued;
    queued.identity = result.identity;
    queued.stage = ProjectSourceValidationStage::queued;
    queued.status = "Project source validation queued";
    queued.totalItemCount = pending->request.sampleSources.size();
    queued.items.reserve(queued.totalItemCount);
    for (const auto& sampleSource : pending->request.sampleSources)
    {
        ProjectSourceValidationItemSnapshot item;
        item.sourceId = sampleSource.id;
        item.sourcePath = sampleSource.path;
        item.status = "Queued";
        queued.items.push_back(std::move(item));
    }
    publish(std::move(queued));
    condition.notify_one();
    return result;
}

bool ProjectSourceValidationService::cancel(std::string reason)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto markCanceled = [&](PendingRequest& request)
    {
        request.cancellationReason = std::move(reason);
        request.cancellation->store(true, std::memory_order_release);
        return true;
    };

    if (active.has_value())
        return markCanceled(*active);
    if (pending.has_value())
        return markCanceled(*pending);
    return false;
}

std::shared_ptr<const ProjectSourceValidationSnapshot>
    ProjectSourceValidationService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

bool ProjectSourceValidationService::waitForTerminal(const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        return current != nullptr && isTerminal(current->stage);
    });
}

void ProjectSourceValidationService::shutdown() noexcept
{
    std::lock_guard<std::mutex> shutdownLock(shutdownMutex);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
            return;
        shutdownRequested = true;
        if (pending.has_value())
        {
            pending->cancellationReason = "Project source validation service shutting down";
            pending->cancellation->store(true, std::memory_order_release);
        }
        if (active.has_value())
        {
            active->cancellationReason = "Project source validation service shutting down";
            active->cancellation->store(true, std::memory_order_release);
        }
    }
    condition.notify_all();
    terminalCondition.notify_all();
    if (worker.joinable())
        worker.join();
}

void ProjectSourceValidationService::runWorker()
{
    for (;;)
    {
        std::optional<PendingRequest> nextRequest;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return shutdownRequested || pending.has_value(); });
            if (shutdownRequested && !pending.has_value())
                return;
            active = std::move(pending);
            pending.reset();
            nextRequest = active;
        }

        if (nextRequest.has_value())
            process(std::move(*nextRequest));

        {
            std::lock_guard<std::mutex> lock(mutex);
            active.reset();
        }
        terminalCondition.notify_all();
    }
}

void ProjectSourceValidationService::process(PendingRequest pendingRequest)
{
    auto progress = ProjectSourceValidationSnapshot {};
    progress.identity = pendingRequest.identity;
    progress.stage = ProjectSourceValidationStage::fingerprinting;
    progress.status = "Fingerprinting project sources";
    progress.totalItemCount = pendingRequest.request.sampleSources.size();
    progress.items.reserve(progress.totalItemCount);
    for (const auto& sampleSource : pendingRequest.request.sampleSources)
    {
        ProjectSourceValidationItemSnapshot item;
        item.sourceId = sampleSource.id;
        item.sourcePath = sampleSource.path;
        item.status = "Queued";
        progress.items.push_back(std::move(item));
    }

    std::optional<drs::engine::ScopedSampleImportHooksOverride> hookScope;
    if (options.sampleImportHooks != nullptr)
        hookScope.emplace(*options.sampleImportHooks);

    const auto batchStartedAt = Clock::now();
    for (std::size_t index = 0; index < pendingRequest.request.sampleSources.size(); ++index)
    {
        const auto& sampleSource = pendingRequest.request.sampleSources[index];
        auto& item = progress.items[index];

        if (pendingRequest.cancellation->load(std::memory_order_acquire))
        {
            item.stage = ProjectSourceValidationItemStage::canceled;
            item.canceled = true;
            item.status = cancellationReason();
            ++progress.canceledItemCount;
            progress.stage = ProjectSourceValidationStage::canceled;
            progress.status = item.status;
            progress.currentSourceId = sampleSource.id;
            progress.currentSourcePath = sampleSource.path;
            progress.totalDurationMicros = elapsedMicros(batchStartedAt);
            publish(std::move(progress));
            return;
        }

        progress.stage = ProjectSourceValidationStage::fingerprinting;
        progress.status = "Fingerprinting project sources";
        progress.currentSourceId = sampleSource.id;
        progress.currentSourcePath = sampleSource.path;
        item.stage = ProjectSourceValidationItemStage::fingerprinting;
        item.status = "Fingerprinting";
        publish(progress);

        class FingerprintCallbacks final : public drs::engine::SampleFingerprintCallbacks
        {
        public:
            FingerprintCallbacks(std::shared_ptr<std::atomic<bool>> cancellationFlag,
                                 ProjectSourceValidationSnapshot& snapshotRef,
                                 ProjectSourceValidationItemSnapshot& itemRef)
                : cancellation(std::move(cancellationFlag)),
                  snapshot(snapshotRef),
                  item(itemRef)
            {
            }

            bool isCancellationRequested() const override
            {
                return cancellation->load(std::memory_order_acquire);
            }

            void onProgress(const drs::engine::SampleFingerprintProgress& progressValue) const override
            {
                item.bytesProcessed = progressValue.bytesProcessed;
                item.totalBytes = progressValue.totalBytes;
                snapshot.totalBytesProcessed = 0;
                snapshot.totalBytesExpected = 0;
                for (const auto& currentItem : snapshot.items)
                {
                    snapshot.totalBytesProcessed += currentItem.bytesProcessed;
                    snapshot.totalBytesExpected += currentItem.totalBytes;
                }
            }

        private:
            std::shared_ptr<std::atomic<bool>> cancellation;
            ProjectSourceValidationSnapshot& snapshot;
            ProjectSourceValidationItemSnapshot& item;
        } callbacks(pendingRequest.cancellation, progress, item);

        const auto fingerprintStartedAt = Clock::now();
        drs::engine::SampleFingerprintOptions fingerprintOptions;
        fingerprintOptions.chunkSizeBytes = options.fingerprintChunkBytes;
        fingerprintOptions.callbacks = &callbacks;
        const auto fingerprint = drs::engine::fingerprintSampleSourceFile(sampleSource.path,
                                                                          fingerprintOptions);
        item.fingerprintDurationMicros = elapsedMicros(fingerprintStartedAt);
        item.totalDurationMicros += item.fingerprintDurationMicros;
        item.fingerprinted = fingerprint.fingerprinted;
        item.issueCount = fingerprint.issues.size();

        if (fingerprint.canceled || pendingRequest.cancellation->load(std::memory_order_acquire))
        {
            item.stage = ProjectSourceValidationItemStage::canceled;
            item.canceled = true;
            item.status = fingerprint.state.empty() ? cancellationReason() : fingerprint.state;
            ++progress.canceledItemCount;
            progress.stage = ProjectSourceValidationStage::canceled;
            progress.status = item.status;
            progress.currentSourceId = sampleSource.id;
            progress.currentSourcePath = sampleSource.path;
            progress.totalDurationMicros = elapsedMicros(batchStartedAt);
            publish(std::move(progress));
            return;
        }

        if (!fingerprint.fingerprinted)
        {
            item.stage = ProjectSourceValidationItemStage::failed;
            item.status = fingerprint.state.empty() ? "Fingerprint failed" : fingerprint.state;
            ++progress.failedItemCount;
            ++progress.completedItemCount;
            progress.totalDurationMicros = elapsedMicros(batchStartedAt);
            publish(progress);
            continue;
        }

        progress.stage = ProjectSourceValidationStage::inspecting;
        progress.status = "Inspecting project sources";
        item.stage = ProjectSourceValidationItemStage::inspecting;
        item.status = "Inspecting";
        publish(progress);

        const auto inspectionStartedAt = Clock::now();
        const auto inspection = drs::engine::inspectSampleFile(sampleSource.path);
        item.inspectionDurationMicros = elapsedMicros(inspectionStartedAt);
        item.totalDurationMicros += item.inspectionDurationMicros;
        item.warningCount = inspection.warnings.size();
        item.issueCount += inspection.issues.size();
        progress.warningItemCount += item.warningCount > 0 ? 1 : 0;

        if (!inspection.accepted)
        {
            item.stage = ProjectSourceValidationItemStage::failed;
            item.status = inspection.state.empty() ? "Inspection failed" : inspection.state;
            ++progress.failedItemCount;
        }
        else
        {
            item.stage = ProjectSourceValidationItemStage::validated;
            item.accepted = true;
            item.status = item.warningCount > 0 ? "Validated with warnings" : "Validated";
            ++progress.successfulItemCount;
        }

        ++progress.completedItemCount;
        progress.totalDurationMicros = elapsedMicros(batchStartedAt);
        publish(progress);
    }

    progress.stage = progress.failedItemCount == 0
        ? ProjectSourceValidationStage::completed
        : ProjectSourceValidationStage::failed;
    progress.status = progress.failedItemCount == 0
        ? "Project source validation complete"
        : "Project source validation finished with failures";
    progress.currentSourceId.clear();
    progress.currentSourcePath.clear();
    progress.totalDurationMicros = elapsedMicros(batchStartedAt);
    publish(std::move(progress));
}

void ProjectSourceValidationService::publish(ProjectSourceValidationSnapshot nextSnapshot)
{
    auto published = std::make_shared<const ProjectSourceValidationSnapshot>(std::move(nextSnapshot));
    std::atomic_store_explicit(&snapshot,
                               std::shared_ptr<const ProjectSourceValidationSnapshot>(published),
                               std::memory_order_release);
    if (options.stageObserver)
        options.stageObserver(published->stage);
    if (options.checkpointObserver)
        options.checkpointObserver(published->stage);
    if (isTerminal(published->stage))
        terminalCondition.notify_all();
}

bool ProjectSourceValidationService::isTerminal(const ProjectSourceValidationStage stage) const noexcept
{
    return stage == ProjectSourceValidationStage::completed
        || stage == ProjectSourceValidationStage::canceled
        || stage == ProjectSourceValidationStage::failed;
}

std::string ProjectSourceValidationService::cancellationReason() const
{
    std::lock_guard<std::mutex> lock(mutex);
    if (active.has_value() && !active->cancellationReason.empty())
        return active->cancellationReason;
    if (pending.has_value() && !pending->cancellationReason.empty())
        return pending->cancellationReason;
    return "Project source validation canceled";
}
} // namespace drs::app
