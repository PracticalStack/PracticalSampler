#include "drs/engine/PreparedPlayback.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <unordered_map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using ordered_json = nlohmann::ordered_json;

void addFinding(PreparedPlaybackBuildResult& result,
                PlaybackSnapshotFindingSeverity severity,
                const std::string& code,
                const std::string& path,
                const std::string& message)
{
    result.findings.push_back({ severity, code, path, message });
}

std::string computeFnv1a64Hex(const std::string& text)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    std::ostringstream stream;
    stream << std::hex;
    stream.width(16);
    stream.fill('0');
    stream << hash;
    return stream.str();
}

ordered_json serializePrepared(const ImmutablePreparedPlayback& prepared, bool includeDigest)
{
    ordered_json root;
    root["snapshotContentDigest"] = prepared.snapshotContentDigest;
    root["compilerVersion"] = prepared.compilerVersion;
    root["draftRevision"] = prepared.draftRevision;
    root["containerId"] = prepared.containerId;
    root["containerPath"] = prepared.containerPath;
    root["payloadEncoding"] = prepared.payloadEncoding;
    root["pageSizeBytes"] = prepared.pageSizeBytes;

    if (includeDigest)
        root["snapshotBuildId"] = prepared.snapshotBuildId;

    if (includeDigest)
        root["preparedContentDigest"] = prepared.preparedContentDigest;

    ordered_json samples = ordered_json::array();
    for (const auto& sample : prepared.samples)
    {
        ordered_json sampleObject;
        sampleObject["sampleSourceId"] = sample.sampleSourceId;
        sampleObject["streamSampleId"] = sample.streamSampleId;
        sampleObject["sourcePath"] = sample.sourcePath;
        sampleObject["sourceFingerprintHex"] = sample.sourceFingerprintHex;
        sampleObject["formatName"] = sample.formatName;
        sampleObject["role"] = sample.role;
        sampleObject["sampleRate"] = sample.sampleRate;
        sampleObject["frameCount"] = sample.frameCount;
        sampleObject["channelCount"] = sample.channelCount;
        sampleObject["rootMidiNotePresent"] = sample.rootMidiNotePresent;
        sampleObject["rootMidiNote"] = sample.rootMidiNote;
        sampleObject["loopRangePresent"] = sample.loopRangePresent;
        sampleObject["loopStartFrame"] = sample.loopStartFrame;
        sampleObject["loopEndFrame"] = sample.loopEndFrame;
        sampleObject["ownershipToken"] = sample.ownershipToken;
        sampleObject["cacheKey"] = sample.cacheKey;
        samples.push_back(std::move(sampleObject));
    }
    root["samples"] = std::move(samples);

    ordered_json streams = ordered_json::array();
    for (const auto& streamHandle : prepared.streams)
    {
        ordered_json streamObject;
        streamObject["sampleSourceId"] = streamHandle.sampleSourceId;
        streamObject["streamSampleId"] = streamHandle.streamSampleId;
        streamObject["containerId"] = streamHandle.containerId;
        streamObject["containerPath"] = streamHandle.containerPath;
        streamObject["payloadEncoding"] = streamHandle.payloadEncoding;
        streamObject["pageSizeBytes"] = streamHandle.pageSizeBytes;
        streamObject["payloadOffsetBytes"] = streamHandle.payloadOffsetBytes;
        streamObject["payloadSizeBytes"] = streamHandle.payloadSizeBytes;
        streamObject["prefetchBytes"] = streamHandle.prefetchBytes;
        streamObject["ownershipToken"] = streamHandle.ownershipToken;
        streamObject["cacheKey"] = streamHandle.cacheKey;

        ordered_json pages = ordered_json::array();
        for (const auto& page : streamHandle.pages)
        {
            ordered_json pageObject;
            pageObject["pageIndex"] = page.pageIndex;
            pageObject["offsetBytes"] = page.offsetBytes;
            pageObject["sizeBytes"] = page.sizeBytes;
            pages.push_back(std::move(pageObject));
        }

        streamObject["pages"] = std::move(pages);
        streams.push_back(std::move(streamObject));
    }
    root["streams"] = std::move(streams);

    ordered_json zones = ordered_json::array();
    for (const auto& zone : prepared.zones)
    {
        ordered_json zoneObject;
        zoneObject["zoneId"] = zone.zoneId;
        zoneObject["sampleSourceId"] = zone.sampleSourceId;
        zoneObject["streamSampleId"] = zone.streamSampleId;
        zoneObject["preparedSampleIndex"] = zone.preparedSampleIndex;
        zoneObject["preparedStreamIndex"] = zone.preparedStreamIndex;
        zoneObject["rootKey"] = zone.rootKey;
        zoneObject["keyLow"] = zone.keyLow;
        zoneObject["keyHigh"] = zone.keyHigh;
        zoneObject["velocityLow"] = zone.velocityLow;
        zoneObject["velocityHigh"] = zone.velocityHigh;
        zoneObject["gainDb"] = zone.gainDb;
        zoneObject["pan"] = zone.pan;
        zoneObject["sampleStartFrame"] = zone.sampleStartFrame;
        zoneObject["loopEnabled"] = zone.loopEnabled;
        zoneObject["loopStartFrame"] = zone.loopStartFrame;
        zoneObject["loopEndFrame"] = zone.loopEndFrame;
        zones.push_back(std::move(zoneObject));
    }
    root["zones"] = std::move(zones);

    ordered_json notes = ordered_json::array();
    for (const auto& note : prepared.notes)
        notes.push_back(note);
    root["notes"] = std::move(notes);
    return root;
}

