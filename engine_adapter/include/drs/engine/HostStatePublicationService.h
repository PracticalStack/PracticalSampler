#pragma once

#include "drs/engine/HostSessionState.h"

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace drs::engine
{
enum class HostStatePublicationKind : std::uint8_t
{
    presetOnly = 0,
    authoringProject,
    performancePackage
};

struct HostStatePublicationRequest
{
    std::uint64_t requestId = 0;
    std::string publicationKey;
    HostStatePublicationKind kind = HostStatePublicationKind::presetOnly;
    RuntimePresetState presetState;
    std::shared_ptr<const RuntimeProjectModel> project;
    HostProjectBinding projectBinding;
    HostPerformancePackageBinding performancePackageBinding;
    std::size_t revision = 0;
    std::size_t savedRevision = 0;
    bool dirty = false;
    std::optional<HostPublishedCheckpoint> publishedState;
};

struct HostStatePublicationResult
{
    std::uint64_t requestId = 0;
    std::string publicationKey;
    std::size_t revision = 0;
    bool serialized = false;
    std::string text;
    std::string failure;
    std::uint64_t durationMicros = 0;
};

struct HostStatePublicationServiceStatus
{
    std::uint64_t submittedCount = 0;
    std::uint64_t startedCount = 0;
    std::uint64_t completedCount = 0;
    std::uint64_t failedCount = 0;
    std::uint64_t coalescedCount = 0;
    std::uint64_t latestSubmittedRequestId = 0;
    std::uint64_t latestStartedRequestId = 0;
    std::uint64_t latestCompletedRequestId = 0;
    std::size_t latestSubmittedRevision = 0;
    std::size_t latestCompletedRevision = 0;
    std::size_t pendingCount = 0;
    std::size_t maximumPendingCount = 0;
    std::size_t pendingCompletionCount = 0;
    std::size_t maximumPendingCompletionCount = 0;
    bool inFlight = false;
    std::uint64_t lastDurationMicros = 0;
    std::uint64_t maximumDurationMicros = 0;
};

class HostStatePublicationService final
{
public:
    HostStatePublicationService();
    ~HostStatePublicationService();

    HostStatePublicationService(const HostStatePublicationService&) = delete;
    HostStatePublicationService& operator=(const HostStatePublicationService&) = delete;

    bool submit(HostStatePublicationRequest request);
    std::vector<HostStatePublicationResult> drainCompleted();
    HostStatePublicationServiceStatus getStatus() const;
    bool waitForIdle(std::uint64_t timeoutMilliseconds);

private:
    void run();

    mutable std::mutex mutex;
    std::condition_variable condition;
    std::condition_variable idleCondition;
    bool stopRequested = false;
    std::optional<HostStatePublicationRequest> queued;
    std::vector<HostStatePublicationResult> completed;
    HostStatePublicationServiceStatus status;
    std::thread worker;
};
} // namespace drs::engine
