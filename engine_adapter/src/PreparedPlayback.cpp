#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/SampleDataSource.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

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

ordered_json serializeVelocityCrossfade(const VelocityCrossfadeDescriptor& crossfade)
{
    return {
        { "fadeInLowVelocity", crossfade.fadeInLowVelocity },
        { "fadeInHighVelocity", crossfade.fadeInHighVelocity },
        { "fadeOutLowVelocity", crossfade.fadeOutLowVelocity },
        { "fadeOutHighVelocity", crossfade.fadeOutHighVelocity },
        { "curve", "linear" }
    };
}

ordered_json serializeVelocityCrossfadeRuntime(const VelocityCrossfadeRuntimeDescriptor& runtime)
{
    ordered_json value;
    value["effectiveLowVelocity"] = runtime.effectiveLowVelocity;
    value["effectiveHighVelocity"] = runtime.effectiveHighVelocity;

    if (!runtime.fadeInNeighborZoneId.empty())
        value["fadeInNeighborZoneId"] = runtime.fadeInNeighborZoneId;
    if (!runtime.fadeOutNeighborZoneId.empty())
        value["fadeOutNeighborZoneId"] = runtime.fadeOutNeighborZoneId;
    if (runtime.fadeInOverlapLowVelocity > 0)
        value["fadeInOverlapLowVelocity"] = runtime.fadeInOverlapLowVelocity;
    if (runtime.fadeInOverlapHighVelocity > 0)
        value["fadeInOverlapHighVelocity"] = runtime.fadeInOverlapHighVelocity;
    if (runtime.fadeOutOverlapLowVelocity > 0)
        value["fadeOutOverlapLowVelocity"] = runtime.fadeOutOverlapLowVelocity;
    if (runtime.fadeOutOverlapHighVelocity > 0)
        value["fadeOutOverlapHighVelocity"] = runtime.fadeOutOverlapHighVelocity;

    return value;
}

std::string toRoundRobinModeString(RoundRobinMode mode)
{
    switch (mode)
    {
        case RoundRobinMode::sequential:
            return "sequential";
        case RoundRobinMode::random:
            return "random";
    }

    return "sequential";
}

ordered_json serializeRoundRobin(const RoundRobinDescriptor& roundRobin)
{
    ordered_json value;
    value["poolId"] = roundRobin.poolId;
    value["slotCount"] = roundRobin.slotCount;
    value["slotIndex"] = roundRobin.slotIndex;
    value["mode"] = toRoundRobinModeString(roundRobin.mode);
    return value;
}

ordered_json serializeGroupRoute(const PreparedPlaybackGroupRoute& route, bool includeWorkspaceVisible)
{
    ordered_json routeObject;
    routeObject["groupId"] = route.groupId;
    routeObject["articulationIds"] = route.articulationIds;
    routeObject["zoneIds"] = route.zoneIds;
    routeObject["displayName"] = route.displayName;
    routeObject["displayOrder"] = route.displayOrder;
    routeObject["routingSourceId"] = route.routingSourceId;
    if (includeWorkspaceVisible)
        routeObject["workspaceVisible"] = route.workspaceVisible;
    routeObject["gainDb"] = route.gainDb;
    routeObject["pan"] = route.pan;
    routeObject["routingBusId"] = route.routingBusId;
    routeObject["auditionAnchorZoneId"] = route.auditionAnchorZoneId;
    return routeObject;
}

PreparedPlaybackGroupRoute toPreparedGroupRoute(const PlaybackSnapshotGroupRoute& route)
{
    PreparedPlaybackGroupRoute preparedRoute;
    preparedRoute.groupId = route.groupId;
    preparedRoute.articulationIds = route.articulationIds;
    preparedRoute.zoneIds = route.zoneIds;
    preparedRoute.displayName = route.displayName;
    preparedRoute.displayOrder = route.displayOrder;
    preparedRoute.routingSourceId = route.routingSourceId;
    preparedRoute.workspaceVisible = route.workspaceVisible;
    preparedRoute.gainDb = route.gainDb;
    preparedRoute.pan = route.pan;
    preparedRoute.routingBusId = route.routingBusId;
    preparedRoute.auditionAnchorZoneId = route.auditionAnchorZoneId;
    return preparedRoute;
}

ordered_json serializePrepared(const ImmutablePreparedPlayback& prepared, bool includeDigest)
{
    ordered_json root;
    root["snapshotContentDigest"] = prepared.snapshotContentDigest;
    root["snapshotDspGraphDigest"] = prepared.snapshotDspGraphDigest;
    root["dspGraphDigest"] = prepared.dspGraphDigest;
    root["compilerVersion"] = prepared.compilerVersion;
    root["draftRevision"] = prepared.draftRevision;
    root["selectedGroupId"] = prepared.selectedGroupId;
    root["masterGainDb"] = prepared.masterGainDb;
    root["containerId"] = prepared.containerId;
    root["containerPath"] = prepared.containerPath;
    root["payloadEncoding"] = prepared.payloadEncoding;
    root["pageSizeBytes"] = prepared.pageSizeBytes;
    root["performanceProgram"] = ordered_json::parse(serializeCompiledPerformanceProgram(prepared.performanceProgram));

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

    if (includeDigest)
    {
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
    }

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
        sampleObject["cacheKey"] = sample.cacheKey;
        if (includeDigest)
        {
            sampleObject["ownershipToken"] = sample.ownershipToken;
            sampleObject["ownershipRecordIndex"] = sample.ownershipRecordIndex;
        }
        if (sample.dataSource != nullptr)
        {
            const auto& descriptor = sample.dataSource->descriptor();
            sampleObject["dataSourceKind"] = static_cast<int>(descriptor.kind);
            if (includeDigest)
                sampleObject["dataSourceGeneration"] = descriptor.generation;
            sampleObject["dataSourceCanonicalIdentity"] = descriptor.canonicalSourceIdentity;
            sampleObject["dataSourceProvenanceIdentity"] = descriptor.provenanceIdentity;
            sampleObject["dataSourceHeadSizeBytes"] = descriptor.headSizeBytes;
            sampleObject["dataSourcePageSizeBytes"] = descriptor.pageSizeBytes;
        }
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
        streamObject["cacheKey"] = streamHandle.cacheKey;
        if (includeDigest)
        {
            streamObject["ownershipToken"] = streamHandle.ownershipToken;
            streamObject["ownershipRecordIndex"] = streamHandle.ownershipRecordIndex;
        }

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

    ordered_json groupRoutes = ordered_json::array();
    for (const auto& route : prepared.groupRoutes)
        groupRoutes.push_back(serializeGroupRoute(route, includeDigest));
    root["groupRoutes"] = std::move(groupRoutes);

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
        if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            zoneObject["velocityCrossfade"] = serializeVelocityCrossfade(zone.velocityCrossfade);
        if (hasAnyVelocityCrossfadeRuntimeValue(zone.velocityCrossfadeRuntime))
            zoneObject["velocityCrossfadeRuntime"] = serializeVelocityCrossfadeRuntime(zone.velocityCrossfadeRuntime);
        zoneObject["gainDb"] = zone.gainDb;
        zoneObject["pan"] = zone.pan;
        zoneObject["fineTuneCents"] = zone.fineTuneCents;
        zoneObject["amplitudeVelocityTracking"] = zone.amplitudeVelocityTracking;
        ordered_json controllerConditions = ordered_json::array();
        for (const auto& condition : zone.controllerConditions)
            controllerConditions.push_back({ { "controllerNumber", condition.controllerNumber },
                                             { "minimumValue", condition.minimumValue },
                                             { "maximumValue", condition.maximumValue } });
        zoneObject["controllerConditions"] = std::move(controllerConditions);
        zoneObject["sampleStartFrame"] = zone.sampleStartFrame;
        zoneObject["loopEnabled"] = zone.loopEnabled;
        zoneObject["loopStartFrame"] = zone.loopStartFrame;
        zoneObject["loopEndFrame"] = zone.loopEndFrame;
        zoneObject["releaseSeconds"] = zone.releaseSeconds;
        zoneObject["releaseShape"] = zone.releaseShape;
        if (zone.roundRobin.has_value())
            zoneObject["roundRobin"] = serializeRoundRobin(*zone.roundRobin);
        if (zone.triggerMode == ZoneTriggerMode::oneShot)
            zoneObject["triggerMode"] = "one-shot";
        zones.push_back(std::move(zoneObject));
    }
    root["zones"] = std::move(zones);
    ordered_json controllerDefaults = ordered_json::array();
    for (const auto& value : prepared.controllerDefaults)
        controllerDefaults.push_back({ { "controllerNumber", value.controllerNumber }, { "value", value.value } });
    root["controllerDefaults"] = std::move(controllerDefaults);

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

