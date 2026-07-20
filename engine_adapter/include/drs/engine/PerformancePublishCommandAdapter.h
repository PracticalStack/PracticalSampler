#pragma once

#include "drs/engine/PerformancePublishContract.h"

#include <cstdint>

namespace drs::engine
{
enum class PerformancePublishCommandSource : std::uint8_t
{
    authoringWorkspace = 0,
    statusPanel,
    externalApi
};

struct PerformancePublishCommandDispatch
{
    bool accepted = false;
    PerformancePublishCommand command;
    PerformancePublishCommandSource source = PerformancePublishCommandSource::externalApi;
    const char* rejectionCode = nullptr;
};

struct PerformancePublishCommandAdapterSnapshot
{
    std::uint64_t acceptedCommandCount = 0;
    std::uint64_t rejectedCommandCount = 0;
    std::uint64_t executionAcceptedCount = 0;
    std::uint64_t executionRejectedCount = 0;
    PerformancePublishCommandSource lastSource = PerformancePublishCommandSource::externalApi;
};

class PerformancePublishCommandAdapter final
{
public:
    PerformancePublishCommandDispatch dispatch(
        const PerformancePublishCommand& command,
        PerformancePublishCommandSource source) noexcept;
    void recordExecutionResult(bool accepted) noexcept;
    PerformancePublishCommandAdapterSnapshot getSnapshot() const noexcept { return snapshot; }

private:
    PerformancePublishCommandAdapterSnapshot snapshot;
};
} // namespace drs::engine
