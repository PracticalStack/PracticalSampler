#pragma once

#include "shared/SfzImportWorkflow.h"

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

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app
{
enum class SfzImportReviewServiceStage
{
    idle,
    queued,
    analyzing,
    projecting,
    reviewReady,
    canceled,
    failed,
    consumed
};

bool isSfzImportReviewServiceStageTransitionAllowed(
    SfzImportReviewServiceStage from,
    SfzImportReviewServiceStage to) noexcept;

const char* toString(SfzImportReviewServiceStage stage) noexcept;

struct SfzImportReviewRequestIdentity
{
    std::uint64_t ownerId = 0;
    std::uint64_t generation = 0;
    std::string projectId;
    std::size_t baseRevision = 0;
};

struct SfzImportReviewRequest
{
    drs::engine::RuntimeProjectModel baseProject;
    std::string sfzPath;
    std::string projectId;
    std::size_t baseRevision = 0;
};

struct SfzImportReviewSnapshot
{
    SfzImportReviewRequestIdentity identity;
    SfzImportReviewServiceStage stage = SfzImportReviewServiceStage::idle;
    float progress01 = 0.0f;
    std::string status;
    std::shared_ptr<const SfzImportReviewPreparationResult> result;
};

enum class SfzImportReviewSubmitDisposition
{
    accepted,
    busy,
    shuttingDown,
    invalid
};

struct SfzImportReviewSubmitResult
{
    SfzImportReviewSubmitDisposition disposition = SfzImportReviewSubmitDisposition::invalid;
    SfzImportReviewRequestIdentity identity;

    bool wasAccepted() const noexcept
    {
        return disposition == SfzImportReviewSubmitDisposition::accepted;
    }
};

struct SfzImportReviewServiceMetrics
{
    std::size_t requestedCount = 0;
    std::size_t completedCount = 0;
    std::size_t canceledCount = 0;
    std::size_t failedCount = 0;
    std::size_t rejectedBusyCount = 0;
    std::size_t maximumPendingCount = 0;
    std::size_t maximumInFlightCount = 0;
    std::size_t liveWorkerCount = 0;
    std::uint64_t maximumShutdownWaitMicros = 0;
    std::uint64_t shutdownWaitMilliseconds = 0;
    std::chrono::nanoseconds shutdownWaitDuration { 0 };
};

struct SfzImportReviewServiceOptions
{
    std::function<void(SfzImportReviewServiceStage)> stageObserver;
    std::function<void(SfzImportReviewServiceStage)> checkpointObserver;
};

class SfzImportProgressComponent final : public juce::Component
{
public:
    using CancelCallback = std::function<void()>;
    explicit SfzImportProgressComponent(CancelCallback callback = {});
    void update(const SfzImportReviewSnapshot& snapshot);
    void resized() override;
    void setCancelCallback(CancelCallback callback);

private:
    juce::Label statusLabel;
    double progressValue = 0.0;
    juce::ProgressBar progressBar { progressValue };
    juce::TextButton cancelButton { "Cancel" };
    CancelCallback cancelCallback;
};

class SfzImportReviewService
{
public:
    class Client
    {
    public:
        Client() = default;
        ~Client();

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&& other) noexcept;
        Client& operator=(Client&& other) noexcept;

        SfzImportReviewSubmitResult submit(SfzImportReviewRequest request);
        bool cancel(std::string reason = "SFZ import canceled");
        bool waitForTerminal(std::chrono::milliseconds timeout) const;
        bool waitForTerminal() const;
        std::shared_ptr<const SfzImportReviewSnapshot> getSnapshot() const;
        bool consume();
        std::uint64_t ownerId() const noexcept { return owner; }
        std::uint64_t generation() const noexcept { return activeGeneration; }

    private:
        friend class SfzImportReviewService;
        Client(SfzImportReviewService* service, std::uint64_t ownerId) noexcept;
        void reset() noexcept;

        SfzImportReviewService* service = nullptr;
        std::uint64_t owner = 0;
        std::uint64_t activeGeneration = 0;
    };

    explicit SfzImportReviewService(SfzImportReviewServiceOptions options = {});
    ~SfzImportReviewService();

    SfzImportReviewService(const SfzImportReviewService&) = delete;
    SfzImportReviewService& operator=(const SfzImportReviewService&) = delete;

    Client openClient();
    SfzImportReviewServiceMetrics getMetrics() const;
    SfzImportReviewServiceMetrics metricsSnapshot() const { return getMetrics(); }
    std::shared_ptr<const SfzImportReviewSnapshot> getSnapshot() const;
    std::shared_ptr<const SfzImportReviewSnapshot> getSnapshot(std::uint64_t ownerId,
                                                               std::uint64_t generation) const;
    bool cancel(std::uint64_t ownerId, std::uint64_t generation, std::string reason);
    bool waitForTerminal(std::uint64_t ownerId,
                         std::uint64_t generation,
                         std::chrono::milliseconds timeout) const;
    bool consume(std::uint64_t ownerId, std::uint64_t generation);
    void shutdown() noexcept;

private:
    struct PendingRequest
    {
        SfzImportReviewRequestIdentity identity;
        SfzImportReviewRequest request;
        std::shared_ptr<std::atomic<bool>> cancellation;
        std::string cancellationReason;
    };

    void runWorker();
    void process(PendingRequest pending);
    SfzImportReviewSubmitResult submit(std::uint64_t ownerId, SfzImportReviewRequest request);
    bool isCurrentLocked(const SfzImportReviewRequestIdentity& identity) const noexcept;
    bool isTerminal(SfzImportReviewServiceStage stage) const noexcept;
    std::string cancellationReason(const SfzImportReviewRequestIdentity& identity) const;
    void publish(SfzImportReviewRequestIdentity identity,
                 SfzImportReviewServiceStage stage,
                 float progress01,
                 std::string status,
                 std::shared_ptr<const SfzImportReviewPreparationResult> result = {});

    SfzImportReviewServiceOptions options;
    std::atomic<std::uint64_t> nextOwnerId { 1 };
    std::uint64_t nextGeneration = 0;
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable std::condition_variable terminalCondition;
    std::optional<PendingRequest> pending;
    std::optional<PendingRequest> active;
    std::shared_ptr<const SfzImportReviewSnapshot> snapshot;
    SfzImportReviewServiceMetrics metrics;
    bool shutdownRequested = false;
    mutable std::mutex shutdownMutex;
    std::thread worker;
};
} // namespace drs::app