std::string normalizePath(const std::string& pathText)
{
    return fs::path(pathText).lexically_normal().generic_string();
}

std::string buildCacheKey(const std::string& compilerVersion,
                          const PlaybackSnapshotSampleIdentity& sampleIdentity,
                          const RuntimeStreamSampleDefinition& streamSample,
                          const RuntimeStreamContainerModel& container)
{
    std::ostringstream stream;
    stream << compilerVersion
           << "|sampleSourceId=" << sampleIdentity.sampleSourceId
           << "|sourcePath=" << normalizePath(sampleIdentity.sourcePath)
           << "|checksum=" << streamSample.sourceChecksumHex
           << "|format=" << streamSample.formatName
           << "|encoding=" << container.payloadEncoding
           << "|pageSize=" << container.pageSizeBytes;
    return "fnv1a64:" + computeFnv1a64Hex(stream.str());
}

const RuntimeStreamSampleDefinition* findStreamSampleByPath(const RuntimeStreamContainerModel& container,
                                                            const std::string& normalizedPath)
{
    const auto iterator = std::find_if(container.samples.begin(),
                                       container.samples.end(),
                                       [&](const RuntimeStreamSampleDefinition& sample)
                                       {
                                           return normalizePath(sample.sourcePath) == normalizedPath;
                                       });
    return iterator != container.samples.end() ? &(*iterator) : nullptr;
}

const RuntimeStreamSampleDefinition* findStreamSampleById(const RuntimeStreamContainerModel& container,
                                                          const std::string& sampleId)
{
    const auto iterator = std::find_if(container.samples.begin(),
                                       container.samples.end(),
                                       [&](const RuntimeStreamSampleDefinition& sample)
                                       {
                                           return sample.sampleId == sampleId;
                                       });
    return iterator != container.samples.end() ? &(*iterator) : nullptr;
}
} // namespace

PreparedPlaybackService::PreparedPlaybackService(std::string compilerVersionIn,
                                                 std::size_t maxPendingJobsIn,
                                                 bool enableBackgroundWorkerIn)
    : compilerVersion(std::move(compilerVersionIn)),
      maxPendingJobs(maxPendingJobsIn),
      backgroundWorkerEnabled(enableBackgroundWorkerIn)
{
    refreshWorkerStatus();

    if (backgroundWorkerEnabled)
        workerThread = std::thread([this] { runBackgroundWorker(); });
}

PreparedPlaybackService::~PreparedPlaybackService()
{
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        stopWorkerRequested = true;
    }

    workerCondition.notify_all();
    workerIdleCondition.notify_all();

    if (workerThread.joinable())
        workerThread.join();
}

