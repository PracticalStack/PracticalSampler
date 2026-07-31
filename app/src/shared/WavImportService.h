#pragma once

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

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app
{
enum class WavImportBatchStage
{
    idle,
    queued,
    staging,
    inspecting,
    completed,
    canceled,
    superseded,
    failed,
    consumed
};

bool isWavImportBatchStageTransitionAllowed(WavImportBatchStage from,
                                            WavImportBatchStage to) noexcept;
const char* toString(WavImportBatchStage stage) noexcept;

enum class WavImportTerminalDisposition
{
    none,
    completed,
    partiallyCompleted,
    canceled,
    superseded,
    failed,
    consumed
};

enum class WavImportItemStage
{
    pending,
    staging,
    fingerprinting,
    inspecting,
    ready,
    failed,
    canceled,
    skipped
};

struct WavImportRequestIdentity
{
    std::uint64_t ownerId = 0;
    std::uint64_t generation = 0;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string contentRootPath;
    std::string selectedGroupId;
};

struct WavImportRequest
{
    std::vector<std::string> sourcePaths;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string contentRootPath;
    std::string selectedGroupId;
};

struct WavImportItemProgress
{
    std::string itemId;
    std::string sourcePath;
    std::string stagedPath;
    WavImportItemStage stage = WavImportItemStage::pending;
    std::uint64_t bytesProcessed = 0;
    std::uint64_t totalBytes = 0;
    std::uint64_t fingerprintBytesProcessed = 0;
    std::uint64_t fingerprintTotalBytes = 0;
    std::uint64_t copyDurationMicros = 0;
    std::uint64_t fingerprintDurationMicros = 0;
    std::uint64_t inspectionDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
    std::size_t warningCount = 0;
    std::size_t issueCount = 0;
    std::string status;
};

struct WavImportCompletionItem
{
    std::string itemId;
    std::string sourcePath;
    std::string stagedPath;
    std::string finalPath;
    WavImportItemStage stage = WavImportItemStage::pending;
    std::uint64_t sourceBytes = 0;
    std::uint64_t copiedBytes = 0;
    std::uint64_t copyDurationMicros = 0;
    std::uint64_t fingerprintDurationMicros = 0;
    std::uint64_t inspectionDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
    drs::engine::SampleSourceFingerprintResult fingerprint;
    drs::engine::SampleInspectionResult inspection;
    std::vector<drs::engine::SampleFilenameToken> filenameTokens;
    std::vector<drs::engine::AuthoringImportFinding> findings;
    drs::engine::AuthoringImportZoneSuggestion suggestedZone;
};

struct WavImportCompletionPayload
{
    WavImportRequestIdentity identity;
    WavImportTerminalDisposition disposition = WavImportTerminalDisposition::none;
    std::string status;
    std::size_t totalItemCount = 0;
    std::size_t successfulItemCount = 0;
    std::size_t failedItemCount = 0;
    std::size_t canceledItemCount = 0;
    std::size_t warningItemCount = 0;
    std::uint64_t totalBytesProcessed = 0;
    std::uint64_t totalBytesExpected = 0;
    std::uint64_t copyDurationMicros = 0;
    std::uint64_t fingerprintDurationMicros = 0;
    std::uint64_t inspectionDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
    std::vector<WavImportCompletionItem> items;
};

struct WavImportBatchSnapshot
{
    WavImportRequestIdentity identity;
    WavImportBatchStage stage = WavImportBatchStage::idle;
    WavImportTerminalDisposition terminalDisposition = WavImportTerminalDisposition::none;
    std::string status;
    std::size_t totalItemCount = 0;
    std::size_t completedItemCount = 0;
    std::size_t successfulItemCount = 0;
    std::size_t failedItemCount = 0;
    std::size_t canceledItemCount = 0;
    std::size_t warningItemCount = 0;
    std::uint64_t totalBytesProcessed = 0;
    std::uint64_t totalBytesExpected = 0;
    std::uint64_t copyDurationMicros = 0;
    std::uint64_t fingerprintDurationMicros = 0;
    std::uint64_t inspectionDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
    std::vector<WavImportItemProgress> items;
    std::shared_ptr<const WavImportCompletionPayload> completion;
};

enum class WavImportSubmitDisposition
{
    accepted,
    busy,
    shuttingDown,
    invalid
};

struct WavImportSubmitResult
{
    WavImportSubmitDisposition disposition = WavImportSubmitDisposition::invalid;
    WavImportRequestIdentity identity;

    bool wasAccepted() const noexcept
    {
        return disposition == WavImportSubmitDisposition::accepted;
    }
};

struct WavImportServiceMetrics
{
    std::size_t requestedCount = 0;
    std::size_t completedCount = 0;
    std::size_t canceledCount = 0;
    std::size_t failedCount = 0;
    std::size_t consumedCount = 0;
    std::size_t rejectedBusyCount = 0;
    std::size_t maximumPendingCount = 0;
    std::size_t maximumInFlightCount = 0;
    std::size_t liveWorkerCount = 0;
    std::uint64_t lastTerminalGeneration = 0;
    std::uint64_t lastCopyDurationMicros = 0;
    std::uint64_t lastFingerprintDurationMicros = 0;
    std::uint64_t lastInspectionDurationMicros = 0;
    std::uint64_t lastBatchDurationMicros = 0;
    std::uint64_t averageBatchDurationMicros = 0;
    std::uint64_t maxBatchDurationMicros = 0;
    std::uint64_t maximumShutdownWaitMicros = 0;
    std::uint64_t shutdownWaitMilliseconds = 0;
    std::chrono::nanoseconds shutdownWaitDuration { 0 };
};

struct WavImportServiceOptions
{
    std::size_t copyChunkBytes = 64 * 1024;
    std::size_t fingerprintChunkBytes = 4096;
    std::function<void(WavImportBatchStage)> stageObserver;
    std::function<void(WavImportBatchStage)> checkpointObserver;
};

class WavImportProgressComponent final : public juce::Component
{
public:
    using CancelCallback = std::function<void()>;
    explicit WavImportProgressComponent(CancelCallback callback = {});
    void update(const WavImportBatchSnapshot& snapshot);
    void resized() override;
    void setCancelCallback(CancelCallback callback);

private:
    juce::Label statusLabel;
    juce::Label detailLabel;
    double progressValue = 0.0;
    juce::ProgressBar progressBar { progressValue };
    juce::TextButton cancelButton { "Cancel" };
    CancelCallback cancelCallback;
};

class WavImportService
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

        WavImportSubmitResult submit(WavImportRequest request);
        bool cancel(std::string reason = "WAV import canceled");
        bool waitForTerminal(std::chrono::milliseconds timeout) const;
        bool waitForTerminal() const;
        std::shared_ptr<const WavImportBatchSnapshot> getSnapshot() const;
        bool consume();
        std::uint64_t ownerId() const noexcept { return owner; }
        std::uint64_t generation() const noexcept { return activeGeneration; }

    private:
        friend class WavImportService;
        Client(WavImportService* service, std::uint64_t ownerId) noexcept;
        void reset() noexcept;

        WavImportService* service = nullptr;
        std::uint64_t owner = 0;
        std::uint64_t activeGeneration = 0;
    };

    explicit WavImportService(WavImportServiceOptions options = {});
    ~WavImportService();

    WavImportService(const WavImportService&) = delete;
    WavImportService& operator=(const WavImportService&) = delete;

    Client openClient();
    WavImportServiceMetrics getMetrics() const;
    WavImportServiceMetrics metricsSnapshot() const { return getMetrics(); }
    std::shared_ptr<const WavImportBatchSnapshot> getSnapshot() const;
    std::shared_ptr<const WavImportBatchSnapshot> getSnapshot(std::uint64_t ownerId,
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
        WavImportRequestIdentity identity;
        WavImportRequest request;
        std::shared_ptr<std::atomic<bool>> cancellation;
        std::string cancellationReason;
    };

    void runWorker();
    void process(PendingRequest pending);
    WavImportSubmitResult submit(std::uint64_t ownerId, WavImportRequest request);
    bool isTerminal(WavImportBatchStage stage) const noexcept;
    std::string cancellationReason(const WavImportRequestIdentity& identity) const;
    void publish(WavImportRequestIdentity identity,
                 WavImportBatchStage stage,
                 WavImportTerminalDisposition terminalDisposition,
                 std::string status,
                 std::vector<WavImportItemProgress> items = {},
                 std::shared_ptr<const WavImportCompletionPayload> completion = {});

    WavImportServiceOptions options;
    std::atomic<std::uint64_t> nextOwnerId { 1 };
    std::uint64_t nextGeneration = 0;
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable std::condition_variable terminalCondition;
    std::optional<PendingRequest> pending;
    std::optional<PendingRequest> active;
    std::shared_ptr<const WavImportBatchSnapshot> snapshot;
    WavImportServiceMetrics metrics;
    bool shutdownRequested = false;
    mutable std::mutex shutdownMutex;
    std::thread worker;
};
} // namespace drs::app