struct EmbeddedPreparedSampleDataResult
{
    bool decoded = false;
    std::string state;
    std::vector<std::string> issues;
    std::shared_ptr<PreparedPlaybackDecodedSampleData> sampleData;
};

EmbeddedPreparedSampleDataResult decodeEmbeddedPreparedSampleData(
    const RuntimeStreamContainerModel& container,
    const RuntimeStreamSampleDefinition& streamSample)
{
    EmbeddedPreparedSampleDataResult result;
    result.state = "Embedded prepared sample decode failed";

    if (!container.payloadEmbedded)
    {
        result.issues.push_back("Embedded payload bytes are unavailable.");
        return result;
    }

    if (container.payloadEncoding != "f32-interleaved-little-endian")
    {
        result.issues.push_back("Unsupported embedded payload encoding '" + container.payloadEncoding + "'.");
        return result;
    }

    if (streamSample.channelCount == 0 || streamSample.frameCount == 0)
    {
        result.issues.push_back("Embedded stream sample metadata must declare frames and channels.");
        return result;
    }

    const auto expectedPayloadBytes = streamSample.frameCount
        * static_cast<std::uint64_t>(streamSample.channelCount)
        * static_cast<std::uint64_t>(sizeof(float));
    if (expectedPayloadBytes != streamSample.payloadSizeBytes)
    {
        result.issues.push_back("Embedded stream sample payloadSizeBytes does not match its frame and channel counts.");
        return result;
    }

    if (streamSample.payloadOffsetBytes > container.embeddedPayloadBytes.size()
        || streamSample.payloadSizeBytes > container.embeddedPayloadBytes.size() - streamSample.payloadOffsetBytes)
    {
        result.issues.push_back("Embedded stream sample payload range exceeds the packaged payload bytes.");
        return result;
    }

    auto sampleData = std::make_shared<PreparedPlaybackDecodedSampleData>();
    sampleData->normalizedChannels.resize(streamSample.channelCount);
    for (auto& channel : sampleData->normalizedChannels)
        channel.resize(static_cast<std::size_t>(streamSample.frameCount));

    const auto* payloadBytes = container.embeddedPayloadBytes.data() + streamSample.payloadOffsetBytes;
    for (std::uint64_t frameIndex = 0; frameIndex < streamSample.frameCount; ++frameIndex)
    {
        for (std::uint32_t channelIndex = 0; channelIndex < streamSample.channelCount; ++channelIndex)
        {
            const auto sampleByteOffset = (frameIndex * static_cast<std::uint64_t>(streamSample.channelCount)
                                           + static_cast<std::uint64_t>(channelIndex))
                * static_cast<std::uint64_t>(sizeof(float));
            const auto* sampleBytes = payloadBytes + sampleByteOffset;
            const auto bits = static_cast<std::uint32_t>(sampleBytes[0])
                | (static_cast<std::uint32_t>(sampleBytes[1]) << 8u)
                | (static_cast<std::uint32_t>(sampleBytes[2]) << 16u)
                | (static_cast<std::uint32_t>(sampleBytes[3]) << 24u);
            float sampleValue = 0.0f;
            std::memcpy(&sampleValue, &bits, sizeof(sampleValue));
            sampleData->normalizedChannels[static_cast<std::size_t>(channelIndex)]
                                        [static_cast<std::size_t>(frameIndex)] = sampleValue;
        }
    }

    result.decoded = true;
    result.state = "Embedded prepared sample decoded";
    result.sampleData = std::move(sampleData);
    return result;
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

ResidentPreparationAdmissionResult assessResidentPreparationAdmission(
    const std::vector<ResidentPreparationSampleMetadata>& samples,
    const std::uint64_t residentBudgetBytes) noexcept
{
    ResidentPreparationAdmissionResult result;
    result.sampleCount = samples.size();
    result.residentBudgetBytes = residentBudgetBytes;
    result.metadataAvailable = !samples.empty();

    constexpr auto floatWidth = static_cast<std::uint64_t>(sizeof(float));
    for (const auto& sample : samples)
    {
        const auto channels = static_cast<std::uint64_t>(sample.channelCount);
        if (sample.frameCount == 0 || channels == 0
            || sample.frameCount > std::numeric_limits<std::uint64_t>::max() / channels
            || sample.frameCount * channels
                > std::numeric_limits<std::uint64_t>::max() / floatWidth)
        {
            result.arithmeticOverflow = true;
            result.readiness = PreparedPlaybackReadinessState::streamingRequired;
            result.findingCode = "resident-admission-size-overflow";
            result.guidance = "Sample dimensions cannot be represented safely for resident playback; use streaming preparation.";
            return result;
        }

        const auto sampleBytes = sample.frameCount * channels * floatWidth;
        if (sampleBytes > std::numeric_limits<std::uint64_t>::max()
                - result.estimatedDecodedBytes)
        {
            result.arithmeticOverflow = true;
            result.readiness = PreparedPlaybackReadinessState::streamingRequired;
            result.findingCode = "resident-admission-size-overflow";
            result.guidance = "Aggregate decoded size overflowed checked 64-bit accounting; use streaming preparation.";
            return result;
        }
        result.estimatedDecodedBytes += sampleBytes;
    }

    if (result.estimatedDecodedBytes > residentBudgetBytes)
    {
        result.readiness = PreparedPlaybackReadinessState::streamingRequired;
        result.findingCode = "resident-admission-budget-exceeded";
        result.guidance = "This scope exceeds the resident preparation budget; select a smaller scope or use streaming preparation.";
        return result;
    }

    result.admitted = result.metadataAvailable;
    result.readiness = result.admitted
        ? PreparedPlaybackReadinessState::playbackDeferred
        : PreparedPlaybackReadinessState::metadataLoaded;
    return result;
}

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

    reclaimerThread = std::thread([this] { runBackgroundReclaimer(); });

    if (backgroundWorkerEnabled)
        workerThread = std::thread([this] { runBackgroundWorker(); });
}

