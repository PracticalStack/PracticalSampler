#include "shared/WaveformPreviewService.h"

#include <utility>

namespace drs::app
{
const char* toString(const WaveformPreviewServiceStage stage) noexcept
{
    using Stage = WaveformPreviewServiceStage;
    switch (stage)
    {
        case Stage::idle: return "idle";
        case Stage::queued: return "queued";
        case Stage::building: return "building";
        case Stage::completed: return "completed";
        case Stage::canceled: return "canceled";
        case Stage::superseded: return "superseded";
        case Stage::failed: return "failed";
    }
    return "failed";
}

WaveformPreviewService::WaveformPreviewService(WaveformPreviewServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const WaveformPreviewServiceSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    worker = std::thread([this] { runWorker(); });
}

WaveformPreviewService::~WaveformPreviewService()
{
    shutdown();
}

WaveformPreviewSubmitResult WaveformPreviewService::submit(WaveformPreviewRequest request)
{
    WaveformPreviewSubmitResult result;
    if (request.projectId.empty() || request.sampleSourceId.empty() || request.sourcePath.empty())
        return result;

    WaveformPreviewServiceSnapshot supersededPending;
    auto hasSupersededPending = false;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
            return result;

        result.accepted = true;
        result.identity.generation = ++nextGeneration;
        result.identity.projectId = request.projectId;
        result.identity.baseRevision = request.baseRevision;
        result.identity.contentRootPath = request.contentRootPath;
        result.identity.sampleSourceId = request.sampleSourceId;
        result.identity.sourcePath = request.sourcePath;
        result.identity.sourceFileSizeBytes = request.sourceFileSizeBytes;
        result.identity.sourceModificationTicks = request.sourceModificationTicks;
        result.identity.displayPointCount = request.displayPointCount;
        result.identity.channelReduction = request.channelReduction;
        result.identity.requestStamp = request.requestStamp;

        if (pending.has_value())
        {
            supersededPending.identity = pending->identity;
            supersededPending.stage = WaveformPreviewServiceStage::superseded;
            supersededPending.status = "Waveform preview request superseded";
            hasSupersededPending = true;
        }

        pending = PendingRequest { result.identity,
                                   std::move(request),
                                   std::make_shared<std::atomic<bool>>(false),
                                   WaveformPreviewServiceStage::canceled,
                                   {} };

        if (active.has_value())
        {
            active->cancellationStage = WaveformPreviewServiceStage::superseded;
            active->cancellationReason = "Waveform preview request superseded";
            active->cancellation->store(true, std::memory_order_release);
        }
    }

    if (hasSupersededPending)
        publish(std::move(supersededPending));

    WaveformPreviewServiceSnapshot queued;
    queued.identity = result.identity;
    queued.stage = WaveformPreviewServiceStage::queued;
    queued.status = "Waveform preview queued";
    queued.totalPointCount = result.identity.displayPointCount;
    publish(std::move(queued));
    condition.notify_one();
    return result;
}

bool WaveformPreviewService::cancel(std::string reason)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto markCanceled = [&](PendingRequest& request)
    {
        request.cancellationStage = WaveformPreviewServiceStage::canceled;
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

std::shared_ptr<const WaveformPreviewServiceSnapshot> WaveformPreviewService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

bool WaveformPreviewService::waitForTerminal(const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        return current != nullptr
            && isTerminal(current->stage)
            && !pending.has_value()
            && !active.has_value();
    });
}

void WaveformPreviewService::shutdown() noexcept
{
    std::lock_guard<std::mutex> shutdownLock(shutdownMutex);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
            return;

        shutdownRequested = true;
        if (pending.has_value())
        {
            pending->cancellationStage = WaveformPreviewServiceStage::canceled;
            pending->cancellationReason = "Waveform preview service shutting down";
            pending->cancellation->store(true, std::memory_order_release);
        }
        if (active.has_value())
        {
            active->cancellationStage = WaveformPreviewServiceStage::canceled;
            active->cancellationReason = "Waveform preview service shutting down";
            active->cancellation->store(true, std::memory_order_release);
        }
    }

    condition.notify_all();
    terminalCondition.notify_all();
    if (worker.joinable())
        worker.join();
}

