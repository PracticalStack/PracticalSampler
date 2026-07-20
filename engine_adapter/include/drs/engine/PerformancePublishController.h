#pragma once

#include "drs/engine/PerformancePublishContract.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>

namespace drs::engine
{
struct PerformancePublishControllerConfig
{
    std::size_t maximumCompletionRecords = 32;
};

struct PerformancePublishCompletionRecord
{
    PerformancePublishRequestIdentity identity;
    PerformancePublishPreparationState outcome = PerformancePublishPreparationState::idle;
    std::uint64_t preparedBuildId = 0;
    bool accepted = false;
};

struct PerformancePublishControllerSnapshot
{
    bool hasRequest = false;
    PerformancePublishRequest currentRequest;
    bool hasActiveRequest = false;
    PerformancePublishRequestIdentity activeRequestIdentity;
    std::uint64_t activePreparedBuildId = 0;
    std::string activePreparedDigest;
    std::string activeRouteDigest;
    std::string activeSourceProvenanceDigest;
    std::string activeMacroSchemaDigest;
    bool hasFailedRequest = false;
    PerformancePublishRequestIdentity failedRequestIdentity;
    PerformancePublishFinding failureFinding;
    PerformancePublishPreparationState preparationState = PerformancePublishPreparationState::idle;
    PerformancePublishActivationState activationState = PerformancePublishActivationState::noActivation;
    std::uint64_t acceptedPreparedBuildId = 0;
    std::string acceptedPreparedDigest;
    std::string acceptedRouteDigest;
    std::string acceptedSourceProvenanceDigest;
    std::string acceptedMacroSchemaDigest;
    std::size_t requestedCount = 0;
    std::size_t duplicateSuppressedCount = 0;
    std::size_t launchedCount = 0;
    std::size_t canceledCount = 0;
    std::size_t supersededCount = 0;
    std::size_t completedCount = 0;
    std::size_t acceptedCount = 0;
    std::size_t rejectedCount = 0;
    std::size_t failedCount = 0;
    std::size_t activationCount = 0;
    std::size_t pendingDepth = 0;
    std::size_t maximumPendingDepth = 0;
    std::size_t retainedCompletionRecordCount = 0;
    std::uint64_t requestReceivedAtMicros = 0;
    std::uint64_t launchedAtMicros = 0;
    std::uint64_t readyAtMicros = 0;
    std::uint64_t activationPendingAtMicros = 0;
    std::uint64_t activeAtMicros = 0;
    std::uint64_t lastPreparationMicros = 0;
    std::uint64_t maxPreparationMicros = 0;
    std::uint64_t lastRequestToActiveMicros = 0;
    std::uint64_t maxRequestToActiveMicros = 0;
};

struct PerformancePublishRequestResult
{
    bool accepted = false;
    bool duplicateSuppressed = false;
    bool supersededPrevious = false;
    bool cancellationRequested = false;
    PerformancePublishRequest request;
};

class PerformancePublishController final
{
public:
    explicit PerformancePublishController(PerformancePublishControllerConfig config = {});

    PerformancePublishRequestResult request(std::uint64_t projectGeneration,
                                            std::size_t draftRevision,
                                            std::string authoredContentDigest,
                                            std::string macroSchemaDigest,
                                            std::uint64_t nowMicros = 0);
    bool markPreparing(const PerformancePublishRequestIdentity& identity,
                       std::uint64_t nowMicros = 0);
    bool acceptPrepared(const PerformancePublishResult& result,
                        std::uint64_t nowMicros = 0);
    bool markActivationPending(const PerformancePublishRequestIdentity& identity,
                               std::uint64_t nowMicros = 0);
    bool markActive(const PerformancePublishRequestIdentity& identity,
                    std::uint64_t nowMicros = 0);
    bool fail(const PerformancePublishRequestIdentity& identity,
              PerformancePublishFinding finding);
    bool cancelCurrent();
    void reset(bool clearActive = true, bool advanceCancellationGeneration = true);

    bool isCurrent(const PerformancePublishRequestIdentity& identity) const noexcept;
    PerformancePublishControllerSnapshot getSnapshot() const;
    std::deque<PerformancePublishCompletionRecord> getCompletionRecords() const
    {
        return completionRecords;
    }

private:
    bool transitionTo(PerformancePublishPreparationState state);
    void publishSnapshot();
    void recordCompletion(const PerformancePublishRequestIdentity& identity,
                          PerformancePublishPreparationState outcome,
                          std::uint64_t preparedBuildId,
                          bool accepted);

    PerformancePublishControllerConfig config;
    PerformancePublishControllerSnapshot snapshot;
    std::shared_ptr<const PerformancePublishControllerSnapshot> publishedSnapshot;
    std::deque<PerformancePublishCompletionRecord> completionRecords;
    std::uint64_t nextRequestId = 1;
    std::uint64_t cancellationGeneration = 1;
};
} // namespace drs::engine
