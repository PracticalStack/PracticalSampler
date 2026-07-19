#include "plugin/RealtimeGuard.h"

#include <algorithm>
#include <cmath>

namespace drs::plugin
{
namespace
{
thread_local RealtimeGuardState* currentRealtimeGuardState = nullptr;

std::size_t loadCounter(const std::array<std::atomic<std::size_t>,
                                         static_cast<std::size_t>(RealtimeGuardOperation::count)>& counters,
                        RealtimeGuardOperation operation) noexcept
{
    return counters[static_cast<std::size_t>(operation)].load(std::memory_order_acquire);
}
} // namespace

std::size_t RealtimeGuardSnapshot::prohibitedOperationCount() const noexcept
{
    return allocationCount + deallocationCount + blockingLockCount + waitCount
        + fileOpenCount + fileReadCount + pathResolutionCount + sampleDecodeCount
        + streamDecodeCount + largeResourceDestructionCount + finalSharedOwnershipReleaseCount;
}

std::size_t RealtimeGuardSnapshot::totalFailureCount() const noexcept
{
    return prohibitedOperationCount() + overBudgetCount;
}

std::uint64_t RealtimeCallbackBudgetProfile::deadlineMicros(double sampleRate,
                                                            std::size_t blockSize) noexcept
{
    if (sampleRate <= 0.0 || blockSize == 0)
        return 0;

    return static_cast<std::uint64_t>(
        std::llround(static_cast<double>(blockSize) * 1000000.0 / sampleRate));
}

bool RealtimeCallbackBudgetProfile::supports(double sampleRate, std::size_t blockSize) noexcept
{
    const auto roundedSampleRate = static_cast<std::uint32_t>(std::llround(sampleRate));
    return std::find(supportedSampleRates.begin(), supportedSampleRates.end(), roundedSampleRate)
            != supportedSampleRates.end()
        && blockSize >= minimumBlockSize
        && blockSize <= maximumBlockSize;
}

void RealtimeGuardState::record(RealtimeGuardOperation operation) noexcept
{
    const auto index = static_cast<std::size_t>(operation);
    if (operation == RealtimeGuardOperation::none || index >= counters.size())
        return;

    counters[index].fetch_add(1, std::memory_order_relaxed);
}

RealtimeGuardSnapshot RealtimeGuardState::snapshot() const noexcept
{
    RealtimeGuardSnapshot result;
    result.allocationCount = loadCounter(counters, RealtimeGuardOperation::allocation);
    result.deallocationCount = loadCounter(counters, RealtimeGuardOperation::deallocation);
    result.blockingLockCount = loadCounter(counters, RealtimeGuardOperation::blockingLock);
    result.waitCount = loadCounter(counters, RealtimeGuardOperation::wait);
    result.fileOpenCount = loadCounter(counters, RealtimeGuardOperation::fileOpen);
    result.fileReadCount = loadCounter(counters, RealtimeGuardOperation::fileRead);
    result.pathResolutionCount = loadCounter(counters, RealtimeGuardOperation::pathResolution);
    result.sampleDecodeCount = loadCounter(counters, RealtimeGuardOperation::sampleDecode);
    result.streamDecodeCount = loadCounter(counters, RealtimeGuardOperation::streamDecode);
    result.largeResourceDestructionCount = loadCounter(counters, RealtimeGuardOperation::largeResourceDestruction);
    result.finalSharedOwnershipReleaseCount = loadCounter(counters, RealtimeGuardOperation::finalSharedOwnershipRelease);
    result.overBudgetCount = loadCounter(counters, RealtimeGuardOperation::overBudget);
    return result;
}

void RealtimeGuardState::reset() noexcept
{
    for (auto& counter : counters)
        counter.store(0, std::memory_order_release);
}

ScopedRealtimeAudioThread::ScopedRealtimeAudioThread(RealtimeGuardState& state) noexcept
    : previousState(currentRealtimeGuardState)
{
    currentRealtimeGuardState = &state;
}

ScopedRealtimeAudioThread::~ScopedRealtimeAudioThread() noexcept
{
    currentRealtimeGuardState = previousState;
}

bool isCurrentThreadRealtimeAudio() noexcept
{
    return currentRealtimeGuardState != nullptr;
}

void recordRealtimeGuardOperation(RealtimeGuardOperation operation) noexcept
{
    if (currentRealtimeGuardState != nullptr)
        currentRealtimeGuardState->record(operation);
}
} // namespace drs::plugin
