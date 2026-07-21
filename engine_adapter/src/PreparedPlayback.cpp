#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/SampleImport.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <numeric>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using ordered_json = nlohmann::ordered_json;

std::uint64_t clockMicros() noexcept
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());
}

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

    if (includeDigest)
    {
        root["routeDigest"] = prepared.routeDigest;
        root["sourceProvenanceDigest"] = prepared.sourceProvenanceDigest;
        root["macroSchemaDigest"] = prepared.macroSchemaDigest;
    }

    ordered_json ownershipRecords = ordered_json::array();
    for (const auto& ownership : prepared.ownershipRecords)
    {
        ordered_json ownershipObject;
        ownershipObject["ownershipToken"] = ownership.ownershipToken;
        ownershipObject["retirementToken"] = ownership.retirementToken;
        ownershipObject["cacheKey"] = ownership.cacheKey;
        ownershipObject["sampleSourceId"] = ownership.sampleSourceId;
        ownershipObject["streamSampleId"] = ownership.streamSampleId;
        ownershipObject["lifetimeState"] = ownership.lifetimeState;
        ownershipObject["retainedBytes"] = ownership.retainedBytes;
        ownershipObject["preparedBuildId"] = ownership.preparedBuildId;
        ownershipObject["retiredByBuildId"] = ownership.retiredByBuildId;
        ownershipRecords.push_back(std::move(ownershipObject));
    }
    root["ownershipRecords"] = std::move(ownershipRecords);

    ordered_json samples = ordered_json::array();
    for (const auto& sample : prepared.samples)
    {
        ordered_json sampleObject;
        sampleObject["sampleSourceId"] = sample.sampleSourceId;
        sampleObject["streamSampleId"] = sample.streamSampleId;
        sampleObject["sourcePath"] = sample.sourcePath;
        sampleObject["canonicalSourcePath"] = sample.canonicalSourcePath;
        sampleObject["canonicalSourceIdentity"] = sample.canonicalSourceIdentity;
        sampleObject["sourceFingerprintHex"] = sample.sourceFingerprintHex;
        sampleObject["formatName"] = sample.formatName;
        sampleObject["role"] = sample.role;
        sampleObject["channelLayout"] = sample.channelLayout;
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
        sampleObject["ownershipRecordIndex"] = sample.ownershipRecordIndex;
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
        streamObject["topologyKind"] = streamHandle.topologyKind;
        streamObject["compiledStreamTopologyAvailable"] = streamHandle.compiledStreamTopologyAvailable;
        streamObject["pageSizeBytes"] = streamHandle.pageSizeBytes;
        streamObject["payloadOffsetBytes"] = streamHandle.payloadOffsetBytes;
        streamObject["payloadSizeBytes"] = streamHandle.payloadSizeBytes;
        streamObject["prefetchBytes"] = streamHandle.prefetchBytes;
        streamObject["streamedPayloadOffsetBytes"] = streamHandle.streamedPayloadOffsetBytes;
        streamObject["streamedPayloadBytes"] = streamHandle.streamedPayloadBytes;
        streamObject["pageCount"] = streamHandle.pageCount;
        streamObject["pageRangePresent"] = streamHandle.pageRangePresent;
        streamObject["firstPageIndex"] = streamHandle.firstPageIndex;
        streamObject["lastPageIndex"] = streamHandle.lastPageIndex;
        streamObject["firstPageOffsetBytes"] = streamHandle.firstPageOffsetBytes;
        streamObject["lastPageOffsetBytes"] = streamHandle.lastPageOffsetBytes;
        streamObject["lastPageSizeBytes"] = streamHandle.lastPageSizeBytes;
        streamObject["ownershipToken"] = streamHandle.ownershipToken;
        streamObject["cacheKey"] = streamHandle.cacheKey;
        streamObject["ownershipRecordIndex"] = streamHandle.ownershipRecordIndex;

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
        if (zone.triggerMode == ZoneTriggerMode::oneShot)
            zoneObject["triggerMode"] = "one-shot";
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

std::string buildCanonicalSourceIdentity(const std::string& sampleSourceId, const std::string& canonicalSourcePath)
{
    return sampleSourceId + "|" + canonicalSourcePath;
}

std::string buildPreparedDecodePolicyFingerprint(const RuntimeStreamSampleDefinition& streamSample,
                                                 const RuntimeStreamContainerModel& container)
{
    std::ostringstream stream;
    stream << "format=" << streamSample.formatName
           << "|sampleRate=" << static_cast<int>(streamSample.sampleRate)
           << "|channelCount=" << streamSample.channelCount
           << "|channelLayout=" << streamSample.channelLayout
           << "|loopRangePresent=" << (streamSample.loopRangePresent ? "true" : "false")
           << "|loopStartFrame=" << streamSample.loopStartFrame
           << "|loopEndFrame=" << streamSample.loopEndFrame
           << "|payloadEncoding=" << container.payloadEncoding
           << "|pageSize=" << container.pageSizeBytes;
    return stream.str();
}

std::string buildAuthoredDecodePolicyFingerprint()
{
    return "format=auto|normalization=float32-normalized-v1|payloadEncoding=decoded-float32|topology=decoded-memory";
}

std::string buildRetirementToken(std::uint64_t retirementOrdinal,
                                 const std::string& sampleSourceId,
                                 std::uint64_t retiredByBuildId)
{
    return "retire:" + std::to_string(retirementOrdinal) + ":" + sampleSourceId + ":" + std::to_string(retiredByBuildId);
}

std::uint64_t computeStreamedPayloadBytes(const RuntimeStreamSampleDefinition& streamSample)
{
    return streamSample.payloadSizeBytes >= streamSample.prefetchBytes
        ? streamSample.payloadSizeBytes - streamSample.prefetchBytes
        : 0;
}

void populateStreamTopologyMetadata(PreparedPlaybackStreamHandle& streamHandle,
                                    const RuntimeStreamSampleDefinition& streamSample,
                                    std::uint64_t containerPageSizeBytes)
{
    streamHandle.streamedPayloadOffsetBytes = streamSample.payloadOffsetBytes + streamSample.prefetchBytes;
    streamHandle.streamedPayloadBytes = computeStreamedPayloadBytes(streamSample);

    const auto computedPageCount = containerPageSizeBytes == 0
        ? static_cast<std::size_t>(0)
        : static_cast<std::size_t>(
              (streamHandle.streamedPayloadBytes + containerPageSizeBytes - 1) / containerPageSizeBytes);
    const auto hasExplicitTopology = !streamSample.pages.empty() || computedPageCount == 0;

    streamHandle.topologyKind = hasExplicitTopology ? "explicit-pages" : "bounded-fallback";
    streamHandle.pageCount = hasExplicitTopology ? streamSample.pages.size() : computedPageCount;

    if (streamHandle.pageCount == 0)
        return;

    streamHandle.pageRangePresent = true;

    if (hasExplicitTopology)
    {
        streamHandle.firstPageIndex = streamSample.pages.front().pageIndex;
        streamHandle.lastPageIndex = streamSample.pages.back().pageIndex;
        streamHandle.firstPageOffsetBytes = streamSample.pages.front().offsetBytes;
        streamHandle.lastPageOffsetBytes = streamSample.pages.back().offsetBytes;
        streamHandle.lastPageSizeBytes = streamSample.pages.back().sizeBytes;
        return;
    }

    streamHandle.firstPageIndex = 0;
    streamHandle.lastPageIndex = static_cast<std::uint32_t>(streamHandle.pageCount - 1);
    streamHandle.firstPageOffsetBytes = streamHandle.streamedPayloadOffsetBytes;
    streamHandle.lastPageOffsetBytes =
        streamHandle.firstPageOffsetBytes + (static_cast<std::uint64_t>(streamHandle.pageCount - 1) * containerPageSizeBytes);
    const auto fullPagesBytes = static_cast<std::uint64_t>(streamHandle.pageCount - 1) * containerPageSizeBytes;
    streamHandle.lastPageSizeBytes = std::min(containerPageSizeBytes,
                                              streamHandle.streamedPayloadBytes - fullPagesBytes);
}

std::string buildCacheKey(const std::string& compilerVersion,
                          const PreparedPlaybackSampleResolution& sampleResolution,
                          const std::string& sourceFingerprint,
                          const std::string& decodePolicyFingerprint)
{
    // Cache identity intentionally excludes zone-only authoring fields such as gain, pan, and key mapping.
    // Those edits should change prepared zone content, but they must not invalidate source-backed prepared assets.
    const auto canonicalSourceIdentity = buildCanonicalSourceIdentity(sampleResolution.sampleSourceId,
                                                                      sampleResolution.normalizedSourcePath);
    std::ostringstream stream;
    stream << "compilerVersion=" << compilerVersion
           << "|canonicalSourceIdentity=" << canonicalSourceIdentity
           << "|sourceFingerprint=" << sourceFingerprint
           << "|decodePolicy=" << decodePolicyFingerprint;
    return "fnv1a64:" + computeFnv1a64Hex(stream.str());
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return {};

    if (issues.size() == 1)
        return issues.front();

    return issues.front() + " (+" + std::to_string(issues.size() - 1) + " more)";
}

struct PreparedSampleImportFailure
{
    std::string code;
    std::string summary;
};

PreparedSampleImportFailure classifyPreparedSampleImportFailure(const SampleImportResult& result)
{
    if (!result.fileFound || result.state == "Sample missing")
    {
        return {
            "prepared-sample-source-missing",
            "Prepared playback source sample is missing"
        };
    }

    if (result.state == "Sample format unsupported")
    {
        return {
            "prepared-sample-format-unsupported",
            "Prepared playback source sample uses an unsupported format"
        };
    }

    return {
        "prepared-sample-decode-failed",
        "Prepared playback source sample failed decode or policy validation"
    };
}

std::string describePreparedSampleImportFailure(const SampleImportResult& result)
{
    if (result.state.empty())
        return summarizeIssues(result.issues);

    if (result.issues.empty())
        return result.state;

    return result.state + " - " + summarizeIssues(result.issues);
}

std::uint64_t computeNormalizedChannelBytes(const std::vector<std::vector<float>>& normalizedChannels)
{
    return std::accumulate(
        normalizedChannels.begin(),
        normalizedChannels.end(),
        std::uint64_t{0},
        [](std::uint64_t total, const std::vector<float>& channel)
        {
            return total + (static_cast<std::uint64_t>(channel.size()) * static_cast<std::uint64_t>(sizeof(float)));
        });
}

std::uint64_t computeDecodedSampleBytes(const ImportedSampleData& sample)
{
    return computeNormalizedChannelBytes(sample.normalizedChannels);
}

std::uint64_t computePreparedSampleDataBytes(const PreparedPlaybackSampleHandle& sample)
{
    return sample.decodedSampleData != nullptr
        ? computeNormalizedChannelBytes(sample.decodedSampleData->normalizedChannels)
        : 0;
}

std::uint64_t computePreparedRetainedBytes(const PreparedPlaybackSampleHandle& sample)
{
    return computePreparedSampleDataBytes(sample);
}

bool equalDecodedSampleData(const std::shared_ptr<const PreparedPlaybackDecodedSampleData>& left,
                            const std::shared_ptr<const PreparedPlaybackDecodedSampleData>& right)
{
    if (left == right)
        return true;

    if (left == nullptr || right == nullptr)
        return false;

    return left->normalizedChannels == right->normalizedChannels;
}

std::vector<std::string> collectDecodeMismatches(const ImportedSampleMetadata& decoded,
                                                 const RuntimeStreamSampleDefinition& streamSample)
{
    std::vector<std::string> mismatches;

    if (decoded.sourceChecksumHex != streamSample.sourceChecksumHex)
    {
        mismatches.push_back("checksum decoded=" + decoded.sourceChecksumHex
                             + " compiled=" + streamSample.sourceChecksumHex);
    }

    if (decoded.formatName != streamSample.formatName)
        mismatches.push_back("format decoded=" + decoded.formatName + " compiled=" + streamSample.formatName);

    if (decoded.sampleRate != streamSample.sampleRate)
    {
        mismatches.push_back("sample rate decoded=" + std::to_string(static_cast<int>(decoded.sampleRate))
                             + " compiled=" + std::to_string(static_cast<int>(streamSample.sampleRate)));
    }

    if (decoded.frameCount != streamSample.frameCount)
    {
        mismatches.push_back("frame count decoded=" + std::to_string(decoded.frameCount)
                             + " compiled=" + std::to_string(streamSample.frameCount));
    }

    if (decoded.channelCount != streamSample.channelCount)
    {
        mismatches.push_back("channel count decoded=" + std::to_string(decoded.channelCount)
                             + " compiled=" + std::to_string(streamSample.channelCount));
    }

    return mismatches;
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
                                                 bool enableBackgroundWorkerIn,
                                                 PreparedPlaybackSchedulerBudgets schedulerBudgetsIn)
    : compilerVersion(std::move(compilerVersionIn)),
      maxPendingJobs(maxPendingJobsIn),
      schedulerBudgets(std::move(schedulerBudgetsIn)),
      backgroundWorkerEnabled(enableBackgroundWorkerIn)
{
    maxPendingJobs = std::max<std::size_t>(
        1, std::min(maxPendingJobs, std::max<std::size_t>(1, schedulerBudgets.maximumPendingJobs)));
    schedulerBudgets.maximumPendingJobs = maxPendingJobs;
    schedulerBudgets.maximumInFlightJobs = 1;
    schedulerBudgets.maximumCompletedResults = std::max<std::size_t>(1, schedulerBudgets.maximumCompletedResults);
    schedulerBudgets.maximumConsecutivePerformanceJobs =
        std::max<std::size_t>(1, schedulerBudgets.maximumConsecutivePerformanceJobs);
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
    request.buildId = nextBuildId.fetch_add(1, std::memory_order_relaxed);
    request.cancellationId = request.buildId;
    request.lifecycleState = PlaybackSnapshotLifecycleState::preparing;
    request.state = "Prepared playback build queued";
    return request;
}

PreparedPlaybackBuildRequest PreparedPlaybackService::requestBuild(const PlaybackSnapshotBuildResult& snapshotResult,
                                                                  const RuntimeStreamLoadResult& streamResult)
{
    return resolveBuildRequest(requestBuild(snapshotResult), snapshotResult, streamResult);
}

PreparedPlaybackBuildRequest PreparedPlaybackService::resolveBuildRequest(
    const PreparedPlaybackBuildRequest& request,
    const PlaybackSnapshotBuildResult& snapshotResult,
    const RuntimeStreamLoadResult& streamResult) const
{
    auto resolvedRequest = request;
    resolvedRequest.sampleResolutions.clear();
    resolvedRequest.sampleResolutionReady = false;

    if (!resolvedRequest.accepted)
        return resolvedRequest;

    resolvedRequest.sampleResolutions.reserve(snapshotResult.snapshot.sampleIdentities.size());

    for (std::size_t index = 0; index < snapshotResult.snapshot.sampleIdentities.size(); ++index)
    {
        const auto& sampleIdentity = snapshotResult.snapshot.sampleIdentities[index];
        PreparedPlaybackSampleResolution resolution;
        resolution.snapshotSampleIndex = index;
        resolution.sampleSourceId = sampleIdentity.sampleSourceId;
        resolution.normalizedSourcePath = normalizePath(sampleIdentity.sourcePath);

        if (const auto* streamSample = findStreamSampleByPath(streamResult.container, resolution.normalizedSourcePath))
        {
            resolution.selectedStreamSampleId = streamSample->sampleId;
            resolution.selectedFormatName = streamSample->formatName;
            resolution.matchedBySourcePath = true;
            resolution.resolutionKind = "compiled-stream-path";
            resolution.compiledStreamTopologyAvailable = true;
        }
        else if (!sampleIdentity.sampleSourceId.empty())
        {
            if (const auto* streamSampleById = findStreamSampleById(streamResult.container, sampleIdentity.sampleSourceId))
            {
                resolution.selectedStreamSampleId = streamSampleById->sampleId;
                resolution.selectedFormatName = streamSampleById->formatName;
                resolution.matchedBySampleSourceId = true;
                resolution.resolutionKind = "compiled-stream-id";
                resolution.compiledStreamTopologyAvailable = true;
            }
        }

        resolvedRequest.sampleResolutions.push_back(std::move(resolution));
    }

    resolvedRequest.sampleResolutionReady = true;
    return resolvedRequest;
}

PreparedPlaybackQueueSubmitResult PreparedPlaybackService::enqueuePreviewBuild(
    const PlaybackSnapshotBuildResult& snapshotResult)
{
    return enqueueBuildForLane(snapshotResult,
                               PreparedPlaybackWorkLane::preview,
                               PreparedPlaybackJobPriority::preview);
}

PreparedPlaybackQueueSubmitResult PreparedPlaybackService::enqueuePublishBuild(
    const PlaybackSnapshotBuildResult& snapshotResult)
{
    return enqueueBuildForLane(snapshotResult,
                               PreparedPlaybackWorkLane::performance,
                               PreparedPlaybackJobPriority::performance);
}

PreparedPlaybackBuildResult PreparedPlaybackService::prepare(const PreparedPlaybackBuildRequest& request,
                                                             const PlaybackSnapshotBuildResult& snapshotResult,
                                                             const RuntimeStreamLoadResult& streamResult)
{
    // Sprint 3 boundary note: this service is the intended playback-preparation seam.
    // Preview/Publish may build immutable snapshots before this point, but prepared asset realization, cache
    // ownership, and retirement policy should converge here rather than spreading new decode paths into the shell.
    PreparedPlaybackBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = request.cancellationId;
    result.cancellationGeneration = request.cancellationGeneration;
    result.snapshotBuildId = request.snapshotBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lane = request.lane;
    result.priority = request.priority;
    result.pendingDepthAtSubmit = request.pendingDepthAtSubmit;
    result.runningDepthAtStart = 1;
    result.queueWaitMicros = request.queuedAtMicros == 0
        ? 0
        : clockMicros() - request.queuedAtMicros;
    result.lifecycleState = request.accepted
        ? PlaybackSnapshotLifecycleState::preparing
        : PlaybackSnapshotLifecycleState::failed;
    result.state = request.accepted
        ? "Prepared playback build in progress"
        : request.state;

    const auto startTime = Clock::now();
    std::lock_guard<std::mutex> lock(workerMutex);

    const auto finishCanceled = [&]()
    {
        for (auto iterator = cacheEntries.begin(); iterator != cacheEntries.end();)
        {
            if (iterator->second.ownership.preparedBuildId == request.buildId)
                iterator = cacheEntries.erase(iterator);
            else
                ++iterator;
        }
        result.built = false;
        result.activationEligible = false;
        result.lifecycleState = PlaybackSnapshotLifecycleState::canceled;
        result.completionDisposition = PreparedPlaybackCompletionDisposition::canceled;
        result.state = "Prepared playback build cooperatively canceled";
        result.metrics.cancellationCount = 1;
        result.prepared = {};
        addFinding(result,
                   PlaybackSnapshotFindingSeverity::error,
                   "prepared-build-cooperatively-canceled",
                   toString(request.lane),
                   "A newer request or explicit cancellation invalidated this in-flight build.");
        ++workerStatus.cooperativeCancellationCount;
        ++workerStatus.cancellationCount;
        result.buildDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
        result.requestToReadyMicros = request.queuedAtMicros == 0
            ? result.buildDurationMicros
            : clockMicros() - request.queuedAtMicros;
        refreshWorkerStatus();
        return result;
    };

    if (!request.accepted)
    {
        result.completionDisposition = PreparedPlaybackCompletionDisposition::rejected;
        result.metrics.failureCount = 1;
        result.findings = snapshotResult.findings;
        if (result.findings.empty())
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "prepared-build-rejected",
                       "snapshot",
                       "Prepared playback build requires a valid immutable playback snapshot.");
        result.buildDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
        return result;
    }

    if (isCancellationRequested(request))
        return finishCanceled();

    const auto resolvedRequest = request.sampleResolutionReady
        ? request
        : resolveBuildRequest(request, snapshotResult, streamResult);

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

    result.prepared.ownershipRecords.reserve(resolvedRequest.sampleResolutions.size());
    result.prepared.samples.reserve(resolvedRequest.sampleResolutions.size());
    result.prepared.streams.reserve(resolvedRequest.sampleResolutions.size());
    result.prepared.zones.reserve(snapshotResult.snapshot.zones.size());

    for (const auto& sampleResolution : resolvedRequest.sampleResolutions)
    {
        if (isCancellationRequested(request))
            return finishCanceled();
        const auto path = "sampleIdentities[" + std::to_string(sampleResolution.snapshotSampleIndex) + "]";
        const auto* candidateStreamSample = !streamResult.loaded || sampleResolution.selectedStreamSampleId.empty()
            ? nullptr
            : findStreamSampleById(streamResult.container, sampleResolution.selectedStreamSampleId);

        const auto fingerprint = fingerprintSampleSourceFile(sampleResolution.normalizedSourcePath);
        if (isCancellationRequested(request))
            return finishCanceled();
        if (!fingerprint.fingerprinted)
        {
            SampleImportResult fingerprintFailure;
            fingerprintFailure.fileFound = fingerprint.fileFound;
            fingerprintFailure.sourcePath = fingerprint.sourcePath;
            fingerprintFailure.state = fingerprint.state;
            fingerprintFailure.issues = fingerprint.issues;
            const auto failure = classifyPreparedSampleImportFailure(fingerprintFailure);
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       failure.code,
                       path + ".sourcePath",
                       failure.summary + " '" + sampleResolution.normalizedSourcePath + "': "
                           + describePreparedSampleImportFailure(fingerprintFailure));
            continue;
        }

        // A compiled stream sample is optional. Reuse it only when it describes the bytes that
        // were actually fingerprinted by this worker; stale or absent topology falls back to a
        // decoded-memory handle without making the authored source ineligible.
        const auto* streamSample = candidateStreamSample != nullptr
                && candidateStreamSample->sourceChecksumHex == fingerprint.fingerprintHex
            ? candidateStreamSample
            : nullptr;
        const auto decodePolicyFingerprint = streamSample != nullptr
            ? buildPreparedDecodePolicyFingerprint(*streamSample, streamResult.container)
            : buildAuthoredDecodePolicyFingerprint();

        const auto cacheKey = buildCacheKey(compilerVersion,
                                            sampleResolution,
                                            fingerprint.fingerprintHex,
                                            decodePolicyFingerprint);
        const auto cacheIterator = std::find_if(cacheEntries.begin(),
                                                cacheEntries.end(),
                                                [&](const auto& entry)
                                                {
                                                    return entry.first == cacheKey;
                                                });
        const auto cacheHit = cacheIterator != cacheEntries.end();

        PreparedPlaybackSampleHandle sampleHandle;
        PreparedPlaybackStreamHandle streamHandle;

        if (cacheHit)
        {
            sampleHandle = cacheIterator->second.sample;
            streamHandle = cacheIterator->second.stream;
            ++result.metrics.cacheHitCount;
        }
        else
        {
            const auto decodedSample = importSampleFile(sampleResolution.normalizedSourcePath,
                                                        fingerprint.fingerprintHex);
            if (isCancellationRequested(request))
                return finishCanceled();
            if (!decodedSample.imported)
            {
                const auto failure = classifyPreparedSampleImportFailure(decodedSample);
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           failure.code,
                           path + ".sourcePath",
                           failure.summary + " '" + sampleResolution.normalizedSourcePath + "': "
                               + describePreparedSampleImportFailure(decodedSample));
                continue;
            }

            const auto decodeMismatches = streamSample != nullptr
                ? collectDecodeMismatches(decodedSample.sample.metadata, *streamSample)
                : std::vector<std::string> {};
            if (!decodeMismatches.empty())
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "prepared-sample-stream-mismatch",
                           path + ".sourcePath",
                           "Prepared playback decoded source sample '" + sampleResolution.normalizedSourcePath
                               + "', but the compiled stream metadata no longer matches: "
                               + summarizeIssues(decodeMismatches));
                continue;
            }

            sampleHandle.sampleSourceId = sampleResolution.sampleSourceId;
            sampleHandle.streamSampleId = streamSample != nullptr
                ? streamSample->sampleId
                : sampleResolution.sampleSourceId;
            sampleHandle.sourcePath = sampleResolution.normalizedSourcePath;
            sampleHandle.canonicalSourcePath = sampleResolution.normalizedSourcePath;
            sampleHandle.canonicalSourceIdentity = buildCanonicalSourceIdentity(sampleResolution.sampleSourceId,
                                                                                sampleResolution.normalizedSourcePath);
            sampleHandle.sourceFingerprintHex = decodedSample.sample.metadata.sourceChecksumHex;
            sampleHandle.formatName = decodedSample.sample.metadata.formatName;
            const auto& snapshotRole = snapshotResult.snapshot.sampleIdentities[sampleResolution.snapshotSampleIndex].role;
            sampleHandle.role = snapshotRole.empty() && streamSample != nullptr ? streamSample->role : snapshotRole;
            sampleHandle.channelLayout = streamSample != nullptr
                ? streamSample->channelLayout
                : decodedSample.sample.metadata.channelLayout;
            sampleHandle.sampleRate = decodedSample.sample.metadata.sampleRate;
            sampleHandle.frameCount = decodedSample.sample.metadata.frameCount;
            sampleHandle.channelCount = decodedSample.sample.metadata.channelCount;
            sampleHandle.rootMidiNotePresent = decodedSample.sample.metadata.rootMidiNotePresent;
            sampleHandle.rootMidiNote = decodedSample.sample.metadata.rootMidiNote;
            sampleHandle.loopRangePresent = decodedSample.sample.metadata.loopRangePresent;
            sampleHandle.loopStartFrame = decodedSample.sample.metadata.loopStartFrame;
            sampleHandle.loopEndFrame = decodedSample.sample.metadata.loopEndFrame;
            auto decodedSampleData = std::make_shared<PreparedPlaybackDecodedSampleData>();
            decodedSampleData->normalizedChannels = decodedSample.sample.normalizedChannels;
            sampleHandle.decodedSampleData = std::move(decodedSampleData);
            sampleHandle.ownershipToken = "cache:" + cacheKey;
            sampleHandle.cacheKey = cacheKey;

            streamHandle.sampleSourceId = sampleResolution.sampleSourceId;
            streamHandle.streamSampleId = sampleHandle.streamSampleId;
            streamHandle.compiledStreamTopologyAvailable = streamSample != nullptr;
            if (streamSample != nullptr)
            {
                streamHandle.containerId = streamResult.container.containerId;
                streamHandle.containerPath = streamResult.containerPath;
                streamHandle.payloadEncoding = streamResult.container.payloadEncoding;
                streamHandle.pageSizeBytes = streamResult.container.pageSizeBytes;
                streamHandle.payloadOffsetBytes = streamSample->payloadOffsetBytes;
                streamHandle.payloadSizeBytes = streamSample->payloadSizeBytes;
                streamHandle.prefetchBytes = streamSample->prefetchBytes;
                populateStreamTopologyMetadata(streamHandle, *streamSample, streamResult.container.pageSizeBytes);
            }
            else
            {
                streamHandle.payloadEncoding = "decoded-float32";
                streamHandle.topologyKind = "decoded-memory";
            }
            streamHandle.ownershipToken = "cache:" + cacheKey;
            streamHandle.cacheKey = cacheKey;
            if (streamSample != nullptr)
            {
                streamHandle.pages.reserve(streamSample->pages.size());
                for (const auto& page : streamSample->pages)
                    streamHandle.pages.push_back({ page.pageIndex, page.offsetBytes, page.sizeBytes });
            }

            retireSupersededCacheEntries(sampleResolution.sampleSourceId, cacheKey, request.buildId);

            const auto retainedSampleBytes = computePreparedRetainedBytes(sampleHandle);
            CacheEntry entry;
            entry.ownership.ownershipToken = sampleHandle.ownershipToken;
            entry.ownership.cacheKey = cacheKey;
            entry.ownership.sampleSourceId = sampleResolution.sampleSourceId;
            entry.ownership.streamSampleId = sampleHandle.streamSampleId;
            entry.ownership.lifetimeState = "active-cache-entry";
            entry.ownership.retainedBytes = retainedSampleBytes;
            entry.ownership.preparedBuildId = request.buildId;
            entry.sample = sampleHandle;
            entry.stream = streamHandle;
            entry.retainedBytes = retainedSampleBytes;
            cacheEntries.push_back({ cacheKey, std::move(entry) });
            ++result.metrics.cacheMissCount;
            result.metrics.decodedBytes += computeDecodedSampleBytes(decodedSample.sample);
        }

        const auto ownershipRecordIndex = result.prepared.ownershipRecords.size();
        const auto& ownershipRecord = cacheHit
            ? cacheIterator->second.ownership
            : cacheEntries.back().second.ownership;
        result.prepared.ownershipRecords.push_back(ownershipRecord);
        sampleHandle.ownershipRecordIndex = ownershipRecordIndex;
        streamHandle.ownershipRecordIndex = ownershipRecordIndex;

        const auto sampleIndex = result.prepared.samples.size();
        result.prepared.samples.push_back(sampleHandle);
        sampleIndices.emplace(sampleResolution.sampleSourceId, sampleIndex);

        const auto streamIndex = result.prepared.streams.size();
        result.prepared.streams.push_back(streamHandle);
        streamIndices.emplace(sampleResolution.sampleSourceId, streamIndex);

        const auto retainedSampleBytes = computePreparedRetainedBytes(sampleHandle);
        result.metrics.preparedBytes += retainedSampleBytes;
        result.metrics.preparedSampleDataBytes += computePreparedSampleDataBytes(sampleHandle);
    }

    for (std::size_t index = 0; index < snapshotResult.snapshot.zones.size(); ++index)
    {
        if (isCancellationRequested(request))
            return finishCanceled();
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
            zone.loopEndFrame,
            zone.triggerMode
        });
    }

    result.metrics.preparedSampleCount = result.prepared.samples.size();
    result.metrics.preparedStreamCount = result.prepared.streams.size();
    result.metrics.preparedZoneCount = result.prepared.zones.size();
    result.metrics.preparedOwnershipRecordCount = result.prepared.ownershipRecords.size();
    result.metrics.preparedOwnershipBytes = std::accumulate(
        result.prepared.ownershipRecords.begin(),
        result.prepared.ownershipRecords.end(),
        static_cast<std::uint64_t>(0),
        [](std::uint64_t total, const PreparedPlaybackOwnershipRecord& ownership)
        {
            return total + ownership.retainedBytes;
        });
    result.metrics.activeCachedOwnershipRecordCount = cacheEntries.size();
    result.metrics.retiredOwnershipRecordCount = retiredCacheEntries.size();
    result.metrics.retiredBytesAwaitingCleanup = workerStatus.retiredBytesAwaitingCleanup;

    if (!snapshotResult.snapshot.contentDigest.empty())
        result.prepared.notes.push_back("Snapshot digest: " + snapshotResult.snapshot.contentDigest);
    result.prepared.notes.push_back("Compiler version: " + compilerVersion);
    result.prepared.notes.push_back(
        "Prepared cache key contract: canonical-source-identity + source-fingerprint + decode-policy + compiler-version");
    result.prepared.notes.push_back(
        "Compiled stream topology is optional; unmatched authored sources retain decoded-memory handles");

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
    result.completionDisposition = errorCount == 0
        ? PreparedPlaybackCompletionDisposition::completed
        : PreparedPlaybackCompletionDisposition::failed;

    if (result.built)
    {
        if (isCancellationRequested(request))
            return finishCanceled();
        result.prepared.routeDigest = computePreparedPlaybackRouteDigest(snapshotResult.snapshot, result.prepared);
        result.prepared.sourceProvenanceDigest = computePreparedPlaybackSourceProvenanceDigest(result.prepared);
        result.prepared.macroSchemaDigest = computePlaybackSnapshotMacroSchemaDigest(snapshotResult.snapshot);
        result.prepared.preparedContentDigest = computePreparedPlaybackContentDigest(result.prepared);
        if (isCancellationRequested(request))
            return finishCanceled();
    }

    result.buildDurationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
    result.requestToReadyMicros = request.queuedAtMicros == 0
        ? result.buildDurationMicros
        : clockMicros() - request.queuedAtMicros;
    return result;
}