PreparedPlaybackBuildRequest PreparedPlaybackService::requestBuild(const PlaybackSnapshotBuildResult& snapshotResult)
{
    PreparedPlaybackBuildRequest request;
    request.snapshotBuildId = snapshotResult.buildId;
    request.requestedDraftRevision = snapshotResult.requestedDraftRevision;
    request.activationRequested = snapshotResult.activationRequested;

    if (!snapshotResult.built || !snapshotResult.activationEligible || snapshotResult.snapshot.contentDigest.empty())
    {
        request.state = "Prepared playback build rejected because the immutable snapshot is unavailable";
        request.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        return request;
    }

    request.accepted = true;
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        request.buildId = nextBuildId++;
    }
    request.cancellationId = request.buildId;
    request.lifecycleState = PlaybackSnapshotLifecycleState::preparing;
    request.state = "Prepared playback build queued";
    return request;
}

PreparedPlaybackBuildResult PreparedPlaybackService::prepare(const PreparedPlaybackBuildRequest& request,
                                                             const PlaybackSnapshotBuildResult& snapshotResult,
                                                             const RuntimeStreamLoadResult& streamResult)
{
    PreparedPlaybackBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = request.cancellationId;
    result.snapshotBuildId = request.snapshotBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lifecycleState = request.accepted
        ? PlaybackSnapshotLifecycleState::preparing
        : PlaybackSnapshotLifecycleState::failed;
    result.state = request.accepted
        ? "Prepared playback build in progress"
        : request.state;

    const auto startTime = Clock::now();
    std::lock_guard<std::mutex> lock(workerMutex);

    if (!request.accepted)
    {
        result.metrics.failureCount = 1;
        if (snapshotResult.findings.empty())
        {
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "prepared-build-rejected",
                       "snapshot",
                       "Prepared playback build requires a valid immutable playback snapshot.");
        }
        result.buildDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
        return result;
    }

    if (!streamResult.loaded)
    {
        result.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        result.state = "Prepared playback build failed";
        result.metrics.failureCount = 1;
        addFinding(result,
                   PlaybackSnapshotFindingSeverity::error,
                   "missing-stream-container",
                   "streamContainer",
                   "Prepared playback requires a loaded stream container.");
        result.buildDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
        return result;
    }

    result.prepared.snapshotBuildId = snapshotResult.buildId;
    result.prepared.snapshotContentDigest = snapshotResult.snapshot.contentDigest;
    result.prepared.compilerVersion = compilerVersion;
    result.prepared.draftRevision = snapshotResult.snapshot.draftRevision;
    result.prepared.containerId = streamResult.container.containerId;
    result.prepared.containerPath = streamResult.containerPath;
    result.prepared.payloadEncoding = streamResult.container.payloadEncoding;
    result.prepared.pageSizeBytes = streamResult.container.pageSizeBytes;

    std::unordered_map<std::string, std::size_t> sampleIndices;
    std::unordered_map<std::string, std::size_t> streamIndices;

    result.prepared.samples.reserve(snapshotResult.snapshot.sampleIdentities.size());
    result.prepared.streams.reserve(snapshotResult.snapshot.sampleIdentities.size());
    result.prepared.zones.reserve(snapshotResult.snapshot.zones.size());

    for (std::size_t index = 0; index < snapshotResult.snapshot.sampleIdentities.size(); ++index)
    {
        const auto& sampleIdentity = snapshotResult.snapshot.sampleIdentities[index];
        const auto path = "sampleIdentities[" + std::to_string(index) + "]";
        const auto normalizedPath = normalizePath(sampleIdentity.sourcePath);

        const auto* streamSample = findStreamSampleByPath(streamResult.container, normalizedPath);
        if (streamSample == nullptr && !sampleIdentity.sampleSourceId.empty())
            streamSample = findStreamSampleById(streamResult.container, sampleIdentity.sampleSourceId);

        if (streamSample == nullptr)
        {
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "missing-prepared-stream-sample",
                       path + ".sourcePath",
                       "No compiled stream sample matches snapshot sample source '" + sampleIdentity.sampleSourceId + "'.");
            continue;
        }

        const auto cacheKey = buildCacheKey(compilerVersion, sampleIdentity, *streamSample, streamResult.container);
        const auto cacheIterator = std::find_if(cacheEntries.begin(),
                                                cacheEntries.end(),
                                                [&](const auto& entry)
                                                {
                                                    return entry.first == cacheKey;
                                                });

        PreparedPlaybackSampleHandle sampleHandle;
        PreparedPlaybackStreamHandle streamHandle;

        if (cacheIterator != cacheEntries.end())
        {
            sampleHandle = cacheIterator->second.sample;
            streamHandle = cacheIterator->second.stream;
            ++result.metrics.cacheHitCount;
        }
        else
        {
            sampleHandle.sampleSourceId = sampleIdentity.sampleSourceId;
            sampleHandle.streamSampleId = streamSample->sampleId;
            sampleHandle.sourcePath = normalizedPath;
            sampleHandle.sourceFingerprintHex = streamSample->sourceChecksumHex;
            sampleHandle.formatName = streamSample->formatName;
            sampleHandle.role = sampleIdentity.role.empty() ? streamSample->role : sampleIdentity.role;
            sampleHandle.sampleRate = streamSample->sampleRate;
            sampleHandle.frameCount = streamSample->frameCount;
            sampleHandle.channelCount = streamSample->channelCount;
            sampleHandle.rootMidiNotePresent = streamSample->rootMidiNotePresent;
            sampleHandle.rootMidiNote = streamSample->rootMidiNote;
            sampleHandle.loopRangePresent = streamSample->loopRangePresent;
            sampleHandle.loopStartFrame = streamSample->loopStartFrame;
            sampleHandle.loopEndFrame = streamSample->loopEndFrame;
            sampleHandle.ownershipToken = "cache:" + cacheKey;
            sampleHandle.cacheKey = cacheKey;

            streamHandle.sampleSourceId = sampleIdentity.sampleSourceId;
            streamHandle.streamSampleId = streamSample->sampleId;
            streamHandle.containerId = streamResult.container.containerId;
            streamHandle.containerPath = streamResult.containerPath;
            streamHandle.payloadEncoding = streamResult.container.payloadEncoding;
            streamHandle.pageSizeBytes = streamResult.container.pageSizeBytes;
            streamHandle.payloadOffsetBytes = streamSample->payloadOffsetBytes;
            streamHandle.payloadSizeBytes = streamSample->payloadSizeBytes;
            streamHandle.prefetchBytes = streamSample->prefetchBytes;
            streamHandle.ownershipToken = "cache:" + cacheKey;
            streamHandle.cacheKey = cacheKey;
            streamHandle.pages.reserve(streamSample->pages.size());
            for (const auto& page : streamSample->pages)
                streamHandle.pages.push_back({ page.pageIndex, page.offsetBytes, page.sizeBytes });

            retireSupersededCacheEntries(sampleIdentity.sampleSourceId, cacheKey);

            CacheEntry entry;
            entry.sample = sampleHandle;
            entry.stream = streamHandle;
            entry.retainedBytes = streamSample->payloadSizeBytes;
            cacheEntries.push_back({ cacheKey, std::move(entry) });
            ++result.metrics.cacheMissCount;
        }

        const auto sampleIndex = result.prepared.samples.size();
        result.prepared.samples.push_back(sampleHandle);
        sampleIndices.emplace(sampleIdentity.sampleSourceId, sampleIndex);

        const auto streamIndex = result.prepared.streams.size();
        result.prepared.streams.push_back(streamHandle);
        streamIndices.emplace(sampleIdentity.sampleSourceId, streamIndex);

        result.metrics.preparedBytes += streamHandle.payloadSizeBytes;
    }

    for (std::size_t index = 0; index < snapshotResult.snapshot.zones.size(); ++index)
    {
        const auto& zone = snapshotResult.snapshot.zones[index];
        const auto sampleIterator = sampleIndices.find(zone.sampleSourceId);
        const auto streamIterator = streamIndices.find(zone.sampleSourceId);
        if (sampleIterator == sampleIndices.end() || streamIterator == streamIndices.end())
        {
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "missing-prepared-zone-sample",
                       "zones[" + std::to_string(index) + "].sampleSourceId",
                       "Prepared playback could not bind zone '" + zone.id + "' to a prepared sample handle.");
            continue;
        }

        result.prepared.zones.push_back({
            zone.id,
            zone.sampleSourceId,
            result.prepared.samples[sampleIterator->second].streamSampleId,
            sampleIterator->second,
            streamIterator->second,
            zone.rootKey,
            zone.keyLow,
            zone.keyHigh,
            zone.velocityLow,
            zone.velocityHigh,
            zone.gainDb,
            zone.pan,
            zone.sampleStartFrame,
            zone.loopEnabled,
            zone.loopStartFrame,
            zone.loopEndFrame
        });
    }

    result.metrics.preparedSampleCount = result.prepared.samples.size();
    result.metrics.preparedStreamCount = result.prepared.streams.size();
    result.metrics.preparedZoneCount = result.prepared.zones.size();

    if (!snapshotResult.snapshot.contentDigest.empty())
        result.prepared.notes.push_back("Snapshot digest: " + snapshotResult.snapshot.contentDigest);
    result.prepared.notes.push_back("Compiler version: " + compilerVersion);

    const auto errorCount = static_cast<std::size_t>(std::count_if(result.findings.begin(),
                                                                   result.findings.end(),
                                                                   [](const PlaybackSnapshotFinding& finding)
                                                                   {
                                                                       return finding.severity == PlaybackSnapshotFindingSeverity::error;
                                                                   }));
    result.built = errorCount == 0;
    result.activationEligible = errorCount == 0;
    result.lifecycleState = errorCount == 0
        ? PlaybackSnapshotLifecycleState::ready
        : PlaybackSnapshotLifecycleState::failed;
    result.state = errorCount == 0
        ? "Prepared playback ready"
        : "Prepared playback failed";
    result.metrics.failureCount = errorCount == 0 ? 0 : 1;

    if (result.built)
        result.prepared.preparedContentDigest = "fnv1a64:" + computeFnv1a64Hex(serializePrepared(result.prepared, false).dump());

    result.buildDurationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
    return result;
}

