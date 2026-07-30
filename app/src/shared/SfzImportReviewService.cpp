#include "shared/SfzImportReviewService.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace drs::app
{
namespace
{
bool isTerminalStage(const SfzImportReviewServiceStage stage) noexcept
{
    return stage == SfzImportReviewServiceStage::canceled
        || stage == SfzImportReviewServiceStage::failed
        || stage == SfzImportReviewServiceStage::reviewReady
        || stage == SfzImportReviewServiceStage::consumed;
}
}

bool isSfzImportReviewServiceStageTransitionAllowed(const SfzImportReviewServiceStage from,
                                                    const SfzImportReviewServiceStage to) noexcept
{
    using Stage = SfzImportReviewServiceStage;
    switch (from)
    {
        case Stage::idle: return to == Stage::queued;
        case Stage::queued: return to == Stage::analyzing || to == Stage::canceled || to == Stage::failed;
        case Stage::analyzing: return to == Stage::projecting || to == Stage::canceled || to == Stage::failed;
        case Stage::projecting: return to == Stage::reviewReady || to == Stage::canceled || to == Stage::failed;
        case Stage::reviewReady: return to == Stage::consumed || to == Stage::canceled || to == Stage::queued;
        case Stage::canceled:
        case Stage::failed:
        case Stage::consumed: return to == Stage::idle || to == Stage::queued;
    }
    return false;
}

const char* toString(const SfzImportReviewServiceStage stage) noexcept
{
    using Stage = SfzImportReviewServiceStage;
    switch (stage)
    {
        case Stage::idle: return "idle";
        case Stage::queued: return "queued";
        case Stage::analyzing: return "analyzing";
        case Stage::projecting: return "projecting";
        case Stage::reviewReady: return "reviewReady";
        case Stage::canceled: return "canceled";
        case Stage::failed: return "failed";
        case Stage::consumed: return "consumed";
    }
    return "failed";
}

SfzImportProgressComponent::SfzImportProgressComponent(CancelCallback callback)
    : cancelCallback(std::move(callback))
{
    setComponentID("sfzImportReviewProgress");
    statusLabel.setComponentID("sfzImportReviewProgressLabel");
    statusLabel.setText("Preparing SFZ import…", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);
    progressBar.setComponentID("sfzImportReviewProgressBar");
    addAndMakeVisible(progressBar);
    cancelButton.setComponentID("sfzImportReviewProgressCancelButton");
    cancelButton.onClick = [this]
    {
        if (cancelCallback)
            cancelCallback();
    };
    addAndMakeVisible(cancelButton);
}

void SfzImportProgressComponent::setCancelCallback(CancelCallback callback)
{
    cancelCallback = std::move(callback);
}

void SfzImportProgressComponent::update(const SfzImportReviewSnapshot& snapshot)
{
    progressValue = snapshot.progress01;
    statusLabel.setText(juce::String(snapshot.status), juce::dontSendNotification);
    cancelButton.setEnabled(snapshot.stage == SfzImportReviewServiceStage::queued
                            || snapshot.stage == SfzImportReviewServiceStage::analyzing
                            || snapshot.stage == SfzImportReviewServiceStage::projecting);
    setVisible(cancelButton.isEnabled());
}

void SfzImportProgressComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    statusLabel.setBounds(area.removeFromTop(22));
    cancelButton.setBounds(area.removeFromRight(80));
    area.removeFromRight(8);
    progressBar.setBounds(area.reduced(0, 4));
}

SfzImportReviewService::Client::Client(SfzImportReviewService* serviceIn,
                                       const std::uint64_t ownerId) noexcept
    : service(serviceIn), owner(ownerId)
{
}

SfzImportReviewService::Client::~Client()
{
    reset();
}

SfzImportReviewService::Client::Client(Client&& other) noexcept
    : service(other.service), owner(other.owner), activeGeneration(other.activeGeneration)
{
    other.service = nullptr;
    other.owner = 0;
    other.activeGeneration = 0;
}

SfzImportReviewService::Client& SfzImportReviewService::Client::operator=(Client&& other) noexcept
{
    if (this != &other)
    {
        reset();
        service = other.service;
        owner = other.owner;
        activeGeneration = other.activeGeneration;
        other.service = nullptr;
        other.owner = 0;
        other.activeGeneration = 0;
    }
    return *this;
}