PreparedPlaybackService::~PreparedPlaybackService()
{
    previewCancellationGeneration.fetch_add(1, std::memory_order_acq_rel);
    performanceCancellationGeneration.fetch_add(1, std::memory_order_acq_rel);
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        stopWorkerRequested = true;
    }

    workerCondition.notify_all();
    workerIdleCondition.notify_all();

    if (workerThread.joinable())
        workerThread.join();

    // With the producer worker stopped, transfer every remaining cache owner to
    // the background reclaimer. Shutdown may wait, but no large ownership graph
    // is destroyed by this caller thread.
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        for (auto& active : cacheEntries)
        {
            workerStatus.retiredBytesAwaitingCleanup += active.second.retainedBytes;
            retiredCacheEntries.push_back(std::move(active));
        }
        cacheEntries.clear();
        refreshWorkerStatus();
    }
    for (;;)
    {
        if (serviceRetiredCacheCleanup() == 0)
        {
            std::lock_guard<std::mutex> lock(workerMutex);
            if (retiredCacheEntries.empty())
                break;
        }
        waitForBackgroundReclamation(std::chrono::seconds(30));
    }
    waitForBackgroundReclamation(std::chrono::seconds(30));
    {
        std::lock_guard<std::mutex> lock(reclaimerMutex);
        stopReclaimerRequested = true;
    }
    reclaimerCondition.notify_all();
    if (reclaimerThread.joinable())
        reclaimerThread.join();
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

    const auto finishCanceled = [&]()
    {
        {
            std::lock_guard<std::mutex> lock(workerMutex);
            for (auto iterator = cacheEntries.begin(); iterator != cacheEntries.end();)
            {
                if (iterator->second.ownership.preparedBuildId == request.buildId)
                    iterator = cacheEntries.erase(iterator);
                else
                    ++iterator;
            }
            ++workerStatus.cooperativeCancellationCount;
            ++workerStatus.cancellationCount;
            refreshWorkerStatus();
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
        result.buildDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
        result.requestToReadyMicros = request.queuedAtMicros == 0
            ? result.buildDurationMicros
            : clockMicros() - request.queuedAtMicros;
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
    const auto publishSourceProgress = [this, &request](std::string phase,
                                                        const std::size_t ordinal,
                                                        const std::size_t total)
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        if (workerStatus.inFlightBuildId != request.buildId)
            return;
        workerStatus.inFlightProgressPhase = std::move(phase);
        workerStatus.inFlightSourceOrdinal = ordinal;
        workerStatus.inFlightSourceCount = total;
    };

    std::vector<ResidentPreparationSampleMetadata> residentMetadata;
    residentMetadata.reserve(resolvedRequest.sampleResolutions.size());
    auto residentMetadataComplete = !resolvedRequest.sampleResolutions.empty();
    std::size_t inspectedSourceOrdinal = 0;
    for (const auto& sampleResolution : resolvedRequest.sampleResolutions)
    {
        publishSourceProgress("Inspecting sources",
                              ++inspectedSourceOrdinal,
                              resolvedRequest.sampleResolutions.size());
        if (isCancellationRequested(request))
            return finishCanceled();
        const auto* candidateStreamSample = !streamResult.loaded
                || sampleResolution.selectedStreamSampleId.empty()
            ? nullptr
            : findStreamSampleById(streamResult.container,
                                   sampleResolution.selectedStreamSampleId);
        const auto* packageEmbeddedSample = streamResult.loaded
                && streamResult.container.payloadEmbedded
            ? candidateStreamSample
            : nullptr;
        if (packageEmbeddedSample != nullptr)
        {
            residentMetadata.push_back({ packageEmbeddedSample->frameCount,
                                         packageEmbeddedSample->channelCount });
            continue;
        }

        const auto inspection = inspectSampleFileMetadataOnly(
            sampleResolution.normalizedSourcePath);
        if (!inspection.inspected || !inspection.accepted)
        {
            residentMetadataComplete = false;
            continue;
        }
        residentMetadata.push_back({ inspection.metadata.frameCount,
                                     inspection.metadata.channelCount });
    }

    result.admission.residentBudgetBytes = schedulerBudgets.maximumRetainedPreparedBytes;
    auto streamingPreparation = false;
    if (residentMetadataComplete
        && residentMetadata.size() == resolvedRequest.sampleResolutions.size())
    {
        result.admission = assessResidentPreparationAdmission(
            residentMetadata, schedulerBudgets.maximumRetainedPreparedBytes);
        if (!result.admission.admitted)
        {
            streamingPreparation = schedulerBudgets.allowWavStreaming
                && !result.admission.arithmeticOverflow
                && result.admission.readiness == PreparedPlaybackReadinessState::streamingRequired
                && !streamResult.container.payloadEmbedded;
            if (streamingPreparation)
            {
                result.admission.readiness = PreparedPlaybackReadinessState::playbackDeferred;
                result.admission.guidance = "Resident budget exceeded; bounded WAV heads will be prepared for streaming playback.";
            }
            else
            {
            result.completionDisposition = PreparedPlaybackCompletionDisposition::rejected;
            result.lifecycleState = PlaybackSnapshotLifecycleState::failed;
            result.state = "Prepared playback streaming required";
            result.metrics.failureCount = 1;
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       result.admission.findingCode,
                       "residentAdmission",
                       result.admission.guidance + " Estimated decoded bytes: "
                           + std::to_string(result.admission.estimatedDecodedBytes)
                           + "; resident budget bytes: "
                           + std::to_string(result.admission.residentBudgetBytes) + ".");
            result.buildDurationMicros = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    Clock::now() - startTime).count());
            result.requestToReadyMicros = request.queuedAtMicros == 0
                ? result.buildDurationMicros
                : clockMicros() - request.queuedAtMicros;
            return result;
            }
        }
    }

    result.prepared.snapshotBuildId = snapshotResult.buildId;
    result.prepared.snapshotContentDigest = snapshotResult.snapshot.contentDigest;
    result.prepared.snapshotDspGraphDigest = snapshotResult.snapshot.dspGraphDigest;
    result.prepared.dspGraphDigest = snapshotResult.snapshot.dspGraphDigest;
    result.prepared.compilerVersion = compilerVersion;
    result.prepared.draftRevision = snapshotResult.snapshot.draftRevision;
    result.prepared.selectedGroupId = snapshotResult.snapshot.selectedGroupId;
    result.prepared.masterGainDb = snapshotResult.snapshot.masterGainDb;
    result.prepared.controllerDefaults = snapshotResult.snapshot.controllerDefaults;
    result.prepared.containerId = streamResult.container.containerId;
    result.prepared.containerPath = streamResult.containerPath;
    result.prepared.payloadEncoding = streamResult.container.payloadEncoding;
    result.prepared.pageSizeBytes = streamResult.container.pageSizeBytes;

    std::unordered_map<std::string, std::size_t> sampleIndices;
    std::unordered_map<std::string, std::size_t> streamIndices;

    result.prepared.ownershipRecords.reserve(resolvedRequest.sampleResolutions.size());
    result.prepared.samples.reserve(resolvedRequest.sampleResolutions.size());
    result.prepared.streams.reserve(resolvedRequest.sampleResolutions.size());
    result.prepared.groupRoutes.reserve(snapshotResult.snapshot.groupRoutes.size());
    result.prepared.zones.reserve(snapshotResult.snapshot.zones.size());

    std::size_t preparedSourceOrdinal = 0;
    for (const auto& sampleResolution : resolvedRequest.sampleResolutions)
    {
        publishSourceProgress("Preparing sources",
                              ++preparedSourceOrdinal,
                              resolvedRequest.sampleResolutions.size());
        if (isCancellationRequested(request))
            return finishCanceled();
        const auto path = "sampleIdentities[" + std::to_string(sampleResolution.snapshotSampleIndex) + "]";
        if (streamingPreparation)
        {
            auto descriptorResult = buildWavSampleDataSourceDescriptor(
                sampleResolution.sampleSourceId,
                sampleResolution.normalizedSourcePath,
                0,
                defaultSampleHeadBytes,
                defaultSamplePageBytes);
            if (!descriptorResult.built)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "prepared-wav-descriptor-failed",
                           path + ".sourcePath",
                           descriptorResult.findings.empty()
                               ? descriptorResult.state
                               : summarizeIssues(descriptorResult.findings));
                continue;
            }
            auto wavSource = std::make_shared<WavPagedSampleDataSource>(
                std::move(descriptorResult));
            if (!wavSource->prepareHead())
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "prepared-wav-head-failed",
                           path + ".sourcePath",
                           wavSource->lastFailure());
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(workerMutex);
                pageServiceSources.emplace_back(wavSource);
            }
            const auto& descriptor = wavSource->descriptor();
            PreparedPlaybackSampleHandle sampleHandle;
            sampleHandle.sampleSourceId = sampleResolution.sampleSourceId;
            sampleHandle.streamSampleId = sampleResolution.sampleSourceId;
            sampleHandle.sourcePath = sampleResolution.normalizedSourcePath;
            sampleHandle.canonicalSourcePath = sampleResolution.normalizedSourcePath;
            sampleHandle.canonicalSourceIdentity = buildCanonicalSourceIdentity(
                sampleResolution.sampleSourceId,
                sampleResolution.normalizedSourcePath);
            sampleHandle.sourceFingerprintHex = descriptor.provenanceIdentity;
            sampleHandle.formatName = descriptor.formatName;
            sampleHandle.role = snapshotResult.snapshot.sampleIdentities[
                sampleResolution.snapshotSampleIndex].role;
            sampleHandle.channelLayout = descriptor.channelLayout;
            sampleHandle.sampleRate = descriptor.sampleRate;
            sampleHandle.frameCount = descriptor.frameCount;
            sampleHandle.channelCount = descriptor.channelCount;
            sampleHandle.dataSource = wavSource;
            sampleHandle.ownershipToken = "wav-generation:"
                + std::to_string(descriptor.generation);
            sampleHandle.cacheKey = descriptor.provenanceIdentity;

            PreparedPlaybackStreamHandle streamHandle;
            streamHandle.sampleSourceId = sampleResolution.sampleSourceId;
            streamHandle.streamSampleId = sampleResolution.sampleSourceId;
            streamHandle.payloadEncoding = descriptor.formatName;
            streamHandle.topologyKind = "wav-paged";
            streamHandle.pageSizeBytes = descriptor.pageSizeBytes;
            streamHandle.payloadOffsetBytes = descriptor.dataOffsetBytes;
            streamHandle.payloadSizeBytes = descriptor.dataSizeBytes;
            streamHandle.prefetchBytes = descriptor.headSizeBytes;
            streamHandle.ownershipToken = sampleHandle.ownershipToken;
            streamHandle.cacheKey = sampleHandle.cacheKey;

            PreparedPlaybackOwnershipRecord ownership;
            ownership.ownershipToken = sampleHandle.ownershipToken;
            ownership.cacheKey = sampleHandle.cacheKey;
            ownership.sampleSourceId = sampleHandle.sampleSourceId;
            ownership.streamSampleId = streamHandle.streamSampleId;
            ownership.lifetimeState = "active-streaming-generation";
            ownership.retainedBytes = wavSource->metrics().residentHeadBytes;
            ownership.preparedBuildId = request.buildId;
            const auto ownershipIndex = result.prepared.ownershipRecords.size();
            result.prepared.ownershipRecords.push_back(ownership);
            sampleHandle.ownershipRecordIndex = ownershipIndex;
            streamHandle.ownershipRecordIndex = ownershipIndex;

            const auto sampleIndex = result.prepared.samples.size();
            result.prepared.samples.push_back(std::move(sampleHandle));
            sampleIndices.emplace(sampleResolution.sampleSourceId, sampleIndex);
            const auto streamIndex = result.prepared.streams.size();
            result.prepared.streams.push_back(std::move(streamHandle));
            streamIndices.emplace(sampleResolution.sampleSourceId, streamIndex);
            result.metrics.preparedBytes += ownership.retainedBytes;
            ++result.metrics.cacheMissCount;
            continue;
        }
        const auto* candidateStreamSample = !streamResult.loaded || sampleResolution.selectedStreamSampleId.empty()
            ? nullptr
            : findStreamSampleById(streamResult.container, sampleResolution.selectedStreamSampleId);
        const auto packageEmbeddedSample = streamResult.loaded
                && streamResult.container.payloadEmbedded
                && candidateStreamSample != nullptr
            ? candidateStreamSample
            : nullptr;

        if (streamResult.loaded
            && streamResult.container.payloadEmbedded
            && packageEmbeddedSample == nullptr)
        {
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "prepared-package-sample-unresolved",
                       path + ".sampleSourceId",
                       "Prepared playback could not resolve packaged sample '" + sampleResolution.sampleSourceId
                           + "' to an embedded compiled-stream sample.");
            continue;
        }

        SampleSourceFingerprintResult fingerprint;
        auto sourceFingerprintHex = std::string {};
        if (packageEmbeddedSample != nullptr)
        {
            sourceFingerprintHex = packageEmbeddedSample->sourceChecksumHex;
        }
        else
        {
            struct FingerprintCancellationCallbacks final : SampleFingerprintCallbacks
            {
                const std::atomic<std::uint64_t>* generation = nullptr;
                std::uint64_t expectedGeneration = 0;

                bool isCancellationRequested() const override
                {
                    return expectedGeneration != 0
                        && generation != nullptr
                        && generation->load(std::memory_order_acquire) != expectedGeneration;
                }
            } fingerprintCancellation;
            fingerprintCancellation.generation = request.lane == PreparedPlaybackWorkLane::performance
                ? &performanceCancellationGeneration
                : &previewCancellationGeneration;
            fingerprintCancellation.expectedGeneration = request.cancellationGeneration;
            SampleFingerprintOptions fingerprintOptions;
            fingerprintOptions.callbacks = &fingerprintCancellation;
            fingerprint = fingerprintSampleSourceFile(sampleResolution.normalizedSourcePath,
                                                       fingerprintOptions);
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

            sourceFingerprintHex = fingerprint.fingerprintHex;
        }

        // A compiled stream sample is optional. Reuse it only when it describes the bytes that
        // were actually fingerprinted by this worker; stale or absent topology falls back to a
        // decoded-memory handle without making the authored source ineligible.
        const auto* streamSample = packageEmbeddedSample != nullptr
                ? packageEmbeddedSample
                : (candidateStreamSample != nullptr
                       && candidateStreamSample->sourceChecksumHex == sourceFingerprintHex
                   ? candidateStreamSample
                   : nullptr);
        const auto decodePolicyFingerprint = streamSample != nullptr
            ? buildPreparedDecodePolicyFingerprint(*streamSample, streamResult.container)
            : buildAuthoredDecodePolicyFingerprint();

        const auto cacheKey = buildCacheKey(compilerVersion,
                                            sampleResolution,
                                            sourceFingerprintHex,
                                            decodePolicyFingerprint);
        std::optional<CacheEntry> cachedEntry;
        {
            std::lock_guard<std::mutex> lock(workerMutex);
            const auto cacheIterator = std::find_if(cacheEntries.begin(),
                                                    cacheEntries.end(),
                                                    [&](const auto& entry)
                                                    {
                                                        return entry.first == cacheKey;
                                                    });
            if (cacheIterator != cacheEntries.end())
                cachedEntry = cacheIterator->second;
        }
        const auto cacheHit = cachedEntry.has_value();

        PreparedPlaybackSampleHandle sampleHandle;
        PreparedPlaybackStreamHandle streamHandle;
        PreparedPlaybackOwnershipRecord ownershipRecord;

        if (cacheHit)
        {
            sampleHandle = cachedEntry->sample;
            streamHandle = cachedEntry->stream;
            ownershipRecord = cachedEntry->ownership;
            ++result.metrics.cacheHitCount;
        }
        else
        {
            const auto& snapshotRole
                = snapshotResult.snapshot.sampleIdentities[sampleResolution.snapshotSampleIndex].role;

            if (packageEmbeddedSample != nullptr)
            {
                const auto embeddedSample
                    = decodeEmbeddedPreparedSampleData(streamResult.container, *packageEmbeddedSample);
                if (!embeddedSample.decoded || embeddedSample.sampleData == nullptr)
                {
                    addFinding(result,
                               PlaybackSnapshotFindingSeverity::error,
                               "prepared-package-sample-decode-failed",
                               path + ".sourcePath",
                               "Prepared playback could not decode packaged sample '"
                                   + sampleResolution.normalizedSourcePath + "': "
                                   + (embeddedSample.issues.empty() ? embeddedSample.state
                                                                    : summarizeIssues(embeddedSample.issues)));
                    continue;
                }

                sampleHandle.sampleSourceId = sampleResolution.sampleSourceId;
                sampleHandle.streamSampleId = packageEmbeddedSample->sampleId;
                sampleHandle.sourcePath = sampleResolution.normalizedSourcePath;
                sampleHandle.canonicalSourcePath = sampleResolution.normalizedSourcePath;
                sampleHandle.canonicalSourceIdentity = buildCanonicalSourceIdentity(sampleResolution.sampleSourceId,
                                                                                    sampleResolution.normalizedSourcePath);
                sampleHandle.sourceFingerprintHex = packageEmbeddedSample->sourceChecksumHex;
                sampleHandle.formatName = packageEmbeddedSample->formatName;
                sampleHandle.role = snapshotRole.empty() ? packageEmbeddedSample->role : snapshotRole;
                sampleHandle.channelLayout = packageEmbeddedSample->channelLayout;
                sampleHandle.sampleRate = packageEmbeddedSample->sampleRate;
                sampleHandle.frameCount = packageEmbeddedSample->frameCount;
                sampleHandle.channelCount = packageEmbeddedSample->channelCount;
                sampleHandle.rootMidiNotePresent = packageEmbeddedSample->rootMidiNotePresent;
                sampleHandle.rootMidiNote = packageEmbeddedSample->rootMidiNote;
                sampleHandle.loopRangePresent = packageEmbeddedSample->loopRangePresent;
                sampleHandle.loopStartFrame = packageEmbeddedSample->loopStartFrame;
                sampleHandle.loopEndFrame = packageEmbeddedSample->loopEndFrame;
                sampleHandle.decodedSampleData = embeddedSample.sampleData;
                sampleHandle.ownershipToken = "cache:" + cacheKey;
                sampleHandle.cacheKey = cacheKey;

                streamHandle.sampleSourceId = sampleResolution.sampleSourceId;
                streamHandle.streamSampleId = sampleHandle.streamSampleId;
                streamHandle.compiledStreamTopologyAvailable = true;
                streamHandle.containerId = streamResult.container.containerId;
                streamHandle.containerPath = streamResult.containerPath;
                streamHandle.payloadEncoding = streamResult.container.payloadEncoding;
                streamHandle.pageSizeBytes = streamResult.container.pageSizeBytes;
                streamHandle.payloadOffsetBytes = packageEmbeddedSample->payloadOffsetBytes;
                streamHandle.payloadSizeBytes = packageEmbeddedSample->payloadSizeBytes;
                streamHandle.prefetchBytes = packageEmbeddedSample->prefetchBytes;
                populateStreamTopologyMetadata(streamHandle,
                                               *packageEmbeddedSample,
                                               streamResult.container.pageSizeBytes);
                streamHandle.ownershipToken = "cache:" + cacheKey;
                streamHandle.cacheKey = cacheKey;
                streamHandle.pages.reserve(packageEmbeddedSample->pages.size());
                for (const auto& page : packageEmbeddedSample->pages)
                    streamHandle.pages.push_back({ page.pageIndex, page.offsetBytes, page.sizeBytes });
            }
            else
            {
                const auto decodedSample = importSampleFile(sampleResolution.normalizedSourcePath,
                                                            sourceFingerprintHex);
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

                result.metrics.decodedBytes += computeDecodedSampleBytes(decodedSample.sample);
            }

            if (isCancellationRequested(request))
                return finishCanceled();

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
            {
                std::lock_guard<std::mutex> lock(workerMutex);
                retireSupersededCacheEntries(sampleResolution.sampleSourceId,
                                             cacheKey,
                                             request.buildId);
                cacheEntries.push_back({ cacheKey, std::move(entry) });
                ownershipRecord = cacheEntries.back().second.ownership;
                refreshWorkerStatus();
            }
            ++result.metrics.cacheMissCount;
        }

        const auto ownershipRecordIndex = result.prepared.ownershipRecords.size();
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
            zone.velocityCrossfade,
            zone.velocityCrossfadeRuntime,
            zone.gainDb,
            zone.pan,
            zone.sampleStartFrame,
            zone.loopEnabled,
            zone.loopStartFrame,
            zone.loopEndFrame,
            zone.releaseSeconds,
            zone.releaseShape,
            zone.roundRobin,
            zone.roundRobinLength,
            zone.roundRobinPosition,
            zone.triggerMode,
            zone.fineTuneCents,
            zone.amplitudeVelocityTracking,
            zone.controllerConditions
        });
    }

    std::unordered_set<std::string> preparedZoneIds;
    preparedZoneIds.reserve(result.prepared.zones.size());
    for (const auto& zone : result.prepared.zones)
        preparedZoneIds.insert(zone.zoneId);

    for (std::size_t index = 0; index < snapshotResult.snapshot.groupRoutes.size(); ++index)
    {
        if (isCancellationRequested(request))
            return finishCanceled();

        const auto& snapshotGroupRoute = snapshotResult.snapshot.groupRoutes[index];
        result.prepared.groupRoutes.push_back(toPreparedGroupRoute(snapshotGroupRoute));

        const auto path = "groupRoutes[" + std::to_string(index) + "]";
        for (const auto& zoneId : snapshotGroupRoute.zoneIds)
        {
            if (!preparedZoneIds.count(zoneId))
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "missing-prepared-group-zone",
                           path + ".zoneIds",
                           "Prepared playback did not retain zone '" + zoneId
                               + "' required by group '" + snapshotGroupRoute.groupId + "'.");
            }
        }
    }

    result.metrics.preparedSampleCount = result.prepared.samples.size();
    result.metrics.preparedStreamCount = result.prepared.streams.size();
    result.metrics.preparedZoneCount = result.prepared.zones.size();
    result.prepared.performanceProgram = snapshotResult.snapshot.performanceProgram;
    result.metrics.preparedPerformanceProgramBytes = result.prepared.performanceProgram.retainedBytes;
    result.metrics.preparedBytes += result.metrics.preparedPerformanceProgramBytes;
    result.metrics.preparedOwnershipRecordCount = result.prepared.ownershipRecords.size();
    result.metrics.preparedOwnershipBytes = std::accumulate(
        result.prepared.ownershipRecords.begin(),
        result.prepared.ownershipRecords.end(),
        static_cast<std::uint64_t>(0),
        [](std::uint64_t total, const PreparedPlaybackOwnershipRecord& ownership)
        {
            return total + ownership.retainedBytes;
        });
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        result.metrics.activeCachedOwnershipRecordCount = cacheEntries.size();
        result.metrics.retiredOwnershipRecordCount = retiredCacheEntries.size();
        result.metrics.retiredBytesAwaitingCleanup = workerStatus.retiredBytesAwaitingCleanup;
    }

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
        result.admission.readiness = PreparedPlaybackReadinessState::playable;
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

