#pragma once

#include "drs/engine/AuthoringPreviewContract.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace drs::engine
{
struct AuthoringPreviewControllerConfig
{
    std::uint64_t coalescingWindowMicros = 12000;
    std::uint64_t maximumLaunchDelayMicros = 40000;
    std::size_t maximumCompletionRecords = 32;
    std::size_t maximumWarmPreparedRecords = 8;
};

struct AuthoringPreviewCompletionRecord
{
    AuthoringPreviewRequestIdentity identity;
    AuthoringPreviewPreparationState outcome = AuthoringPreviewPreparationState::idle;
    std::uint64_t preparedBuildId = 0;
    bool accepted = false;
};

struct AuthoringPreviewControllerSnapshot
{
    bool hasRequest = false;
    AuthoringPreviewRequest currentRequest;
    AuthoringPreviewPreparationState preparationState = AuthoringPreviewPreparationState::idle;
    AuthoringPreviewActivationState activationState = AuthoringPreviewActivationState::noActivation;
    std::uint64_t acceptedPreparedBuildId = 0;
    std::uint64_t reusablePreparedBuildId = 0;
    std::uint64_t coalescingBurstStartedAtMicros = 0;
    std::uint64_t launchEligibleAtMicros = 0;
    std::uint64_t configuredCoalescingWindowMicros = 0;
    std::uint64_t configuredMaximumLaunchDelayMicros = 0;
    std::string failureState;
    std::size_t requestedCount = 0;
    std::size_t coalescedCount = 0;
    std::size_t launchedCount = 0;
    std::size_t canceledCount = 0;
    std::size_t supersededCount = 0;
    std::size_t completedCount = 0;
    std::size_t acceptedCount = 0;
    std::size_t rejectedCount = 0;
    std::size_t reusedPreparedCount = 0;
    std::size_t pendingDepth = 0;
    std::size_t maximumPendingDepth = 0;
    std::size_t retainedCompletionRecordCount = 0;
    std::size_t activationCount = 0;
};

struct AuthoringPreviewRequestResult
{
    bool accepted = false;
    bool supersededPrevious = false;
    bool cancellationRequested = false;
    bool expeditedCurrent = false;
    AuthoringPreviewRequest request;
};

struct AuthoringPreviewLaunchResult
{
    bool launched = false;
    bool deadlineForced = false;
    bool warmPreparedResultAvailable = false;
    AuthoringPreviewRequest request;
    std::uint64_t reusablePreparedBuildId = 0;
};

class AuthoringPreviewController final
{
public:
    explicit AuthoringPreviewController(AuthoringPreviewControllerConfig config = {});

    AuthoringPreviewRequestResult request(AuthoringPreviewScope scope,
                                          std::size_t draftRevision,
                                          std::string selectedZoneId,
                                          AuthoringPreviewRequestReason reason,
                                          AuthoringPreviewInvalidationCategory invalidationCategory,
                                          std::string requestSignature,
                                          std::uint64_t nowMicros);
    AuthoringPreviewLaunchResult launchIfEligible(std::uint64_t nowMicros,
                                                  bool directAuditionContentPrepared = false);
    bool acceptPrepared(const AuthoringPreviewRequestIdentity& identity,
                        std::uint64_t preparedBuildId);
    bool markActivationPending(const AuthoringPreviewRequestIdentity& identity);
    bool markActive(const AuthoringPreviewRequestIdentity& identity);
    bool fail(const AuthoringPreviewRequestIdentity& identity, std::string failureState);
    bool cancelCurrent();
    void recordWorkerCancellation();
    void reset(bool advanceCancellationGeneration = true);

    bool isCurrent(const AuthoringPreviewRequestIdentity& identity) const noexcept;
    AuthoringPreviewControllerSnapshot getSnapshot() const { return snapshot; }
    std::deque<AuthoringPreviewCompletionRecord> getCompletionRecords() const
    {
        return completionRecords;
    }

private:
    struct WarmPreparedRecord
    {
        AuthoringPreviewScope scope = AuthoringPreviewScope::selectedZone;
        std::string selectedZoneId;
        std::string requestSignature;
        std::uint64_t preparedBuildId = 0;
    };

    bool transitionTo(AuthoringPreviewPreparationState state);
    void recordCompletion(const AuthoringPreviewRequestIdentity& identity,
                          AuthoringPreviewPreparationState outcome,
                          std::uint64_t preparedBuildId,
                          bool accepted);
    std::uint64_t findReusablePreparedBuildId(AuthoringPreviewScope scope,
                                              const std::string& selectedZoneId,
                                              const std::string& requestSignature) const;
    void rememberPreparedResult(const AuthoringPreviewRequest& request,
                                std::uint64_t preparedBuildId);

    AuthoringPreviewControllerConfig config;
    AuthoringPreviewControllerSnapshot snapshot;
    std::deque<AuthoringPreviewCompletionRecord> completionRecords;
    std::deque<WarmPreparedRecord> warmPreparedRecords;
    std::uint64_t nextRequestId = 1;
    std::uint64_t cancellationGeneration = 1;
};
} // namespace drs::engine
