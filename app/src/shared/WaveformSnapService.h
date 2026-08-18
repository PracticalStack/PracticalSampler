#pragma once

#include "drs/engine/WaveformRegionPolicy.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace drs::app
{
enum class WaveformSnapServiceStage : std::uint8_t
{
    idle = 0,
    queued,
    searching,
    completed,
    canceled,
    superseded,
    failed
};

struct WaveformSnapRequest
{
    std::string sourceIdentity;
    std::string sourcePath;
    std::uint64_t candidateFrame = 0;
    std::uint64_t searchRadiusFrames = 2048;
};

struct WaveformSnapRequestIdentity
{
    std::uint64_t generation = 0;
    std::string sourceIdentity;
    std::string sourcePath;
    std::uint64_t candidateFrame = 0;
    std::uint64_t searchRadiusFrames = 0;
};

struct WaveformSnapServiceSnapshot
{
    WaveformSnapRequestIdentity identity;
    WaveformSnapServiceStage stage = WaveformSnapServiceStage::idle;
    drs::engine::WaveformSnapDecision decision;
    bool cacheHit = false;
    std::size_t cacheEntryCount = 0;
    std::string status;
};

struct WaveformSnapSubmitResult
{
    bool accepted = false;
    WaveformSnapRequestIdentity identity;
};

struct WaveformSnapServiceOptions
{
    std::uint64_t maximumSearchRadiusFrames = 16384;
    std::size_t maximumCacheEntries = 128;
    std::function<std::unique_ptr<juce::AudioFormatReader>(const std::string&)> readerFactory;
};

class WaveformSnapService final
{
public:
    explicit WaveformSnapService(WaveformSnapServiceOptions options = {});
    ~WaveformSnapService();

    WaveformSnapService(const WaveformSnapService&) = delete;
    WaveformSnapService& operator=(const WaveformSnapService&) = delete;

    WaveformSnapSubmitResult submit(WaveformSnapRequest request);
    bool cancel(std::string reason = "Zero-crossing search canceled");
    std::shared_ptr<const WaveformSnapServiceSnapshot> getSnapshot() const;
    bool waitForTerminal(std::chrono::milliseconds timeout) const;
    void shutdown() noexcept;

private:
    struct PendingRequest
    {
        WaveformSnapRequestIdentity identity;
        std::shared_ptr<std::atomic<bool>> cancellation;
        WaveformSnapServiceStage cancellationStage = WaveformSnapServiceStage::canceled;
        std::string cancellationReason;
    };

    void runWorker();
    void process(PendingRequest request);
    void publish(WaveformSnapServiceSnapshot snapshot);
    std::unique_ptr<juce::AudioFormatReader> createReader(const std::string& path) const;
    static std::string cacheKey(const WaveformSnapRequestIdentity& identity);
    static bool terminal(WaveformSnapServiceStage stage) noexcept;

    WaveformSnapServiceOptions options;
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable std::condition_variable terminalCondition;
    std::optional<PendingRequest> pending;
    std::optional<PendingRequest> active;
    std::unordered_map<std::string, drs::engine::WaveformSnapDecision> cache;
    std::uint64_t nextGeneration = 0;
    std::shared_ptr<const WaveformSnapServiceSnapshot> snapshot;
    bool shutdownRequested = false;
    std::thread worker;
};
} // namespace drs::app