void WaveformPreviewService::runWorker()
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

void WaveformPreviewService::process(PendingRequest pendingRequest)
{
    WaveformPreviewServiceSnapshot progress;
    progress.identity = pendingRequest.identity;
    progress.stage = WaveformPreviewServiceStage::building;
    progress.status = "Building waveform preview";
    progress.totalPointCount = pendingRequest.request.displayPointCount;
    publish(progress);

    class BuildCallbacks final : public drs::engine::WaveformPeakBuildCallbacks
    {
    public:
        BuildCallbacks(PendingRequest& requestIn,
                       WaveformPreviewServiceSnapshot& progressIn,
                       WaveformPreviewService* ownerIn) noexcept
            : request(requestIn),
              progress(progressIn),
              owner(ownerIn)
        {
        }

        bool isCancellationRequested() const override
        {
            return request.cancellation->load(std::memory_order_acquire);
        }

        void onProgress(const drs::engine::WaveformPeakBuildProgress& nextProgress) const override
        {
            progress.framesProcessed = nextProgress.framesProcessed;
            progress.totalFrames = nextProgress.totalFrames;
            progress.pointsCompleted = nextProgress.pointsCompleted;
            progress.totalPointCount = nextProgress.totalPointCount;
            owner->publish(progress);
        }

    private:
        PendingRequest& request;
        WaveformPreviewServiceSnapshot& progress;
        WaveformPreviewService* owner = nullptr;
    } callbacks(pendingRequest, progress, this);

    drs::engine::WaveformPeakBuildOptions buildOptions;
    buildOptions.displayPointCount = pendingRequest.request.displayPointCount;
    buildOptions.chunkFrameCount = pendingRequest.request.chunkFrameCount;
    buildOptions.channelReduction = pendingRequest.request.channelReduction;
    buildOptions.callbacks = &callbacks;

    std::optional<drs::engine::ScopedSampleImportHooksOverride> hookScope;
    if (options.sampleImportHooks != nullptr)
        hookScope.emplace(*options.sampleImportHooks);

    const auto built = std::make_shared<const drs::engine::WaveformPeakBuildResult>(
        drs::engine::buildWaveformPeaks(pendingRequest.request.sourcePath, {}, buildOptions));

    WaveformPreviewServiceSnapshot terminal;
    terminal.identity = pendingRequest.identity;
    terminal.result = built;
    terminal.framesProcessed = progress.framesProcessed;
    terminal.totalFrames = progress.totalFrames;
    terminal.pointsCompleted = progress.pointsCompleted;
    terminal.totalPointCount = progress.totalPointCount;

    if (built->built)
    {
        terminal.stage = WaveformPreviewServiceStage::completed;
        terminal.status = "Waveform preview ready";
    }
    else if (built->canceled || pendingRequest.cancellation->load(std::memory_order_acquire))
    {
        terminal.stage = pendingRequest.cancellationStage;
        terminal.status = pendingRequest.cancellationReason.empty()
            ? built->state
            : pendingRequest.cancellationReason;
    }
    else
    {
        terminal.stage = WaveformPreviewServiceStage::failed;
        terminal.status = built->state.empty() ? "Waveform preview unavailable" : built->state;
    }

    publish(std::move(terminal));
}

void WaveformPreviewService::publish(WaveformPreviewServiceSnapshot nextSnapshot)
{
    auto published = std::make_shared<const WaveformPreviewServiceSnapshot>(std::move(nextSnapshot));
    std::atomic_store_explicit(&snapshot,
                               std::shared_ptr<const WaveformPreviewServiceSnapshot>(published),
                               std::memory_order_release);
    if (options.stageObserver)
        options.stageObserver(published->stage);
    if (isTerminal(published->stage))
        terminalCondition.notify_all();
}

bool WaveformPreviewService::isTerminal(const WaveformPreviewServiceStage stage) const noexcept
{
    return stage == WaveformPreviewServiceStage::completed
        || stage == WaveformPreviewServiceStage::canceled
        || stage == WaveformPreviewServiceStage::superseded
        || stage == WaveformPreviewServiceStage::failed;
}
} // namespace drs::app
