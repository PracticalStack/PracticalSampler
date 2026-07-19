#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace drs::plugin
{
enum class RealtimeGuardOperation : std::uint8_t
{
    none = 0,
    allocation,
    deallocation,
    blockingLock,
    wait,
    fileOpen,
    fileRead,
    pathResolution,
    sampleDecode,
    streamDecode,
    largeResourceDestruction,
    finalSharedOwnershipRelease,
    overBudget,
    count
};

struct RealtimeGuardSnapshot
{
    std::size_t allocationCount = 0;
    std::size_t deallocationCount = 0;
    std::size_t blockingLockCount = 0;
    std::size_t waitCount = 0;
    std::size_t fileOpenCount = 0;
    std::size_t fileReadCount = 0;
    std::size_t pathResolutionCount = 0;
    std::size_t sampleDecodeCount = 0;
    std::size_t streamDecodeCount = 0;
    std::size_t largeResourceDestructionCount = 0;
    std::size_t finalSharedOwnershipReleaseCount = 0;
    std::size_t overBudgetCount = 0;

    std::size_t prohibitedOperationCount() const noexcept;
    std::size_t totalFailureCount() const noexcept;
};

struct RealtimeCallbackBudgetProfile
{
    static constexpr std::array<std::uint32_t, 2> supportedSampleRates { 44100, 48000 };
    static constexpr std::size_t minimumBlockSize = 32;
    static constexpr std::size_t maximumBlockSize = 1024;
    static constexpr std::size_t maximumEventsPerBlock = 128;
    static constexpr std::size_t targetPolyphonyPerContext = 24;
    static constexpr std::size_t playbackContextCount = 2;

    static std::uint64_t deadlineMicros(double sampleRate, std::size_t blockSize) noexcept;
    static bool supports(double sampleRate, std::size_t blockSize) noexcept;
};

class RealtimeGuardState
{
public:
    void record(RealtimeGuardOperation operation) noexcept;
    RealtimeGuardSnapshot snapshot() const noexcept;
    void reset() noexcept;

private:
    static constexpr auto operationCount = static_cast<std::size_t>(RealtimeGuardOperation::count);
    std::array<std::atomic<std::size_t>, operationCount> counters {};
};

class ScopedRealtimeAudioThread
{
public:
    explicit ScopedRealtimeAudioThread(RealtimeGuardState& state) noexcept;
    ~ScopedRealtimeAudioThread() noexcept;

    ScopedRealtimeAudioThread(const ScopedRealtimeAudioThread&) = delete;
    ScopedRealtimeAudioThread& operator=(const ScopedRealtimeAudioThread&) = delete;

private:
    RealtimeGuardState* previousState = nullptr;
};

bool isCurrentThreadRealtimeAudio() noexcept;
void recordRealtimeGuardOperation(RealtimeGuardOperation operation) noexcept;
} // namespace drs::plugin
