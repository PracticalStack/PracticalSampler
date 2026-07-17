#include "drs/engine/DraftPlaybackContract.h"

#include <utility>

namespace drs::engine
{
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

    return completeBuild(status.pendingPreview, status.preview, "Ready");
}

bool DraftPlaybackContract::completePerformanceBuild(std::uint64_t requestId)
{
    if (!status.pendingPerformance.active || status.pendingPerformance.requestId != requestId)
        return false;

    return completeBuild(status.pendingPerformance, status.performance, "Active");
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
    pending.requestedRevision = status.draftRevision;
    pending.state = "Preparing";

    request.accepted = true;
    request.requestId = pending.requestId;
    request.requestedRevision = pending.requestedRevision;
    request.state = state;

    status.lastEvent = kind + " build requested";
    refreshPreparedStates();
    return request;
}

bool DraftPlaybackContract::completeBuild(DraftPlaybackPendingRequest& pending,
                                          DraftPlaybackPreparedRevision& prepared,
                                          const std::string& completedState)
{
    prepared.available = true;
    prepared.revision = pending.requestedRevision;
    prepared.state = completedState;
    prepared.issues.clear();
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
    prepared.issues = issues;

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
        if (status.preview.issues.empty())
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
        if (status.performance.issues.empty())
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
    prepared.state = state;
    prepared.issues.clear();
}
} // namespace drs::engine