PreparedPlaybackBuildResult PreparedPlaybackService::cancelBuild(const PreparedPlaybackBuildRequest& request,
                                                                 const std::string& state) const
{
    PreparedPlaybackBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = request.cancellationId;
    result.snapshotBuildId = request.snapshotBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lifecycleState = PlaybackSnapshotLifecycleState::canceled;
    result.state = state;
    result.metrics.cancellationCount = 1;
    return result;
}

PreparedPlaybackBuildResult PreparedPlaybackService::supersedeBuild(const PreparedPlaybackBuildRequest& request,
                                                                    std::uint64_t replacementBuildId,
                                                                    const std::string& state) const
{
    PreparedPlaybackBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = replacementBuildId;
    result.snapshotBuildId = request.snapshotBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lifecycleState = PlaybackSnapshotLifecycleState::superseded;
    result.state = state;
    return result;
}

void PreparedPlaybackService::setBackgroundWorkerStream(const RuntimeStreamLoadResult& streamResult)
{
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        workerStreamResult = streamResult;
    }

    workerCondition.notify_all();
}

PreparedPlaybackQueueSubmitResult PreparedPlaybackService::enqueueBuild(const PlaybackSnapshotBuildResult& snapshotResult,
                                                                       PreparedPlaybackWorkLane lane,
                                                                       PreparedPlaybackJobPriority priority)
{
    PreparedPlaybackQueueSubmitResult submitResult;
    submitResult.request = requestBuild(snapshotResult);
    submitResult.state = submitResult.request.state;
    std::lock_guard<std::mutex> lock(workerMutex);

    if (!submitResult.request.accepted)
    {
        refreshWorkerStatus();
        return submitResult;
    }

    for (auto iterator = queuedJobs.begin(); iterator != queuedJobs.end();)
    {
        if (iterator->lane == lane)
        {
            submitResult.displacedResults.push_back(
                supersedeBuild(iterator->request,
                               submitResult.request.buildId,
                               "Prepared playback build superseded by a newer " + toString(lane) + " request"));
            iterator = queuedJobs.erase(iterator);
            ++workerStatus.supersededCount;
            continue;
        }

        ++iterator;
    }

    if (queuedJobs.size() >= maxPendingJobs)
    {
        submitResult.request.accepted = false;
        submitResult.request.state = "Prepared playback queue is full";
        submitResult.request.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        submitResult.state = submitResult.request.state;
        refreshWorkerStatus();
        return submitResult;
    }

    queuedJobs.push_back({
        lane,
        priority,
        submitResult.request,
        snapshotResult,
        nextQueueOrdinal++
    });
    submitResult.accepted = true;
    submitResult.state = "Prepared playback build queued";
    workerStatus.lastEvent = submitResult.state;
    refreshWorkerStatus();
    workerCondition.notify_all();
    return submitResult;
}

