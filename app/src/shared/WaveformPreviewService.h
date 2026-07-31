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

namespace drs::app
{
enum class WaveformPreviewServiceStage
{
    idle,
    queued,
    building,
    completed,
    canceled,
    superseded,
    failed
};

const char* toString(WaveformPreviewServiceStage stage) noexcept;

struct WaveformPreviewRequestIdentity
{
    std::uint64_t generation = 0;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string contentRootPath;
    std::string sampleSourceId;
    std::string sourcePath;
    std::uint64_t sourceFileSizeBytes = 0;
    std::int64_t sourceModificationTicks = 0;
    std::size_t displayPointCount = 0;
    drs::engine::WaveformPeakChannelReduction channelReduction
        = drs::engine::WaveformPeakChannelReduction::channelExtrema;
    std::string requestStamp;
};

struct WaveformPreviewRequest
{
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string contentRootPath;
    std::string sampleSourceId;
    std::string sourcePath;
    std::uint64_t sourceFileSizeBytes = 0;
    std::int64_t sourceModificationTicks = 0;
    std::size_t displayPointCount = 192;
    std::uint64_t chunkFrameCount = 4096;
    drs::engine::WaveformPeakChannelReduction channelReduction
        = drs::engine::WaveformPeakChannelReduction::channelExtrema;
    std::string requestStamp;
};

struct WaveformPreviewServiceSnapshot
{
    WaveformPreviewRequestIdentity identity;
    WaveformPreviewServiceStage stage = WaveformPreviewServiceStage::idle;
    std::string status;
    std::uint64_t framesProcessed = 0;
    std::uint64_t totalFrames = 0;
    std::size_t pointsCompleted = 0;
    std::size_t totalPointCount = 0;
    std::shared_ptr<const drs::engine::WaveformPeakBuildResult> result;
};

struct WaveformPreviewSubmitResult
{
    bool accepted = false;
    WaveformPreviewRequestIdentity identity;
};

struct WaveformPreviewServiceOptions
{
    const drs::engine::SampleImportHooks* sampleImportHooks = nullptr;
    std::function<void(WaveformPreviewServiceStage)> stageObserver;
};

class WaveformPreviewService
{
public:
    explicit WaveformPreviewService(WaveformPreviewServiceOptions options = {});
    ~WaveformPreviewService();

    WaveformPreviewService(const WaveformPreviewService&) = delete;
    WaveformPreviewService& operator=(const WaveformPreviewService&) = delete;

    WaveformPreviewSubmitResult submit(WaveformPreviewRequest request);
    bool cancel(std::string reason = "Waveform preview canceled");
    std::shared_ptr<const WaveformPreviewServiceSnapshot> getSnapshot() const;
    bool waitForTerminal(std::chrono::milliseconds timeout) const;
    void shutdown() noexcept;

private:
    struct PendingRequest
    {
        WaveformPreviewRequestIdentity identity;
        WaveformPreviewRequest request;
        std::shared_ptr<std::atomic<bool>> cancellation;
        WaveformPreviewServiceStage cancellationStage = WaveformPreviewServiceStage::canceled;
        std::string cancellationReason;
    };

    void runWorker();
    void process(PendingRequest pending);
    void publish(WaveformPreviewServiceSnapshot snapshot);
    bool isTerminal(WaveformPreviewServiceStage stage) const noexcept;

    WaveformPreviewServiceOptions options;
    std::uint64_t nextGeneration = 0;
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable std::condition_variable terminalCondition;
    std::optional<PendingRequest> pending;
    std::optional<PendingRequest> active;
    std::shared_ptr<const WaveformPreviewServiceSnapshot> snapshot;
    bool shutdownRequested = false;
    mutable std::mutex shutdownMutex;
    std::thread worker;
};
} // namespace drs::app
