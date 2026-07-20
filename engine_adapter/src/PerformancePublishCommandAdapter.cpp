#include "drs/engine/PerformancePublishCommandAdapter.h"

namespace drs::engine
{
PerformancePublishCommandDispatch PerformancePublishCommandAdapter::dispatch(
    const PerformancePublishCommand& command,
    PerformancePublishCommandSource source) noexcept
{
    PerformancePublishCommandDispatch result;
    result.command = command;
    result.source = source;
    snapshot.lastSource = source;
    if (command.type != PerformancePublishCommandType::publishCurrentDraft)
    {
        ++snapshot.rejectedCommandCount;
        result.rejectionCode = "publish-command-type-invalid";
        return result;
    }

    result.accepted = true;
    ++snapshot.acceptedCommandCount;
    return result;
}

void PerformancePublishCommandAdapter::recordExecutionResult(bool accepted) noexcept
{
    if (accepted)
        ++snapshot.executionAcceptedCount;
    else
        ++snapshot.executionRejectedCount;
}
} // namespace drs::engine