void SfzImportReviewService::Client::reset() noexcept
{
    if (service != nullptr && activeGeneration != 0)
    {
        service->cancel(owner, activeGeneration, "SFZ import owner closed");
        // Client lifetime is an ownership barrier: returning early here would
        // allow the shell to release state while the processor-owned worker
        // still references the request context.
        service->waitForTerminal(owner, activeGeneration, std::chrono::milliseconds::max());
    }
    service = nullptr;
    owner = 0;
    activeGeneration = 0;
}

SfzImportReviewSubmitResult SfzImportReviewService::Client::submit(SfzImportReviewRequest request)
{
    if (service == nullptr)
        return {};

    const auto result = service->submit(owner, std::move(request));
    if (result.disposition == SfzImportReviewSubmitDisposition::accepted)
        activeGeneration = result.identity.generation;
    return result;
}

bool SfzImportReviewService::Client::cancel(std::string reason)
{
    return service != nullptr && activeGeneration != 0
        && service->cancel(owner, activeGeneration, std::move(reason));
}

bool SfzImportReviewService::Client::waitForTerminal(const std::chrono::milliseconds timeout) const
{
    return service != nullptr && activeGeneration != 0
        && service->waitForTerminal(owner, activeGeneration, timeout);
}

bool SfzImportReviewService::Client::waitForTerminal() const
{
    return waitForTerminal(std::chrono::milliseconds::max());
}

std::shared_ptr<const SfzImportReviewSnapshot> SfzImportReviewService::Client::getSnapshot() const
{
    return service == nullptr ? nullptr : service->getSnapshot(owner, activeGeneration);
}

bool SfzImportReviewService::Client::consume()
{
    return service != nullptr && activeGeneration != 0
        && service->consume(owner, activeGeneration);
}

SfzImportReviewService::SfzImportReviewService(SfzImportReviewServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const SfzImportReviewSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    metrics.liveWorkerCount = 1;
    worker = std::thread([this] { runWorker(); });
}

SfzImportReviewService::~SfzImportReviewService()
{
    shutdown();
}

SfzImportReviewService::Client SfzImportReviewService::openClient()
{
    return Client(this, nextOwnerId.fetch_add(1, std::memory_order_relaxed));
}

SfzImportReviewSubmitResult SfzImportReviewService::submit(const std::uint64_t ownerId,
                                                           SfzImportReviewRequest request)
{
    SfzImportReviewSubmitResult result;
    if (request.sfzPath.empty() || request.projectId.empty())
    {
        result.disposition = SfzImportReviewSubmitDisposition::invalid;
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
        {
            result.disposition = SfzImportReviewSubmitDisposition::shuttingDown;
            return result;
        }
        const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (pending.has_value() || active.has_value()
            || (currentSnapshot && currentSnapshot->stage == SfzImportReviewServiceStage::reviewReady))
        {
            ++metrics.rejectedBusyCount;
            result.disposition = SfzImportReviewSubmitDisposition::busy;
            return result;
        }

        result.disposition = SfzImportReviewSubmitDisposition::accepted;
        result.identity.ownerId = ownerId;
        result.identity.generation = ++nextGeneration;
        result.identity.projectId = request.projectId;
        result.identity.baseRevision = request.baseRevision;
        pending = PendingRequest { result.identity,
                                   std::move(request),
                                   std::make_shared<std::atomic<bool>>(false),
                                   {} };
        ++metrics.requestedCount;
        metrics.maximumPendingCount = std::max<std::size_t>(metrics.maximumPendingCount, 1);
        const auto queued = std::make_shared<SfzImportReviewSnapshot>(
            SfzImportReviewSnapshot { result.identity,
                                      SfzImportReviewServiceStage::queued,
                                      0.0f,
                                      "SFZ review queued",
                                      {} });
         std::atomic_store_explicit(&snapshot,
                                    std::shared_ptr<const SfzImportReviewSnapshot>(queued),
                                    std::memory_order_release);
    }
    condition.notify_one();
    return result;
}

SfzImportReviewServiceMetrics SfzImportReviewService::getMetrics() const
{
    std::lock_guard<std::mutex> lock(mutex);
    auto copy = metrics;
    copy.shutdownWaitDuration = std::chrono::microseconds(copy.maximumShutdownWaitMicros);
    copy.shutdownWaitMilliseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(copy.shutdownWaitDuration).count());
    return copy;
}

