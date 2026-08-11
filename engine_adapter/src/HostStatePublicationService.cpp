#include "drs/engine/HostStatePublicationService.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace drs::engine
{
namespace
{
using Clock = std::chrono::steady_clock;
HostStatePublicationResult serializeRequest(const HostStatePublicationRequest& request)
{
    const auto started = Clock::now();
    HostStatePublicationResult result;
    result.requestId = request.requestId;
    result.publicationKey = request.publicationKey;
    result.revision = request.revision;

    if (request.kind == HostStatePublicationKind::presetOnly)
    {
        result.text = serializeRuntimePresetState(request.presetState);
        result.serialized = !result.text.empty();
    }
    else if (request.kind == HostStatePublicationKind::performancePackage)
    {
        HostSessionState state;
        state.presetState = request.presetState;
        state.performancePackageBinding = request.performancePackageBinding;
        const auto serialized = serializeHostSessionState(state);
        result.serialized = serialized.serialized;
        result.text = serialized.text;
        if (!serialized.serialized)
            result.failure = serialized.findings.empty()
                ? "Performance-package host state serialization failed."
                : serialized.findings.front().message;
    }
    else if (request.project == nullptr || request.project->projectId.empty())
    {
        result.failure = "Authoring host state serialization requires an immutable project checkpoint.";
    }
    else
    {
        const auto& project = *request.project;
        HostSessionState state;
        state.presetState = request.presetState;
        state.projectBinding = request.projectBinding;
        if (state.projectBinding.projectId != project.projectId
            || state.projectBinding.manifestFileName.empty())
        {
            state.projectBinding = {};
            state.projectBinding.projectId = project.projectId;
            state.projectBinding.manifestFileName = "unsaved-project.drsproj";
            state.projectBinding.contentRootHint = project.contentRootPath;
        }

        const auto digestPath = state.projectBinding.manifestPath.empty()
            ? state.projectBinding.manifestFileName
            : state.projectBinding.manifestPath;
        state.projectBinding.manifestDigest = computeHostProjectManifestDigest(project, digestPath);
        state.authoringState.revision = request.revision;
        state.authoringState.savedRevision = request.savedRevision;
        state.authoringState.dirty = request.dirty;
        if (request.dirty || state.projectBinding.manifestPath.empty())
            state.authoringState.projectSnapshot = project;
        state.publishedState = request.publishedState;

        const auto serialized = serializeHostSessionState(state, digestPath);
        result.serialized = serialized.serialized;
        result.text = serialized.text;
        if (!serialized.serialized)
            result.failure = serialized.findings.empty()
                ? "Authoring host state serialization failed."
                : serialized.findings.front().message;
    }

    result.durationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started).count());
    if (!result.serialized && result.failure.empty())
        result.failure = "Host state serialization produced no publication.";
    return result;
}
} // namespace

HostStatePublicationService::HostStatePublicationService()
    : worker([this] { run(); })
{
}

HostStatePublicationService::~HostStatePublicationService()
{
    {
        const std::lock_guard<std::mutex> lock(mutex);
        stopRequested = true;
        queued.reset();
        status.pendingCount = 0;
    }
    condition.notify_all();
    if (worker.joinable())
        worker.join();
}

bool HostStatePublicationService::submit(HostStatePublicationRequest request)
{
    if (request.requestId == 0 || request.publicationKey.empty())
        return false;
    if (request.kind == HostStatePublicationKind::authoringProject && request.project == nullptr)
        return false;

    {
        const std::lock_guard<std::mutex> lock(mutex);
        if (stopRequested)
            return false;
        if (queued.has_value())
            ++status.coalescedCount;
        queued = std::move(request);
        ++status.submittedCount;
        status.latestSubmittedRequestId = queued->requestId;
        status.latestSubmittedRevision = queued->revision;
        status.pendingCount = 1;
        status.maximumPendingCount = std::max(status.maximumPendingCount, status.pendingCount);
    }
    condition.notify_one();
    return true;
}

std::vector<HostStatePublicationResult> HostStatePublicationService::drainCompleted()
{
    const std::lock_guard<std::mutex> lock(mutex);
    auto result = std::move(completed);
    completed.clear();
    status.pendingCompletionCount = 0;
    return result;
}

HostStatePublicationServiceStatus HostStatePublicationService::getStatus() const
{
    const std::lock_guard<std::mutex> lock(mutex);
    return status;
}

bool HostStatePublicationService::waitForIdle(const std::uint64_t timeoutMilliseconds)
{
    std::unique_lock<std::mutex> lock(mutex);
    return idleCondition.wait_for(lock,
                                 std::chrono::milliseconds(timeoutMilliseconds),
                                 [this] { return !status.inFlight && !queued.has_value(); });
}

void HostStatePublicationService::run()
{
    for (;;)
    {
        HostStatePublicationRequest request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this] { return stopRequested || queued.has_value(); });
            if (stopRequested)
                return;
            request = std::move(*queued);
            queued.reset();
            status.pendingCount = 0;
            status.inFlight = true;
            ++status.startedCount;
            status.latestStartedRequestId = request.requestId;
        }

        auto result = serializeRequest(request);

        {
            const std::lock_guard<std::mutex> lock(mutex);
            status.inFlight = false;
            ++status.completedCount;
            status.failedCount += result.serialized ? 0 : 1;
            status.latestCompletedRequestId = result.requestId;
            status.latestCompletedRevision = result.revision;
            status.lastDurationMicros = result.durationMicros;
            status.maximumDurationMicros = std::max(status.maximumDurationMicros,
                                                    result.durationMicros);
            completed.erase(std::remove_if(completed.begin(), completed.end(),
                                           [&result](const auto& older)
                                           {
                                               return older.serialized == result.serialized;
                                           }),
                            completed.end());
            completed.push_back(std::move(result));
            status.pendingCompletionCount = completed.size();
            status.maximumPendingCompletionCount = std::max(
                status.maximumPendingCompletionCount, status.pendingCompletionCount);
        }
        idleCondition.notify_all();
    }
}
} // namespace drs::engine