PreparedPlaybackBuildResult PreparedPlaybackService::cancelBuild(const PreparedPlaybackBuildRequest& request,
                                                                 const std::string& state) const
{
    PreparedPlaybackBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = request.cancellationId;
    result.cancellationGeneration = request.cancellationGeneration;
    result.snapshotBuildId = request.snapshotBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lane = request.lane;
    result.priority = request.priority;
    result.completionDisposition = PreparedPlaybackCompletionDisposition::canceled;
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
    result.cancellationGeneration = request.cancellationGeneration;
    result.snapshotBuildId = request.snapshotBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lane = request.lane;
    result.priority = request.priority;
    result.completionDisposition = PreparedPlaybackCompletionDisposition::superseded;
    result.lifecycleState = PlaybackSnapshotLifecycleState::superseded;
    result.state = state;
    return result;
}

void PreparedPlaybackService::setBackgroundWorkerStream(const RuntimeStreamLoadResult& streamResult)
{
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        workerStreamResult = streamResult;
        workerStreamConfigured = true;
    }

    workerCondition.notify_all();
}

PreparedPlaybackQueueSubmitResult PreparedPlaybackService::enqueueBuildForLane(
    const PlaybackSnapshotBuildResult& snapshotResult,
    PreparedPlaybackWorkLane lane,
    PreparedPlaybackJobPriority priority)
{
    const auto commandStartedAtMicros = clockMicros();
    auto& cancellationGeneration = lane == PreparedPlaybackWorkLane::performance
        ? performanceCancellationGeneration
        : previewCancellationGeneration;
    const auto nextCancellationGeneration =
        cancellationGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
    PreparedPlaybackQueueSubmitResult submitResult;
    submitResult.request = requestBuild(snapshotResult);
    submitResult.request.lane = lane;
    submitResult.request.priority = priority;
    submitResult.request.cancellationGeneration = nextCancellationGeneration;
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
            const auto supersedeReason =
                "Prepared playback build superseded by a newer " + toString(lane) + " request";
            submitResult.displacedResults.push_back(
                supersedeBuild(iterator->request,
                               submitResult.request.buildId,
                               supersedeReason));
            iterator = queuedJobs.erase(iterator);
            ++workerStatus.supersededCount;
            workerStatus.lastSupersededLane = toString(lane);
            workerStatus.lastSupersededReason = supersedeReason;
            continue;
        }

        ++iterator;
    }

    if (queuedJobs.size() >= maxPendingJobs)
    {
        const auto displacedIterator = selectQueuedJobToDisplaceForPriority(priority);
        if (displacedIterator != queuedJobs.end())
        {
            const auto supersedeReason =
                "Prepared playback build superseded by higher-priority " + toString(priority) + " request";
            submitResult.displacedResults.push_back(
                supersedeBuild(displacedIterator->request,
                               submitResult.request.buildId,
                               supersedeReason));
            workerStatus.lastSupersededLane = toString(displacedIterator->lane);
            workerStatus.lastSupersededReason = supersedeReason;
            queuedJobs.erase(displacedIterator);
            ++workerStatus.supersededCount;
        }
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

    const auto enqueueOrdinal = nextQueueOrdinal++;
    const auto queuedAtMicros = clockMicros();
    submitResult.request.enqueueOrdinal = enqueueOrdinal;
    submitResult.request.queuedAtMicros = queuedAtMicros;
    submitResult.request.pendingDepthAtSubmit = queuedJobs.size() + 1;
    submitResult.request.commandToQueuedMicros = queuedAtMicros - commandStartedAtMicros;
    submitResult.commandToQueuedMicros = submitResult.request.commandToQueuedMicros;
    queuedJobs.push_back({
        lane,
        priority,
        submitResult.request,
        snapshotResult,
        enqueueOrdinal
    });
    submitResult.accepted = true;
    submitResult.state = "Prepared playback build queued";
    if (completedResults.size() >= schedulerBudgets.maximumCompletedResults)
        ++workerStatus.completionBackpressureCount;
    workerStatus.lastEvent = submitResult.state;
    workerStatus.maxCommandToQueuedMicros = std::max(workerStatus.maxCommandToQueuedMicros,
                                                     submitResult.commandToQueuedMicros);
    if (submitResult.commandToQueuedMicros > schedulerBudgets.maximumCommandToQueuedMicros)
        ++workerStatus.commandToQueuedBudgetViolationCount;
    refreshWorkerStatus();
    workerCondition.notify_all();
    return submitResult;
}