std::shared_ptr<const SfzImportReviewSnapshot> SfzImportReviewService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

std::shared_ptr<const SfzImportReviewSnapshot> SfzImportReviewService::getSnapshot(
    const std::uint64_t ownerId,
    const std::uint64_t generation) const
{
    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    if (!current || current->identity.ownerId != ownerId
        || (generation != 0 && current->identity.generation != generation))
        return nullptr;
    return current;
}

bool SfzImportReviewService::cancel(const std::uint64_t ownerId,
                                    const std::uint64_t generation,
                                    std::string reason)
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto matches = [&](const auto& request)
    {
        return request.has_value() && request->identity.ownerId == ownerId
            && request->identity.generation == generation;
    };
    if (matches(pending))
    {
        pending->cancellationReason = std::move(reason);
        pending->cancellation->store(true, std::memory_order_release);
        return true;
    }
    if (matches(active))
    {
        active->cancellationReason = std::move(reason);
        active->cancellation->store(true, std::memory_order_release);
        return true;
    }
    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    return current && current->identity.ownerId == ownerId
        && current->identity.generation == generation
        && !isTerminalStage(current->stage);
}

bool SfzImportReviewService::waitForTerminal(const std::uint64_t ownerId,
                                             const std::uint64_t generation,
                                             const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        return shutdownRequested || (current && current->identity.ownerId == ownerId
            && current->identity.generation == generation && isTerminalStage(current->stage));
    });
}

bool SfzImportReviewService::consume(const std::uint64_t ownerId, const std::uint64_t generation)
{
    std::shared_ptr<const SfzImportReviewSnapshot> current;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (!currentSnapshot || currentSnapshot->identity.ownerId != ownerId
            || currentSnapshot->identity.generation != generation
            || currentSnapshot->stage != SfzImportReviewServiceStage::reviewReady)
            return false;
        current = currentSnapshot;
    }
    publish(current->identity, SfzImportReviewServiceStage::consumed, current->progress01,
            "SFZ review result consumed", current->result);
    return true;
}

void SfzImportReviewService::shutdown() noexcept
{
    std::lock_guard<std::mutex> shutdownLock(shutdownMutex);
    const auto started = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
        {
            // The join below remains safe and makes shutdown idempotent even if
            // the first caller is the destructor path.
        }
        shutdownRequested = true;
        if (pending)
            pending->cancellation->store(true, std::memory_order_release);
        if (active)
            active->cancellation->store(true, std::memory_order_release);
    }
    condition.notify_all();
    terminalCondition.notify_all();
    if (worker.joinable() && worker.get_id() != std::this_thread::get_id())
        worker.join();
    std::lock_guard<std::mutex> lock(mutex);
    metrics.liveWorkerCount = 0;
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto micros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    metrics.maximumShutdownWaitMicros = std::max(metrics.maximumShutdownWaitMicros, micros);
}

bool SfzImportReviewService::isCurrentLocked(const SfzImportReviewRequestIdentity& identity) const noexcept
{
    return !shutdownRequested && active.has_value()
        && active->identity.ownerId == identity.ownerId
        && active->identity.generation == identity.generation;
}

bool SfzImportReviewService::isTerminal(const SfzImportReviewServiceStage stage) const noexcept
{
    return isTerminalStage(stage);
}

std::string SfzImportReviewService::cancellationReason(
    const SfzImportReviewRequestIdentity& identity) const
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto matches = [&identity](const auto& request)
    {
        return request.has_value() && request->identity.ownerId == identity.ownerId
            && request->identity.generation == identity.generation;
    };
    if (matches(active))
        return active->cancellationReason;
    if (matches(pending))
        return pending->cancellationReason;
    return {};
}