PreparedPlaybackWorkerStepResult PreparedPlaybackService::processNextQueuedBuild(const RuntimeStreamLoadResult& streamResult)
{
    if (backgroundWorkerEnabled)
        return {};

    PreparedPlaybackWorkerStepResult stepResult;
    QueuedJob job;

    {
        std::lock_guard<std::mutex> lock(workerMutex);
        if (queuedJobs.empty())
        {
            refreshWorkerStatus();
            return stepResult;
        }

        const auto selectedIterator = std::min_element(
            queuedJobs.begin(),
            queuedJobs.end(),
            [](const QueuedJob& left, const QueuedJob& right)
            {
                if (left.priority != right.priority)
                    return static_cast<int>(left.priority) > static_cast<int>(right.priority);

                return left.enqueueOrdinal < right.enqueueOrdinal;
            });

        job = *selectedIterator;
        queuedJobs.erase(selectedIterator);
        workerStatus.inFlightWorkCount = 1;
        refreshWorkerStatus();
    }

    stepResult = processQueuedJob(job, streamResult);

    {
        std::lock_guard<std::mutex> lock(workerMutex);
        ++workerStatus.completedWorkCount;
        workerStatus.failureCount += stepResult.result.metrics.failureCount;
        workerStatus.lastEvent = stepResult.result.state;
        workerStatus.inFlightWorkCount = 0;
        refreshWorkerStatus();
    }

    workerIdleCondition.notify_all();
    return stepResult;
}