std::vector<PreparedPlaybackService::QueuedJob>::iterator PreparedPlaybackService::selectNextQueuedJob()
{
    const auto previewIterator = std::find_if(
        queuedJobs.begin(), queuedJobs.end(), [](const QueuedJob& job)
        {
            return job.lane == PreparedPlaybackWorkLane::preview;
        });
    const auto previewPending = previewIterator != queuedJobs.end();
    if (previewPending
        && workerStatus.consecutivePerformanceDispatchCount
            >= schedulerBudgets.maximumConsecutivePerformanceJobs)
    {
        return previewIterator;
    }

    return std::min_element(
        queuedJobs.begin(), queuedJobs.end(), [](const QueuedJob& left, const QueuedJob& right)
        {
            if (left.priority != right.priority)
                return static_cast<int>(left.priority) > static_cast<int>(right.priority);
            return left.enqueueOrdinal < right.enqueueOrdinal;
        });
}

std::vector<PreparedPlaybackService::QueuedJob>::iterator
PreparedPlaybackService::selectQueuedJobToDisplaceForPriority(const PreparedPlaybackJobPriority incomingPriority)
{
    auto selectedIterator = queuedJobs.end();

    for (auto iterator = queuedJobs.begin(); iterator != queuedJobs.end(); ++iterator)
    {
        if (static_cast<int>(iterator->priority) >= static_cast<int>(incomingPriority))
            continue;

        if (selectedIterator == queuedJobs.end())
        {
            selectedIterator = iterator;
            continue;
        }

        if (iterator->priority != selectedIterator->priority)
        {
            if (static_cast<int>(iterator->priority) < static_cast<int>(selectedIterator->priority))
                selectedIterator = iterator;

            continue;
        }

        if (iterator->enqueueOrdinal < selectedIterator->enqueueOrdinal)
            selectedIterator = iterator;
    }

    return selectedIterator;
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

        const auto selectedIterator = selectNextQueuedJob();

        job = *selectedIterator;
        queuedJobs.erase(selectedIterator);
        workerStatus.inFlightWorkCount = 1;
        workerStatus.inFlightBuildId = job.request.buildId;
        workerStatus.inFlightLane = job.lane;
        if (job.lane == PreparedPlaybackWorkLane::performance)
        {
            ++workerStatus.performanceDispatchCount;
            ++workerStatus.consecutivePerformanceDispatchCount;
            workerStatus.maxConsecutivePerformanceDispatchCount = std::max(
                workerStatus.maxConsecutivePerformanceDispatchCount,
                workerStatus.consecutivePerformanceDispatchCount);
        }
        else
        {
            ++workerStatus.previewDispatchCount;
            workerStatus.consecutivePerformanceDispatchCount = 0;
        }
        refreshWorkerStatus();
    }

    stepResult = processQueuedJob(job, streamResult);

    {
        std::lock_guard<std::mutex> lock(workerMutex);
        ++workerStatus.completedWorkCount;
        workerStatus.failureCount += stepResult.result.metrics.failureCount;
        workerStatus.maxQueueWaitMicros = std::max(workerStatus.maxQueueWaitMicros,
                                                   stepResult.result.queueWaitMicros);
        workerStatus.maxRequestToReadyMicros = std::max(workerStatus.maxRequestToReadyMicros,
                                                        stepResult.result.requestToReadyMicros);
        if (stepResult.result.requestToReadyMicros > schedulerBudgets.maximumRequestToReadyMicros)
            ++workerStatus.requestToReadyBudgetViolationCount;
        workerStatus.lastEvent = stepResult.result.state;
        workerStatus.inFlightWorkCount = 0;
        workerStatus.inFlightBuildId = 0;
        refreshWorkerStatus();
    }

    workerIdleCondition.notify_all();
    return stepResult;
}