void PreparedPlaybackService::registerPageServiceSource(const SampleDataSourcePtr& source)
{
    if (source == nullptr)
        return;
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        pageServiceSources.emplace_back(source);
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
        workerStatus.inFlightSourceOrdinal = 0;
        workerStatus.inFlightSourceCount = 0;
        workerStatus.inFlightProgressPhase.clear();
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
        const auto retainedAtCompletion = workerStatus.retiredBytesAwaitingCleanup
            + std::max(workerStatus.activeOwnershipBytes,
                       stepResult.result.metrics.preparedBytes);
        workerStatus.maxObservedRetainedPreparedBytes = std::max(
            workerStatus.maxObservedRetainedPreparedBytes, retainedAtCompletion);
        if (retainedAtCompletion > schedulerBudgets.maximumRetainedPreparedBytes)
            ++workerStatus.retainedPreparedBytesBudgetViolationCount;
        workerStatus.lastEvent = stepResult.result.state;
        workerStatus.inFlightWorkCount = 0;
        workerStatus.inFlightBuildId = 0;
        workerStatus.inFlightSourceOrdinal = 0;
        workerStatus.inFlightSourceCount = 0;
        workerStatus.inFlightProgressPhase.clear();
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
    {
        std::lock_guard<std::mutex> workerLock(workerMutex);
        std::lock_guard<std::mutex> reclaimerLock(reclaimerMutex);
        while (!retiredCacheEntries.empty() && retiredCount < maxEntries
               && reclamationCount < reclamationQueue.size())
        {
            auto& target = reclamationQueue[reclamationWriteIndex];
            target.key = std::move(retiredCacheEntries.back().first);
            target.entry = std::move(retiredCacheEntries.back().second);
            retiredCacheEntries.pop_back();
            reclamationWriteIndex = (reclamationWriteIndex + 1) % reclamationQueue.size();
            ++reclamationCount;
            ++retiredCount;

            if (workerStatus.retiredBytesAwaitingCleanup >= target.entry.retainedBytes)
                workerStatus.retiredBytesAwaitingCleanup -= target.entry.retainedBytes;
            else
                workerStatus.retiredBytesAwaitingCleanup = 0;
        }

        if (retiredCount != 0)
            workerStatus.lastEvent = "Queued " + std::to_string(retiredCount)
                + " stale prepared cache entr"
                + (retiredCount == 1 ? "y" : "ies") + " for background reclamation";

        refreshWorkerStatus();
    }

    if (retiredCount != 0)
        reclaimerCondition.notify_one();
    return retiredCount;
}