std::vector<PreparedPlaybackWorkerStepResult> PreparedPlaybackService::drainCompletedBuilds()
{
    std::lock_guard<std::mutex> lock(workerMutex);
    auto results = std::move(completedResults);
    completedResults.clear();
    return results;
}

std::vector<PreparedPlaybackBuildResult> PreparedPlaybackService::cancelQueuedBuilds(
    PreparedPlaybackWorkLane lane,
    const std::string& state)
{
    std::vector<PreparedPlaybackBuildResult> results;
    std::lock_guard<std::mutex> lock(workerMutex);

    for (auto iterator = queuedJobs.begin(); iterator != queuedJobs.end();)
    {
        if (iterator->lane == lane)
        {
            results.push_back(cancelBuild(iterator->request, state));
            iterator = queuedJobs.erase(iterator);
            ++workerStatus.cancellationCount;
            continue;
        }

        ++iterator;
    }

    if (!results.empty())
        workerStatus.lastEvent = state;

    refreshWorkerStatus();
    workerIdleCondition.notify_all();
    return results;
}

std::size_t PreparedPlaybackService::retireStaleCacheEntries(std::size_t maxEntries)
{
    std::size_t retiredCount = 0;
    std::lock_guard<std::mutex> lock(workerMutex);
    while (!retiredCacheEntries.empty() && retiredCount < maxEntries)
    {
        const auto& entry = retiredCacheEntries.back();
        if (workerStatus.retiredBytesAwaitingCleanup >= entry.second.retainedBytes)
            workerStatus.retiredBytesAwaitingCleanup -= entry.second.retainedBytes;
        else
            workerStatus.retiredBytesAwaitingCleanup = 0;

        retiredCacheEntries.pop_back();
        ++retiredCount;
    }

    if (retiredCount != 0)
        workerStatus.lastEvent = "Retired " + std::to_string(retiredCount) + " stale prepared cache entr"
            + (retiredCount == 1 ? "y" : "ies");

    refreshWorkerStatus();
    return retiredCount;
}