std::vector<PreparedPlaybackWorkerStepResult> PreparedPlaybackService::drainCompletedBuilds()
{
    std::vector<PreparedPlaybackWorkerStepResult> results;
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        results = std::move(completedResults);
        completedResults.clear();
        refreshWorkerStatus();
    }
    workerCondition.notify_all();
    return results;
}

std::vector<PreparedPlaybackBuildResult> PreparedPlaybackService::cancelQueuedPreviewBuilds(const std::string& state)
{
    previewCancellationGeneration.fetch_add(1, std::memory_order_acq_rel);
    return cancelQueuedBuildsForLane(PreparedPlaybackWorkLane::preview, state);
}

std::vector<PreparedPlaybackBuildResult> PreparedPlaybackService::cancelQueuedPublishBuilds(const std::string& state)
{
    performanceCancellationGeneration.fetch_add(1, std::memory_order_acq_rel);
    return cancelQueuedBuildsForLane(PreparedPlaybackWorkLane::performance, state);
}

std::vector<PreparedPlaybackBuildResult> PreparedPlaybackService::cancelQueuedBuildsForLane(
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
            workerStatus.lastCancellationLane = toString(lane);
            workerStatus.lastCancellationReason = state;
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

std::size_t PreparedPlaybackService::serviceRetiredCacheCleanup(std::size_t maxEntries)
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

std::size_t PreparedPlaybackService::retireStaleCacheEntries(std::size_t maxEntries)
{
    return serviceRetiredCacheCleanup(maxEntries);
}

std::vector<PreparedPlaybackOwnershipRecord> PreparedPlaybackService::snapshotRetiredOwnershipRecords() const
{
    std::vector<PreparedPlaybackOwnershipRecord> records;
    std::lock_guard<std::mutex> lock(workerMutex);
    records.reserve(retiredCacheEntries.size());

    for (const auto& entry : retiredCacheEntries)
        records.push_back(entry.second.ownership);

    return records;
}

PreparedPlaybackWorkerStatus PreparedPlaybackService::getWorkerStatus() const
{
    std::lock_guard<std::mutex> lock(workerMutex);
    return workerStatus;
}

void PreparedPlaybackService::recordCommandToQueuedDuration(std::uint64_t durationMicros)
{
    std::lock_guard<std::mutex> lock(workerMutex);
    workerStatus.maxCommandToQueuedMicros = std::max(workerStatus.maxCommandToQueuedMicros,
                                                     durationMicros);
    if (durationMicros > schedulerBudgets.maximumCommandToQueuedMicros)
        ++workerStatus.commandToQueuedBudgetViolationCount;
    refreshWorkerStatus();
}

void PreparedPlaybackService::recordMessageThreadServiceDuration(std::uint64_t durationMicros)
{
    std::lock_guard<std::mutex> lock(workerMutex);
    workerStatus.maxMessageThreadServiceMicros = std::max(workerStatus.maxMessageThreadServiceMicros,
                                                          durationMicros);
    if (durationMicros > schedulerBudgets.maximumMessageThreadServiceMicros)
        ++workerStatus.messageThreadServiceBudgetViolationCount;
    refreshWorkerStatus();
}

bool PreparedPlaybackService::hasPendingQueuedBuilds() const
{
    std::lock_guard<std::mutex> lock(workerMutex);
    return !queuedJobs.empty();
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
    const auto resolvedRequest = resolveBuildRequest(job.request, job.snapshotResult, streamResult);
    stepResult.result = prepare(resolvedRequest, job.snapshotResult, streamResult);
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
                        || (!queuedJobs.empty() && workerStreamConfigured
                            && completedResults.size() < schedulerBudgets.maximumCompletedResults);
                });

            if (stopWorkerRequested)
                break;

            const auto selectedIterator = selectNextQueuedJob();

            job = *selectedIterator;
            queuedJobs.erase(selectedIterator);
            streamResult = workerStreamResult;
            workerStatus.inFlightWorkCount = 1;
            workerStatus.inFlightBuildId = job.request.buildId;
            workerStatus.inFlightLane = job.lane;
            if (job.lane == PreparedPlaybackWorkLane::performance)
            {
                ++workerStatus.performanceDispatchCount;
                ++workerStatus.consecutivePerformanceDispatchCount;
                workerStatus.maxConsecutivePerformanceDispatchCount = std::max(
                    workerStatus.maxConsecutivePerformanceDispatchCount,
                    workerStatus.consecutivePerformanceDispatchCount);
            }
            else
            {
                ++workerStatus.previewDispatchCount;
                workerStatus.consecutivePerformanceDispatchCount = 0;
            }
            workerStatus.lastEvent = "Prepared playback worker processing " + toString(job.lane) + " request";
            refreshWorkerStatus();
        }

        auto stepResult = processQueuedJob(job, streamResult);

        {
            std::lock_guard<std::mutex> lock(workerMutex);
            completedResults.push_back(stepResult);
            ++workerStatus.completedWorkCount;
            workerStatus.failureCount += stepResult.result.metrics.failureCount;
            workerStatus.maxQueueWaitMicros = std::max(workerStatus.maxQueueWaitMicros,
                                                       stepResult.result.queueWaitMicros);
            workerStatus.maxRequestToReadyMicros = std::max(workerStatus.maxRequestToReadyMicros,
                                                            stepResult.result.requestToReadyMicros);
            if (stepResult.result.requestToReadyMicros > schedulerBudgets.maximumRequestToReadyMicros)
                ++workerStatus.requestToReadyBudgetViolationCount;
            workerStatus.inFlightWorkCount = 0;
            workerStatus.inFlightBuildId = 0;
            workerStatus.lastEvent = stepResult.result.state;
            refreshWorkerStatus();
        }

        workerIdleCondition.notify_all();
    }
}