void SfzImportReviewService::publish(SfzImportReviewRequestIdentity identity,
                                     const SfzImportReviewServiceStage stage,
                                     const float progress01,
                                     std::string status,
                                     std::shared_ptr<const SfzImportReviewPreparationResult> result)
{
    SfzImportReviewServiceOptions localOptions;
    SfzImportReviewServiceStage publishedStage = SfzImportReviewServiceStage::idle;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        const auto previous = current ? current->stage : SfzImportReviewServiceStage::idle;
        if (current && current->identity.generation == identity.generation
            && previous != stage
            && !isSfzImportReviewServiceStageTransitionAllowed(previous, stage))
            return;
        auto next = std::make_shared<SfzImportReviewSnapshot>();
        next->identity = std::move(identity);
        next->stage = stage;
        next->progress01 = std::clamp(progress01, 0.0f, 1.0f);
        next->status = std::move(status);
        next->result = std::move(result);
        publishedStage = next->stage;
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const SfzImportReviewSnapshot>(std::move(next)),
                                   std::memory_order_release);
        if (stage == SfzImportReviewServiceStage::canceled)
            ++metrics.canceledCount;
        else if (stage == SfzImportReviewServiceStage::failed)
            ++metrics.failedCount;
        else if (stage == SfzImportReviewServiceStage::reviewReady)
            ++metrics.completedCount;
        localOptions = options;
    }
    terminalCondition.notify_all();
    try
    {
        if (localOptions.stageObserver)
            localOptions.stageObserver(publishedStage);
    }
    catch (...)
    {
        // Observer hooks are diagnostics/checkpoints. They cannot be allowed
        // to unwind through the owned worker entry point.
    }
    try
    {
        if (localOptions.checkpointObserver)
            localOptions.checkpointObserver(publishedStage);
    }
    catch (...)
    {
    }
}

void SfzImportReviewService::runWorker()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        metrics.liveWorkerCount = 1;
    }
    while (true)
    {
        std::optional<PendingRequest> request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return shutdownRequested || pending.has_value(); });
            if (shutdownRequested && !pending.has_value())
                break;
            request = std::move(pending);
            pending.reset();
            active = request;
            metrics.maximumInFlightCount = std::max<std::size_t>(metrics.maximumInFlightCount, 1);
        }
        if (request)
        {
            try
            {
                process(std::move(*request));
            }
            catch (const std::exception& exception)
            {
                if (request->cancellation->load(std::memory_order_acquire))
                    publish(request->identity,
                            SfzImportReviewServiceStage::canceled,
                            1.0f,
                            "SFZ import canceled");
                else
                    publish(request->identity,
                            SfzImportReviewServiceStage::failed,
                            1.0f,
                            exception.what());
            }
            catch (...)
            {
                publish(request->identity,
                        SfzImportReviewServiceStage::failed,
                        1.0f,
                        "Unexpected SFZ review worker failure");
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            active.reset();
        }
    }
    std::lock_guard<std::mutex> lock(mutex);
    metrics.liveWorkerCount = 0;
}

void SfzImportReviewService::process(PendingRequest pendingRequest)
{
    const auto identity = pendingRequest.identity;
    publish(identity, SfzImportReviewServiceStage::analyzing, 0.05f, "Analyzing SFZ document");

    drs::engine::SfzImportExecutionContext context;
    context.cancellationProbe = [flag = pendingRequest.cancellation]
    {
        return flag->load(std::memory_order_acquire);
    };
    context.cancellationReasonProbe = [flag = pendingRequest.cancellation]
    {
        return flag->load(std::memory_order_acquire)
            ? drs::engine::SfzImportCancellationReason::requested
            : drs::engine::SfzImportCancellationReason::none;
    };
    context.progressSink = [this, identity](const drs::engine::SfzImportStage stage, const float progress)
    {
        if (stage == drs::engine::SfzImportStage::projected || stage == drs::engine::SfzImportStage::reviewReady)
            publish(identity, SfzImportReviewServiceStage::projecting, progress, "Projecting SFZ review");
    };

    auto review = prepareSfzImportReview(pendingRequest.request.baseProject,
                                         pendingRequest.request.sfzPath,
                                         context);
    if (pendingRequest.cancellation->load(std::memory_order_acquire)
        || review.analysis.execution.canceled()
        || review.projection.execution.canceled())
    {
        const auto reason = cancellationReason(identity);
        publish(identity,
                SfzImportReviewServiceStage::canceled,
                review.analysis.execution.canceled() ? 0.5f : 0.0f,
                reason.empty() ? "SFZ import canceled" : reason);
        return;
    }

    auto result = std::make_shared<const SfzImportReviewPreparationResult>(std::move(review));
    if (!result->prepared)
    {
        publish(identity, SfzImportReviewServiceStage::failed, 1.0f,
                result->issues.empty() ? "SFZ review preparation failed" : result->issues.front(), result);
        return;
    }
    publish(identity, SfzImportReviewServiceStage::reviewReady, 1.0f,
            "SFZ review ready", std::move(result));
}
} // namespace drs::app