bool PreparedPlaybackService::waitForWorkerIdle(std::uint64_t timeoutMillis)
{
    std::unique_lock<std::mutex> lock(workerMutex);
    const auto timeout = std::chrono::milliseconds(timeoutMillis);
    return workerIdleCondition.wait_for(
        lock,
        timeout,
        [this]
        {
            return (queuedJobs.empty() && workerStatus.inFlightWorkCount == 0) || stopWorkerRequested;
        });
}

PreparedPlaybackWorkerStepResult PreparedPlaybackService::processQueuedJob(const QueuedJob& job,
                                                                           const RuntimeStreamLoadResult& streamResult)
{
    PreparedPlaybackWorkerStepResult stepResult;
    stepResult.processed = true;
    stepResult.lane = job.lane;
    stepResult.priority = job.priority;
    stepResult.result = prepare(job.request, job.snapshotResult, streamResult);
    return stepResult;
}

void PreparedPlaybackService::runBackgroundWorker()
{
    for (;;)
    {
        QueuedJob job;
        RuntimeStreamLoadResult streamResult;

        {
            std::unique_lock<std::mutex> lock(workerMutex);
            workerCondition.wait(
                lock,
                [this]
                {
                    return stopWorkerRequested
                        || (!queuedJobs.empty() && workerStreamResult.loaded);
                });

            if (stopWorkerRequested)
                break;

            const auto selectedIterator = std::min_element(
                queuedJobs.begin(),
                queuedJobs.end(),
                [](const QueuedJob& left, const QueuedJob& right)
                {
                    if (left.priority != right.priority)
                        return static_cast<int>(left.priority) > static_cast<int>(right.priority);

                    return left.enqueueOrdinal < right.enqueueOrdinal;
                });

            job = *selectedIterator;
            queuedJobs.erase(selectedIterator);
            streamResult = workerStreamResult;
            workerStatus.inFlightWorkCount = 1;
            workerStatus.lastEvent = "Prepared playback worker processing " + toString(job.lane) + " request";
            refreshWorkerStatus();
        }

        auto stepResult = processQueuedJob(job, streamResult);

        {
            std::lock_guard<std::mutex> lock(workerMutex);
            completedResults.push_back(stepResult);
            ++workerStatus.completedWorkCount;
            workerStatus.failureCount += stepResult.result.metrics.failureCount;
            workerStatus.inFlightWorkCount = 0;
            workerStatus.lastEvent = stepResult.result.state;
            refreshWorkerStatus();
        }

        workerIdleCondition.notify_all();
    }
}

void PreparedPlaybackService::refreshWorkerStatus()
{
    workerStatus.pendingWorkCount = queuedJobs.size();
    workerStatus.maxPendingWorkCount = std::max(workerStatus.maxPendingWorkCount, workerStatus.pendingWorkCount);
}

void PreparedPlaybackService::retireSupersededCacheEntries(const std::string& sampleSourceId, const std::string& cacheKey)
{
    for (auto iterator = cacheEntries.begin(); iterator != cacheEntries.end();)
    {
        if (iterator->first != cacheKey && iterator->second.sample.sampleSourceId == sampleSourceId)
        {
            workerStatus.retiredBytesAwaitingCleanup += iterator->second.retainedBytes;
            retiredCacheEntries.push_back(*iterator);
            iterator = cacheEntries.erase(iterator);
            continue;
        }

        ++iterator;
    }
}

std::string serializeImmutablePreparedPlayback(const ImmutablePreparedPlayback& prepared)
{
    return serializePrepared(prepared, true).dump(2) + "\n";
}

std::string toString(PreparedPlaybackWorkLane lane)
{
    switch (lane)
    {
    case PreparedPlaybackWorkLane::preview:
        return "preview";
    case PreparedPlaybackWorkLane::performance:
        return "publish";
    }

    return "unknown";
}

std::string toString(PreparedPlaybackJobPriority priority)
{
    switch (priority)
    {
    case PreparedPlaybackJobPriority::preview:
        return "preview";
    case PreparedPlaybackJobPriority::performance:
        return "publish";
    }

    return "unknown";
}
} // namespace drs::engine