bool PreparedPlaybackService::waitForBackgroundReclamation(
    const std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(reclaimerMutex);
    return reclaimerCondition.wait_for(lock, timeout, [this]
    {
        return reclamationCount == 0 && !reclamationInFlight;
    });
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
    PreparedPlaybackWorkerStatus result;
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        result = workerStatus;
    }
    {
        std::lock_guard<std::mutex> lock(reclaimerMutex);
        result.pendingBackgroundReclamationCount
            = reclamationCount + (reclamationInFlight ? 1u : 0u);
    }
    result.completedBackgroundReclamationCount
        = completedBackgroundReclamationCount.load(std::memory_order_relaxed);
    result.lastBackgroundReclamationMicros
        = lastBackgroundReclamationMicros.load(std::memory_order_relaxed);
    result.maxBackgroundReclamationMicros
        = maxBackgroundReclamationMicros.load(std::memory_order_relaxed);
    result.lastBackgroundReclaimerThreadHash
        = lastBackgroundReclaimerThreadHash.load(std::memory_order_relaxed);
    return result;
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
        bool hasJob = false;

        {
            std::unique_lock<std::mutex> lock(workerMutex);
            workerCondition.wait_for(
                lock,
                std::chrono::milliseconds(std::max<std::uint64_t>(
                    1, schedulerBudgets.pageServicePollMilliseconds)),
                [this]
                {
                    return stopWorkerRequested
                        || (!queuedJobs.empty() && workerStreamConfigured
                            && completedResults.size() < schedulerBudgets.maximumCompletedResults);
                });

            if (stopWorkerRequested)
                break;

            if (!queuedJobs.empty() && workerStreamConfigured
                && completedResults.size() < schedulerBudgets.maximumCompletedResults)
            {
                const auto selectedIterator = selectNextQueuedJob();

                job = *selectedIterator;
                queuedJobs.erase(selectedIterator);
                streamResult = workerStreamResult;
                hasJob = true;
                workerStatus.inFlightWorkCount = 1;
                workerStatus.inFlightBuildId = job.request.buildId;
                workerStatus.inFlightLane = job.lane;
                workerStatus.inFlightSourceOrdinal = 0;
                workerStatus.inFlightSourceCount = 0;
                workerStatus.inFlightProgressPhase.clear();
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
        }

        if (!hasJob)
        {
            serviceStreamPageRequests();
            continue;
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
            const auto retainedAtCompletion = workerStatus.retiredBytesAwaitingCleanup
                + std::max(workerStatus.activeOwnershipBytes,
                           stepResult.result.metrics.preparedBytes);
            workerStatus.maxObservedRetainedPreparedBytes = std::max(
                workerStatus.maxObservedRetainedPreparedBytes, retainedAtCompletion);
            if (retainedAtCompletion > schedulerBudgets.maximumRetainedPreparedBytes)
                ++workerStatus.retainedPreparedBytesBudgetViolationCount;
            workerStatus.inFlightWorkCount = 0;
            workerStatus.inFlightBuildId = 0;
            workerStatus.inFlightSourceOrdinal = 0;
            workerStatus.inFlightSourceCount = 0;
            workerStatus.inFlightProgressPhase.clear();
            workerStatus.lastEvent = stepResult.result.state;
            refreshWorkerStatus();
        }

        workerIdleCondition.notify_all();
    }
}

