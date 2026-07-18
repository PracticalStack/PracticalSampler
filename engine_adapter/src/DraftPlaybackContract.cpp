#include "drs/engine/DraftPlaybackContract.h"

#include <algorithm>
#include <utility>

namespace drs::engine
{
namespace
{
struct PlayableRangeSummary
{
    bool available = false;
    int lowestNote = 0;
    int highestNote = 127;
};

std::vector<PlaybackSnapshotFinding> mergeFindings(const PlaybackSnapshotBuildResult* snapshotResult,
                                                   const PreparedPlaybackBuildResult* preparedResult)
{
    std::vector<PlaybackSnapshotFinding> findings;

    if (snapshotResult != nullptr)
        findings = snapshotResult->findings;

    if (preparedResult != nullptr)
        findings.insert(findings.end(), preparedResult->findings.begin(), preparedResult->findings.end());

    return findings;
}

PlayableRangeSummary summarizePlayableRange(const PlaybackSnapshotBuildResult* snapshotResult)
{
    PlayableRangeSummary summary;

    if (snapshotResult == nullptr || snapshotResult->snapshot.zones.empty())
        return summary;

    auto lowestNote = 127;
    auto highestNote = 0;

    for (const auto& zone : snapshotResult->snapshot.zones)
    {
        lowestNote = std::min(lowestNote, zone.keyLow);
        highestNote = std::max(highestNote, zone.keyHigh);
    }

    if (lowestNote > highestNote)
        return summary;

    summary.available = true;
    summary.lowestNote = std::clamp(lowestNote, 0, 127);
    summary.highestNote = std::clamp(highestNote, summary.lowestNote, 127);
    return summary;
}
} // namespace

DraftPlaybackContract::DraftPlaybackContract(std::size_t initialDraftRevision)
{
    status.draftRevision = initialDraftRevision;
    status.preview.state = "Idle";
    status.performance.state = "Idle";
    status.lastEvent = "Contract initialized";
}

bool DraftPlaybackContract::setDraftRevision(std::size_t revision)
{
    if (!status.projectOpen || revision < status.draftRevision)
        return false;

    status.draftRevision = revision;
    status.lastEvent = "Draft revision changed";
    refreshPreparedStates();
    return true;
}

DraftPlaybackBuildRequest DraftPlaybackContract::requestPreviewBuild()
{
    return requestBuild(status.pendingPreview, "Preview", "Preview build queued");
}

DraftPlaybackBuildRequest DraftPlaybackContract::requestPerformanceBuild()
{
    return requestBuild(status.pendingPerformance, "Publish", "Publish build queued");
}

bool DraftPlaybackContract::completePreviewBuild(std::uint64_t requestId)
{
    if (!status.pendingPreview.active || status.pendingPreview.requestId != requestId)
        return false;

    return completeBuild(status.pendingPreview, status.preview, nullptr, nullptr, "Ready");
}

bool DraftPlaybackContract::completePerformanceBuild(std::uint64_t requestId)
{
    if (!status.pendingPerformance.active || status.pendingPerformance.requestId != requestId)
        return false;

    return completeBuild(status.pendingPerformance, status.performance, nullptr, nullptr, "Active");
}

bool DraftPlaybackContract::completePreviewBuild(std::uint64_t requestId, const PlaybackSnapshotBuildResult& buildResult)
{
    if (!status.pendingPreview.active || status.pendingPreview.requestId != requestId)
        return false;

    return completeBuild(status.pendingPreview, status.preview, &buildResult, nullptr, "Ready");
}

bool DraftPlaybackContract::completePerformanceBuild(std::uint64_t requestId, const PlaybackSnapshotBuildResult& buildResult)
{
    if (!status.pendingPerformance.active || status.pendingPerformance.requestId != requestId)
        return false;

    return completeBuild(status.pendingPerformance, status.performance, &buildResult, nullptr, "Active");
}

bool DraftPlaybackContract::completePreviewBuild(std::uint64_t requestId,
                                                 const PlaybackSnapshotBuildResult& snapshotResult,
                                                 const PreparedPlaybackBuildResult& preparedResult)
{
    if (!status.pendingPreview.active || status.pendingPreview.requestId != requestId)
        return false;

    return completeBuild(status.pendingPreview, status.preview, &snapshotResult, &preparedResult, "Ready");
}

bool DraftPlaybackContract::completePerformanceBuild(std::uint64_t requestId,
                                                     const PlaybackSnapshotBuildResult& snapshotResult,
                                                     const PreparedPlaybackBuildResult& preparedResult)
{
    if (!status.pendingPerformance.active || status.pendingPerformance.requestId != requestId)
        return false;

    return completeBuild(status.pendingPerformance, status.performance, &snapshotResult, &preparedResult, "Active");
}

bool DraftPlaybackContract::failPreviewBuild(std::uint64_t requestId,
                                             const std::vector<std::string>& issues,
                                             const std::string& state)
{
    if (!status.pendingPreview.active || status.pendingPreview.requestId != requestId)
        return false;

    return failBuild(status.pendingPreview, status.preview, issues, state);
}

bool DraftPlaybackContract::failPerformanceBuild(std::uint64_t requestId,
                                                 const std::vector<std::string>& issues,
                                                 const std::string& state)
{
    if (!status.pendingPerformance.active || status.pendingPerformance.requestId != requestId)
        return false;

    return failBuild(status.pendingPerformance, status.performance, issues, state);
}

bool DraftPlaybackContract::cancelPreviewBuild(std::uint64_t requestId)
{
    if (!status.pendingPreview.active || status.pendingPreview.requestId != requestId)
        return false;

    return cancelBuild(status.pendingPreview, "Preview");
}

bool DraftPlaybackContract::cancelPerformanceBuild(std::uint64_t requestId)
{
    if (!status.pendingPerformance.active || status.pendingPerformance.requestId != requestId)
        return false;

    return cancelBuild(status.pendingPerformance, "Publish");
}

void DraftPlaybackContract::closeProject()
{
    status.projectOpen = false;
    status.deviceRestartInProgress = false;
    status.pendingPreview = {};
    status.pendingPerformance = {};
    resetPreparedRevision(status.preview, "Closed");
    resetPreparedRevision(status.performance, "Closed");
    status.lastEvent = "Project closed";
}

void DraftPlaybackContract::reopenProject(std::size_t draftRevision)
{
    status = {};
    status.projectOpen = true;
    status.draftRevision = draftRevision;
    status.preview.state = "Idle";
    status.performance.state = "Idle";
    status.lastEvent = "Project reopened";
}

bool DraftPlaybackContract::beginDeviceRestart()
{
    if (!status.projectOpen || status.deviceRestartInProgress)
        return false;

    status.deviceRestartInProgress = true;
    status.pendingPreview = {};
    status.pendingPerformance = {};
    status.lastEvent = "Device restart started";
    refreshPreparedStates();
    return true;
}

bool DraftPlaybackContract::completeDeviceRestart(bool restored)
{
    if (!status.deviceRestartInProgress)
        return false;

    status.deviceRestartInProgress = false;
    status.lastEvent = restored ? "Device restart completed" : "Device restart failed";

    if (!restored)
    {
        resetPreparedRevision(status.preview, "Failed");
        resetPreparedRevision(status.performance, "Failed");
        return true;
    }

    refreshPreparedStates();
    return true;
}

DraftPlaybackBuildRequest DraftPlaybackContract::requestBuild(DraftPlaybackPendingRequest& pending,
                                                              const std::string& kind,
                                                              const std::string& state)
{
    DraftPlaybackBuildRequest request;
    request.requestedRevision = status.draftRevision;

    if (!status.projectOpen)
    {
        request.state = kind + " build rejected because the project is closed";
        return request;
    }

    if (status.deviceRestartInProgress)
    {
        request.state = kind + " build rejected during device restart";
        return request;
    }

    pending.active = true;
    pending.requestId = nextRequestId++;
    pending.cancellationId = pending.requestId;
    pending.requestedRevision = status.draftRevision;
    pending.lifecycleState = PlaybackSnapshotLifecycleState::preparing;
    pending.state = "Preparing";

    request.accepted = true;
    request.requestId = pending.requestId;
    request.cancellationId = pending.cancellationId;
    request.requestedRevision = pending.requestedRevision;
    request.lifecycleState = pending.lifecycleState;
    request.state = state;

    status.lastEvent = kind + " build requested";
    refreshPreparedStates();
    return request;
}

bool DraftPlaybackContract::completeBuild(DraftPlaybackPendingRequest& pending,
                                          DraftPlaybackPreparedRevision& prepared,
                                          const PlaybackSnapshotBuildResult* buildResult,
                                          const PreparedPlaybackBuildResult* preparedBuildResult,
                                          const std::string& completedState)
{
    const auto snapshotFailed = buildResult != nullptr && (!buildResult->built || !buildResult->activationEligible);
    const auto preparedFailed = preparedBuildResult != nullptr
        && (!preparedBuildResult->built || !preparedBuildResult->activationEligible);
    if (snapshotFailed || preparedFailed)
    {
        const auto failedState = preparedFailed
            ? (preparedBuildResult->state.empty() ? completedState + " failed" : preparedBuildResult->state)
            : (buildResult->state.empty() ? completedState + " failed" : buildResult->state);
        pending = {};
        prepared.buildId = buildResult != nullptr ? buildResult->buildId : 0;
        prepared.preparedBuildId = preparedBuildResult != nullptr ? preparedBuildResult->buildId : 0;
        prepared.preparedAssetsAvailable = false;
        prepared.preparationCacheHitCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.cacheHitCount : 0;
        prepared.preparationCacheMissCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.cacheMissCount : 0;
        prepared.preparedSampleCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedSampleCount : 0;
        prepared.preparedStreamCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedStreamCount : 0;
        prepared.preparedZoneCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedZoneCount : 0;
        prepared.preparedOwnershipRecordCount = preparedBuildResult != nullptr
            ? preparedBuildResult->metrics.preparedOwnershipRecordCount
            : 0;
        prepared.preparedBytes = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedBytes : 0;
        prepared.preparedOwnershipBytes = preparedBuildResult != nullptr
            ? preparedBuildResult->metrics.preparedOwnershipBytes
            : 0;
        prepared.playableRangeAvailable = false;
        prepared.lowestPlayableNote = 0;
        prepared.highestPlayableNote = 127;
        prepared.findings = mergeFindings(buildResult, preparedBuildResult);
        prepared.activationEligible = false;
        prepared.lifecycleState = preparedBuildResult != nullptr
            ? preparedBuildResult->lifecycleState
            : buildResult->lifecycleState;

        if (!prepared.available)
            prepared.state = failedState;

        status.lastEvent = failedState;
        refreshPreparedStates();
        return true;
    }

    prepared.available = true;
    prepared.revision = pending.requestedRevision;
    prepared.buildId = buildResult != nullptr ? buildResult->buildId : pending.requestId;
    prepared.preparedAssetsAvailable = preparedBuildResult != nullptr ? preparedBuildResult->built : false;
    prepared.preparedBuildId = preparedBuildResult != nullptr ? preparedBuildResult->buildId : 0;
    prepared.activationEligible = preparedBuildResult != nullptr
        ? preparedBuildResult->activationEligible
        : (buildResult != nullptr ? buildResult->activationEligible : true);
    prepared.lifecycleState = preparedBuildResult != nullptr
        ? preparedBuildResult->lifecycleState
        : (buildResult != nullptr ? buildResult->lifecycleState : PlaybackSnapshotLifecycleState::ready);
    prepared.contentDigest = buildResult != nullptr ? buildResult->snapshot.contentDigest : std::string {};
    prepared.preparedContentDigest = preparedBuildResult != nullptr
        ? preparedBuildResult->prepared.preparedContentDigest
        : std::string {};
    prepared.state = completedState;
    prepared.preparedSampleCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedSampleCount : 0;
    prepared.preparedStreamCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedStreamCount : 0;
    prepared.preparedZoneCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedZoneCount : 0;
    prepared.preparedOwnershipRecordCount = preparedBuildResult != nullptr
        ? preparedBuildResult->metrics.preparedOwnershipRecordCount
        : 0;
    prepared.preparedBytes = preparedBuildResult != nullptr ? preparedBuildResult->metrics.preparedBytes : 0;
    prepared.preparedOwnershipBytes = preparedBuildResult != nullptr
        ? preparedBuildResult->metrics.preparedOwnershipBytes
        : 0;
    const auto playableRange = summarizePlayableRange(buildResult);
    prepared.playableRangeAvailable = playableRange.available;
    prepared.lowestPlayableNote = playableRange.lowestNote;
    prepared.highestPlayableNote = playableRange.highestNote;
    prepared.preparationCacheHitCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.cacheHitCount : 0;
    prepared.preparationCacheMissCount = preparedBuildResult != nullptr ? preparedBuildResult->metrics.cacheMissCount : 0;
    prepared.findings = mergeFindings(buildResult, preparedBuildResult);
    pending = {};
    status.lastEvent = "Build completed";
    refreshPreparedStates();
    return true;
}

bool DraftPlaybackContract::failBuild(DraftPlaybackPendingRequest& pending,
                                      DraftPlaybackPreparedRevision& prepared,
                                      const std::vector<std::string>& issues,
                                      const std::string& failedState)
{
    pending = {};
    prepared.findings.clear();
    prepared.findings.reserve(issues.size());
    for (const auto& issue : issues)
        prepared.findings.push_back({ PlaybackSnapshotFindingSeverity::error, "contract-failure", "", issue });
    prepared.activationEligible = false;
    prepared.lifecycleState = PlaybackSnapshotLifecycleState::failed;

    if (!prepared.available)
        prepared.state = failedState;

    status.lastEvent = failedState;
    refreshPreparedStates();
    return true;
}

bool DraftPlaybackContract::cancelBuild(DraftPlaybackPendingRequest& pending, const std::string& kind)
{
    pending = {};
    status.lastEvent = kind + " build canceled";
    refreshPreparedStates();
    return true;
}

void DraftPlaybackContract::refreshPreparedStates()
{
    if (!status.projectOpen)
        return;

    if (status.deviceRestartInProgress)
    {
        status.preview.state = "Restarting";
        status.performance.state = "Restarting";
        return;
    }

    if (status.pendingPreview.active)
    {
        status.preview.state = status.preview.available ? "Stale" : "Preparing";
    }
    else if (!status.preview.available)
    {
        if (status.preview.findings.empty())
            status.preview.state = "Idle";
    }
    else
    {
        status.preview.state = status.preview.revision == status.draftRevision ? "Ready" : "Stale";
    }

    if (status.pendingPerformance.active)
    {
        status.performance.state = status.performance.available ? "Active" : "Preparing";
    }
    else if (!status.performance.available)
    {
        if (status.performance.findings.empty())
            status.performance.state = "Idle";
    }
    else
    {
        status.performance.state = "Active";
    }
}

void DraftPlaybackContract::resetPreparedRevision(DraftPlaybackPreparedRevision& prepared, const std::string& state)
{
    prepared.available = false;
    prepared.revision = 0;
    prepared.buildId = 0;
    prepared.preparedAssetsAvailable = false;
    prepared.preparedBuildId = 0;
    prepared.activationEligible = false;
    prepared.lifecycleState = PlaybackSnapshotLifecycleState::idle;
    prepared.contentDigest.clear();
    prepared.preparedContentDigest.clear();
    prepared.state = state;
    prepared.preparedSampleCount = 0;
    prepared.preparedStreamCount = 0;
    prepared.preparedZoneCount = 0;
    prepared.preparedOwnershipRecordCount = 0;
    prepared.preparedBytes = 0;
    prepared.preparedOwnershipBytes = 0;
    prepared.playableRangeAvailable = false;
    prepared.lowestPlayableNote = 0;
    prepared.highestPlayableNote = 127;
    prepared.preparationCacheHitCount = 0;
    prepared.preparationCacheMissCount = 0;
    prepared.findings.clear();
}
} // namespace drs::engine