void PreparedPlaybackService::refreshWorkerStatus()
{
    workerStatus.pendingWorkCount = queuedJobs.size();
    workerStatus.configuredMaxPendingWorkCount = maxPendingJobs;
    workerStatus.configuredMaxInFlightWorkCount = 1;
    workerStatus.configuredMaxCompletedResultCount = schedulerBudgets.maximumCompletedResults;
    workerStatus.configuredMaxConsecutivePerformanceJobs = schedulerBudgets.maximumConsecutivePerformanceJobs;
    workerStatus.configuredMaxCommandToQueuedMicros = schedulerBudgets.maximumCommandToQueuedMicros;
    workerStatus.configuredMaxRequestToReadyMicros = schedulerBudgets.maximumRequestToReadyMicros;
    workerStatus.configuredMaxRetainedPreparedBytes = schedulerBudgets.maximumRetainedPreparedBytes;
    workerStatus.configuredMaxMessageThreadServiceMicros = schedulerBudgets.maximumMessageThreadServiceMicros;
    workerStatus.completedResultCount = completedResults.size();
    workerStatus.maxCompletedResultCount = std::max(workerStatus.maxCompletedResultCount,
                                                    workerStatus.completedResultCount);
    workerStatus.maxPendingWorkCount = std::max(workerStatus.maxPendingWorkCount, workerStatus.pendingWorkCount);
    workerStatus.activeOwnershipRecordCount = cacheEntries.size();
    workerStatus.activeOwnershipBytes = std::accumulate(
        cacheEntries.begin(),
        cacheEntries.end(),
        std::uint64_t {0},
        [](std::uint64_t total, const auto& entry)
        {
            return total + entry.second.retainedBytes;
        });
    workerStatus.retiredOwnershipRecordCount = retiredCacheEntries.size();
    const auto retainedBytes = workerStatus.activeOwnershipBytes + workerStatus.retiredBytesAwaitingCleanup;
    workerStatus.maxObservedRetainedPreparedBytes = std::max(workerStatus.maxObservedRetainedPreparedBytes,
                                                             retainedBytes);
    if (retainedBytes > schedulerBudgets.maximumRetainedPreparedBytes)
        ++workerStatus.retainedPreparedBytesBudgetViolationCount;
}