void PreparedPlaybackService::runBackgroundReclaimer()
{
    for (;;)
    {
        CacheReclamationEntry entry;
        {
            std::unique_lock<std::mutex> lock(reclaimerMutex);
            reclaimerCondition.wait(lock, [this]
            {
                return stopReclaimerRequested || reclamationCount != 0;
            });
            if (reclamationCount == 0 && stopReclaimerRequested)
                break;

            auto& queued = reclamationQueue[reclamationReadIndex];
            entry.key = std::move(queued.key);
            entry.entry = std::move(queued.entry);
            reclamationReadIndex = (reclamationReadIndex + 1) % reclamationQueue.size();
            --reclamationCount;
            reclamationInFlight = true;
        }

        const auto startedAt = Clock::now();
        entry.entry.sample = {};
        entry.entry.stream = {};
        entry.entry.ownership = {};
        entry.key.clear();
        const auto elapsedMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - startedAt).count());
        lastBackgroundReclamationMicros.store(elapsedMicros, std::memory_order_relaxed);
        lastBackgroundReclaimerThreadHash.store(
            static_cast<std::uint64_t>(std::hash<std::thread::id> {}(
                std::this_thread::get_id())),
            std::memory_order_relaxed);
        completedBackgroundReclamationCount.fetch_add(1, std::memory_order_relaxed);
        auto maximum = maxBackgroundReclamationMicros.load(std::memory_order_relaxed);
        while (maximum < elapsedMicros
               && !maxBackgroundReclamationMicros.compare_exchange_weak(
                   maximum, elapsedMicros, std::memory_order_relaxed))
        {
        }

        {
            std::lock_guard<std::mutex> lock(reclaimerMutex);
            reclamationInFlight = false;
        }
        reclaimerCondition.notify_all();
    }
}

bool PreparedPlaybackService::serviceStreamPageRequests()
{
    std::vector<SampleDataSourcePtr> sources;
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        pageServiceSources.erase(
            std::remove_if(pageServiceSources.begin(),
                           pageServiceSources.end(),
                           [](const auto& source) { return source.expired(); }),
            pageServiceSources.end());
        sources.reserve(cacheEntries.size() + pageServiceSources.size());
        for (const auto& source : pageServiceSources)
        {
            if (auto retained = source.lock())
                sources.push_back(std::move(retained));
        }
        for (const auto& entry : cacheEntries)
        {
            if (entry.second.sample.dataSource != nullptr)
                sources.push_back(entry.second.sample.dataSource);
        }
    }

    std::uint64_t intentCount = 0;
    std::uint64_t preparedCount = 0;
    std::uint64_t failureCount = 0;
    for (const auto& source : sources)
    {
        SamplePageRequestScheduler scheduler(8);
        if (const auto constWav = std::dynamic_pointer_cast<const WavPagedSampleDataSource>(source))
        {
            const auto wav = std::const_pointer_cast<WavPagedSampleDataSource>(constWav);
            intentCount += wav->drainPageIntents(scheduler, 8);
            SamplePageRequest request;
            while (scheduler.popNext(request))
            {
                if (wav->preparePage(request.pageIndex))
                    ++preparedCount;
                else
                    ++failureCount;
            }
        }
        else if (const auto constPackage = std::dynamic_pointer_cast<const PackagePagedSampleDataSource>(source))
        {
            const auto package = std::const_pointer_cast<PackagePagedSampleDataSource>(constPackage);
            intentCount += package->drainPageIntents(scheduler, 8);
            SamplePageRequest request;
            while (scheduler.popNext(request))
            {
                if (package->preparePage(request.pageIndex))
                    ++preparedCount;
                else
                    ++failureCount;
            }
        }
    }

    if (intentCount == 0)
        return false;

    {
        std::lock_guard<std::mutex> lock(workerMutex);
        workerStatus.pageIntentCount += intentCount;
        workerStatus.pagePrepareCount += preparedCount;
        workerStatus.pagePrepareFailureCount += failureCount;
        workerStatus.lastEvent = failureCount == 0
            ? "Prepared streamed sample pages"
            : "Streamed sample page preparation reported failures";
        refreshWorkerStatus();
    }
    return preparedCount != 0;
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
        if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            value["velocityCrossfade"] = serializeVelocityCrossfade(zone.velocityCrossfade);
        if (hasAnyVelocityCrossfadeRuntimeValue(zone.velocityCrossfadeRuntime))
            value["velocityCrossfadeRuntime"] = serializeVelocityCrossfadeRuntime(zone.velocityCrossfadeRuntime);
        value["gainDb"] = zone.gainDb;
        value["pan"] = zone.pan;
        value["fineTuneCents"] = zone.fineTuneCents;
        value["amplitudeVelocityTracking"] = zone.amplitudeVelocityTracking;
        ordered_json authoredConditions = ordered_json::array();
        for (const auto& condition : zone.controllerConditions)
            authoredConditions.push_back({ { "controllerNumber", condition.controllerNumber },
                                           { "minimumValue", condition.minimumValue },
                                           { "maximumValue", condition.maximumValue } });
        value["controllerConditions"] = std::move(authoredConditions);
        value["sampleStartFrame"] = zone.sampleStartFrame;
        value["loopEnabled"] = zone.loopEnabled;
        value["loopStartFrame"] = zone.loopStartFrame;
        value["loopEndFrame"] = zone.loopEndFrame;
        value["releaseSeconds"] = zone.releaseSeconds;
        value["releaseShape"] = zone.releaseShape;
        if (zone.roundRobin.has_value())
            value["roundRobin"] = serializeRoundRobin(*zone.roundRobin);
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
        if (hasAnyVelocityCrossfadeValue(handle.velocityCrossfade))
            value["velocityCrossfade"] = serializeVelocityCrossfade(handle.velocityCrossfade);
        if (hasAnyVelocityCrossfadeRuntimeValue(handle.velocityCrossfadeRuntime))
            value["velocityCrossfadeRuntime"] = serializeVelocityCrossfadeRuntime(handle.velocityCrossfadeRuntime);
        value["gainDb"] = handle.gainDb;
        value["pan"] = handle.pan;
        value["fineTuneCents"] = handle.fineTuneCents;
        value["amplitudeVelocityTracking"] = handle.amplitudeVelocityTracking;
        ordered_json preparedConditions = ordered_json::array();
        for (const auto& condition : handle.controllerConditions)
            preparedConditions.push_back({ { "controllerNumber", condition.controllerNumber },
                                           { "minimumValue", condition.minimumValue },
                                           { "maximumValue", condition.maximumValue } });
        value["controllerConditions"] = std::move(preparedConditions);
        value["sampleStartFrame"] = handle.sampleStartFrame;
        value["loopEnabled"] = handle.loopEnabled;
        value["loopStartFrame"] = handle.loopStartFrame;
        value["loopEndFrame"] = handle.loopEndFrame;
        value["releaseSeconds"] = handle.releaseSeconds;
        value["releaseShape"] = handle.releaseShape;
        if (handle.roundRobin.has_value())
            value["roundRobin"] = serializeRoundRobin(*handle.roundRobin);
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
        root["groupRoutes"].push_back({
            { "id", route.groupId },
            { "articulations", route.articulationIds },
            { "zones", route.zoneIds },
            { "displayName", route.displayName },
            { "displayOrder", route.displayOrder },
            { "routingSourceId", route.routingSourceId },
            { "gainDb", route.gainDb },
            { "pan", route.pan },
            { "routingBusId", route.routingBusId },
            { "auditionAnchorZoneId", route.auditionAnchorZoneId }
        });
    }

    auto preparedGroupRoutes = prepared.groupRoutes;
    std::sort(preparedGroupRoutes.begin(), preparedGroupRoutes.end(), [](const auto& left, const auto& right)
    {
        return left.groupId < right.groupId;
    });
    root["preparedGroupRoutes"] = ordered_json::array();
    for (auto& route : preparedGroupRoutes)
    {
        std::sort(route.articulationIds.begin(), route.articulationIds.end());
        std::sort(route.zoneIds.begin(), route.zoneIds.end());
        root["preparedGroupRoutes"].push_back({
            { "id", route.groupId },
            { "articulations", route.articulationIds },
            { "zones", route.zoneIds },
            { "displayName", route.displayName },
            { "displayOrder", route.displayOrder },
            { "routingSourceId", route.routingSourceId },
            { "gainDb", route.gainDb },
            { "pan", route.pan },
            { "routingBusId", route.routingBusId },
            { "auditionAnchorZoneId", route.auditionAnchorZoneId }
        });
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
    const auto sourcesEqual = [&]()
    {
        if (left.dataSource == nullptr || right.dataSource == nullptr)
            return left.dataSource == nullptr && right.dataSource == nullptr;
        const auto& leftDescriptor = left.dataSource->descriptor();
        const auto& rightDescriptor = right.dataSource->descriptor();
        return leftDescriptor.kind == rightDescriptor.kind
            && leftDescriptor.canonicalSourceIdentity == rightDescriptor.canonicalSourceIdentity
            && leftDescriptor.provenanceIdentity == rightDescriptor.provenanceIdentity
            && leftDescriptor.generation == rightDescriptor.generation
            && leftDescriptor.frameCount == rightDescriptor.frameCount
            && leftDescriptor.channelCount == rightDescriptor.channelCount
            && leftDescriptor.headSizeBytes == rightDescriptor.headSizeBytes
            && leftDescriptor.pageSizeBytes == rightDescriptor.pageSizeBytes;
    };
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
        && sourcesEqual()
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
        && left.velocityCrossfade.fadeInLowVelocity == right.velocityCrossfade.fadeInLowVelocity
        && left.velocityCrossfade.fadeInHighVelocity == right.velocityCrossfade.fadeInHighVelocity
        && left.velocityCrossfade.fadeOutLowVelocity == right.velocityCrossfade.fadeOutLowVelocity
        && left.velocityCrossfade.fadeOutHighVelocity == right.velocityCrossfade.fadeOutHighVelocity
        && left.velocityCrossfadeRuntime.effectiveLowVelocity == right.velocityCrossfadeRuntime.effectiveLowVelocity
        && left.velocityCrossfadeRuntime.effectiveHighVelocity == right.velocityCrossfadeRuntime.effectiveHighVelocity
        && left.velocityCrossfadeRuntime.fadeInNeighborZoneId == right.velocityCrossfadeRuntime.fadeInNeighborZoneId
        && left.velocityCrossfadeRuntime.fadeOutNeighborZoneId == right.velocityCrossfadeRuntime.fadeOutNeighborZoneId
        && left.velocityCrossfadeRuntime.fadeInOverlapLowVelocity == right.velocityCrossfadeRuntime.fadeInOverlapLowVelocity
        && left.velocityCrossfadeRuntime.fadeInOverlapHighVelocity == right.velocityCrossfadeRuntime.fadeInOverlapHighVelocity
        && left.velocityCrossfadeRuntime.fadeOutOverlapLowVelocity == right.velocityCrossfadeRuntime.fadeOutOverlapLowVelocity
        && left.velocityCrossfadeRuntime.fadeOutOverlapHighVelocity == right.velocityCrossfadeRuntime.fadeOutOverlapHighVelocity
        && left.gainDb == right.gainDb
        && left.pan == right.pan
        && left.fineTuneCents == right.fineTuneCents
        && left.amplitudeVelocityTracking == right.amplitudeVelocityTracking
        && left.controllerConditions == right.controllerConditions
        && left.sampleStartFrame == right.sampleStartFrame
        && left.loopEnabled == right.loopEnabled
        && left.loopStartFrame == right.loopStartFrame
        && left.loopEndFrame == right.loopEndFrame
        && left.releaseSeconds == right.releaseSeconds
        && left.releaseShape == right.releaseShape
        && left.roundRobin == right.roundRobin
        && left.roundRobinLength == right.roundRobinLength
        && left.roundRobinPosition == right.roundRobinPosition
        && left.triggerMode == right.triggerMode;
}