bool PreparedPlaybackService::isCancellationRequested(
    const PreparedPlaybackBuildRequest& request) const noexcept
{
    if (request.cancellationGeneration == 0)
        return false;
    const auto currentGeneration = request.lane == PreparedPlaybackWorkLane::performance
        ? performanceCancellationGeneration.load(std::memory_order_acquire)
        : previewCancellationGeneration.load(std::memory_order_acquire);
    return currentGeneration != request.cancellationGeneration;
}

void PreparedPlaybackService::retireSupersededCacheEntries(const std::string& sampleSourceId,
                                                           const std::string& cacheKey,
                                                           std::uint64_t retiredByBuildId)
{
    for (auto iterator = cacheEntries.begin(); iterator != cacheEntries.end();)
    {
        if (iterator->first != cacheKey && iterator->second.sample.sampleSourceId == sampleSourceId)
        {
            iterator->second.ownership.retirementToken =
                buildRetirementToken(nextRetirementToken++, sampleSourceId, retiredByBuildId);
            iterator->second.ownership.lifetimeState = "retired-awaiting-cleanup";
            iterator->second.ownership.retiredByBuildId = retiredByBuildId;
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

std::string computePreparedPlaybackContentDigest(const ImmutablePreparedPlayback& prepared)
{
    return "fnv1a64:" + computeFnv1a64Hex(serializePrepared(prepared, false).dump());
}

std::string computePreparedPlaybackRouteDigest(const ImmutablePlaybackSnapshot& snapshot,
                                               const ImmutablePreparedPlayback& prepared)
{
    ordered_json root;
    ordered_json zones = ordered_json::array();
    auto authoredZones = snapshot.zones;
    std::sort(authoredZones.begin(), authoredZones.end(), [](const auto& left, const auto& right)
    {
        return left.id < right.id;
    });
    for (const auto& zone : authoredZones)
    {
        ordered_json value;
        value["id"] = zone.id;
        value["sampleSourceId"] = zone.sampleSourceId;
        value["groupId"] = zone.groupId;
        value["articulationId"] = zone.articulationId;
        value["rootKey"] = zone.rootKey;
        value["keyLow"] = zone.keyLow;
        value["keyHigh"] = zone.keyHigh;
        value["velocityLow"] = zone.velocityLow;
        value["velocityHigh"] = zone.velocityHigh;
        value["gainDb"] = zone.gainDb;
        value["pan"] = zone.pan;
        value["sampleStartFrame"] = zone.sampleStartFrame;
        value["loopEnabled"] = zone.loopEnabled;
        value["loopStartFrame"] = zone.loopStartFrame;
        value["loopEndFrame"] = zone.loopEndFrame;
        if (zone.triggerMode == ZoneTriggerMode::oneShot)
            value["triggerMode"] = "one-shot";
        zones.push_back(std::move(value));
    }
    root["zones"] = std::move(zones);

    ordered_json preparedRoutes = ordered_json::array();
    auto handles = prepared.zones;
    std::sort(handles.begin(), handles.end(), [](const auto& left, const auto& right)
    {
        return left.zoneId < right.zoneId;
    });
    for (const auto& handle : handles)
    {
        ordered_json value;
        value["zoneId"] = handle.zoneId;
        value["sampleSourceId"] = handle.sampleSourceId;
        value["streamSampleId"] = handle.streamSampleId;
        value["rootKey"] = handle.rootKey;
        value["keyLow"] = handle.keyLow;
        value["keyHigh"] = handle.keyHigh;
        value["velocityLow"] = handle.velocityLow;
        value["velocityHigh"] = handle.velocityHigh;
        value["gainDb"] = handle.gainDb;
        value["pan"] = handle.pan;
        value["sampleStartFrame"] = handle.sampleStartFrame;
        value["loopEnabled"] = handle.loopEnabled;
        value["loopStartFrame"] = handle.loopStartFrame;
        value["loopEndFrame"] = handle.loopEndFrame;
        if (handle.triggerMode == ZoneTriggerMode::oneShot)
            value["triggerMode"] = "one-shot";
        preparedRoutes.push_back(std::move(value));
    }
    root["preparedRoutes"] = std::move(preparedRoutes);

    auto articulationRoutes = snapshot.articulationRoutes;
    std::sort(articulationRoutes.begin(), articulationRoutes.end(), [](const auto& left, const auto& right)
    {
        return left.articulationId < right.articulationId;
    });
    root["articulationRoutes"] = ordered_json::array();
    for (auto& route : articulationRoutes)
    {
        std::sort(route.zoneIds.begin(), route.zoneIds.end());
        root["articulationRoutes"].push_back({ { "id", route.articulationId }, { "zones", route.zoneIds } });
    }

    auto groupRoutes = snapshot.groupRoutes;
    std::sort(groupRoutes.begin(), groupRoutes.end(), [](const auto& left, const auto& right)
    {
        return left.groupId < right.groupId;
    });
    root["groupRoutes"] = ordered_json::array();
    for (auto& route : groupRoutes)
    {
        std::sort(route.articulationIds.begin(), route.articulationIds.end());
        std::sort(route.zoneIds.begin(), route.zoneIds.end());
        root["groupRoutes"].push_back({ { "id", route.groupId },
                                         { "articulations", route.articulationIds },
                                         { "zones", route.zoneIds } });
    }

    auto buses = snapshot.routingBuses;
    std::sort(buses.begin(), buses.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    root["routingBuses"] = ordered_json::array();
    for (auto& bus : buses)
    {
        root["routingBuses"].push_back({ { "id", bus.id },
                                          { "input", bus.inputSourceId },
                                          { "fxSlots", bus.fxSlotIds } });
    }

    return "fnv1a64:" + computeFnv1a64Hex(root.dump());
}

std::string computePreparedPlaybackSourceProvenanceDigest(const ImmutablePreparedPlayback& prepared)
{
    auto samples = prepared.samples;
    std::sort(samples.begin(), samples.end(), [](const auto& left, const auto& right)
    {
        return left.sampleSourceId < right.sampleSourceId;
    });

    ordered_json values = ordered_json::array();
    for (const auto& sample : samples)
    {
        values.push_back({ { "sampleSourceId", sample.sampleSourceId },
                           { "canonicalSourceIdentity", sample.canonicalSourceIdentity },
                           { "sourceFingerprintHex", sample.sourceFingerprintHex },
                           { "formatName", sample.formatName },
                           { "sampleRate", sample.sampleRate },
                           { "frameCount", sample.frameCount },
                           { "channelCount", sample.channelCount } });
    }
    return "fnv1a64:" + computeFnv1a64Hex(values.dump());
}

std::string computePlaybackSnapshotMacroSchemaDigest(const ImmutablePlaybackSnapshot& snapshot)
{
    auto macros = snapshot.macroDefaults;
    std::sort(macros.begin(), macros.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    ordered_json values = ordered_json::array();
    for (auto& macro : macros)
    {
        std::sort(macro.targets.begin(), macro.targets.end(), [](const auto& left, const auto& right)
        {
            return std::tie(left.parameterId, left.parameterPath, left.role)
                < std::tie(right.parameterId, right.parameterPath, right.role);
        });
        ordered_json targets = ordered_json::array();
        for (const auto& target : macro.targets)
            targets.push_back({ { "parameterId", target.parameterId },
                                { "parameterPath", target.parameterPath },
                                { "role", target.role } });
        values.push_back({ { "id", macro.id },
                           { "minValue", macro.minValue },
                           { "maxValue", macro.maxValue },
                           { "targets", std::move(targets) } });
    }
    return "fnv1a64:" + computeFnv1a64Hex(values.dump());
}

std::string serializePreparedPlaybackContent(const ImmutablePreparedPlayback& prepared)
{
    return serializePrepared(prepared, false).dump(2) + "\n";
}

bool operator==(const PreparedPlaybackPageHandle& left, const PreparedPlaybackPageHandle& right)
{
    return left.pageIndex == right.pageIndex
        && left.offsetBytes == right.offsetBytes
        && left.sizeBytes == right.sizeBytes;
}

bool operator==(const PreparedPlaybackOwnershipRecord& left, const PreparedPlaybackOwnershipRecord& right)
{
    return left.ownershipToken == right.ownershipToken
        && left.retirementToken == right.retirementToken
        && left.cacheKey == right.cacheKey
        && left.sampleSourceId == right.sampleSourceId
        && left.streamSampleId == right.streamSampleId
        && left.lifetimeState == right.lifetimeState
        && left.retainedBytes == right.retainedBytes
        && left.preparedBuildId == right.preparedBuildId
        && left.retiredByBuildId == right.retiredByBuildId;
}

bool operator==(const PreparedPlaybackSampleHandle& left, const PreparedPlaybackSampleHandle& right)
{
    return left.sampleSourceId == right.sampleSourceId
        && left.streamSampleId == right.streamSampleId
        && left.sourcePath == right.sourcePath
        && left.canonicalSourcePath == right.canonicalSourcePath
        && left.canonicalSourceIdentity == right.canonicalSourceIdentity
        && left.sourceFingerprintHex == right.sourceFingerprintHex
        && left.formatName == right.formatName
        && left.role == right.role
        && left.channelLayout == right.channelLayout
        && left.sampleRate == right.sampleRate
        && left.frameCount == right.frameCount
        && left.channelCount == right.channelCount
        && left.rootMidiNotePresent == right.rootMidiNotePresent
        && left.rootMidiNote == right.rootMidiNote
        && left.loopRangePresent == right.loopRangePresent
        && left.loopStartFrame == right.loopStartFrame
        && left.loopEndFrame == right.loopEndFrame
        && equalDecodedSampleData(left.decodedSampleData, right.decodedSampleData)
        && left.ownershipToken == right.ownershipToken
        && left.cacheKey == right.cacheKey
        && left.ownershipRecordIndex == right.ownershipRecordIndex;
}

bool operator==(const PreparedPlaybackStreamHandle& left, const PreparedPlaybackStreamHandle& right)
{
    return left.sampleSourceId == right.sampleSourceId
        && left.streamSampleId == right.streamSampleId
        && left.containerId == right.containerId
        && left.containerPath == right.containerPath
        && left.payloadEncoding == right.payloadEncoding
        && left.topologyKind == right.topologyKind
        && left.compiledStreamTopologyAvailable == right.compiledStreamTopologyAvailable
        && left.pageSizeBytes == right.pageSizeBytes
        && left.payloadOffsetBytes == right.payloadOffsetBytes
        && left.payloadSizeBytes == right.payloadSizeBytes
        && left.prefetchBytes == right.prefetchBytes
        && left.streamedPayloadOffsetBytes == right.streamedPayloadOffsetBytes
        && left.streamedPayloadBytes == right.streamedPayloadBytes
        && left.pageCount == right.pageCount
        && left.pageRangePresent == right.pageRangePresent
        && left.firstPageIndex == right.firstPageIndex
        && left.lastPageIndex == right.lastPageIndex
        && left.firstPageOffsetBytes == right.firstPageOffsetBytes
        && left.lastPageOffsetBytes == right.lastPageOffsetBytes
        && left.lastPageSizeBytes == right.lastPageSizeBytes
        && left.ownershipToken == right.ownershipToken
        && left.cacheKey == right.cacheKey
        && left.ownershipRecordIndex == right.ownershipRecordIndex
        && left.pages == right.pages;
}

bool operator==(const PreparedPlaybackZoneHandle& left, const PreparedPlaybackZoneHandle& right)
{
    return left.zoneId == right.zoneId
        && left.sampleSourceId == right.sampleSourceId
        && left.streamSampleId == right.streamSampleId
        && left.preparedSampleIndex == right.preparedSampleIndex
        && left.preparedStreamIndex == right.preparedStreamIndex
        && left.rootKey == right.rootKey
        && left.keyLow == right.keyLow
        && left.keyHigh == right.keyHigh
        && left.velocityLow == right.velocityLow
        && left.velocityHigh == right.velocityHigh
        && left.gainDb == right.gainDb
        && left.pan == right.pan
        && left.sampleStartFrame == right.sampleStartFrame
        && left.loopEnabled == right.loopEnabled
        && left.loopStartFrame == right.loopStartFrame
        && left.loopEndFrame == right.loopEndFrame
        && left.triggerMode == right.triggerMode;
}

bool operator==(const ImmutablePreparedPlayback& left, const ImmutablePreparedPlayback& right)
{
    return left.snapshotBuildId == right.snapshotBuildId
        && left.snapshotContentDigest == right.snapshotContentDigest
        && left.compilerVersion == right.compilerVersion
        && left.draftRevision == right.draftRevision
        && left.containerId == right.containerId
        && left.containerPath == right.containerPath
        && left.payloadEncoding == right.payloadEncoding
        && left.pageSizeBytes == right.pageSizeBytes
        && left.preparedContentDigest == right.preparedContentDigest
        && left.routeDigest == right.routeDigest
        && left.sourceProvenanceDigest == right.sourceProvenanceDigest
        && left.macroSchemaDigest == right.macroSchemaDigest
        && left.ownershipRecords == right.ownershipRecords
        && left.samples == right.samples
        && left.streams == right.streams
        && left.zones == right.zones
        && left.notes == right.notes;
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

std::string toString(PreparedPlaybackCompletionDisposition disposition)
{
    switch (disposition)
    {
    case PreparedPlaybackCompletionDisposition::none:
        return "none";
    case PreparedPlaybackCompletionDisposition::completed:
        return "completed";
    case PreparedPlaybackCompletionDisposition::canceled:
        return "canceled";
    case PreparedPlaybackCompletionDisposition::superseded:
        return "superseded";
    case PreparedPlaybackCompletionDisposition::rejected:
        return "rejected";
    case PreparedPlaybackCompletionDisposition::failed:
        return "failed";
    }
    return "unknown";
}
} // namespace drs::engine