bool operator==(const PreparedPlaybackGroupRoute& left, const PreparedPlaybackGroupRoute& right)
{
    return left.groupId == right.groupId
        && left.articulationIds == right.articulationIds
        && left.zoneIds == right.zoneIds
        && left.displayName == right.displayName
        && left.displayOrder == right.displayOrder
        && left.routingSourceId == right.routingSourceId
        && left.workspaceVisible == right.workspaceVisible
        && left.gainDb == right.gainDb
        && left.pan == right.pan
        && left.routingBusId == right.routingBusId
        && left.auditionAnchorZoneId == right.auditionAnchorZoneId;
}

bool operator==(const ImmutablePreparedPlayback& left, const ImmutablePreparedPlayback& right)
{
    return left.snapshotBuildId == right.snapshotBuildId
        && left.snapshotContentDigest == right.snapshotContentDigest
        && left.compilerVersion == right.compilerVersion
        && left.draftRevision == right.draftRevision
        && left.selectedGroupId == right.selectedGroupId
        && left.masterGainDb == right.masterGainDb
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
        && left.groupRoutes == right.groupRoutes
        && left.zones == right.zones
        && left.controllerDefaults == right.controllerDefaults
        && serializeCompiledPerformanceProgram(left.performanceProgram)
            == serializeCompiledPerformanceProgram(right.performanceProgram)
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

std::string toString(PreparedPlaybackReadinessState state)
{
    switch (state)
    {
    case PreparedPlaybackReadinessState::metadataLoaded:
        return "metadata-loaded";
    case PreparedPlaybackReadinessState::playbackDeferred:
        return "playback-deferred";
    case PreparedPlaybackReadinessState::playable:
        return "playable";
    case PreparedPlaybackReadinessState::streamingRequired:
        return "streaming-required";
    }
    return "unknown";
}
} // namespace drs::engine
