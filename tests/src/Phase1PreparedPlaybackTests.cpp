#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool containsFinding(const drs::engine::PreparedPlaybackBuildResult& result,
                     drs::engine::PlaybackSnapshotFindingSeverity severity,
                     const std::string& code,
                     const std::string& pathFragment)
{
    for (const auto& finding : result.findings)
    {
        if (finding.severity == severity
            && finding.code == code
            && finding.path.find(pathFragment) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

const drs::engine::PlaybackSnapshotFinding* findFinding(const drs::engine::PreparedPlaybackBuildResult& result,
                                                        drs::engine::PlaybackSnapshotFindingSeverity severity,
                                                        const std::string& code,
                                                        const std::string& pathFragment)
{
    for (const auto& finding : result.findings)
    {
        if (finding.severity == severity
            && finding.code == code
            && finding.path.find(pathFragment) != std::string::npos)
        {
            return &finding;
        }
    }

    return nullptr;
}

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-phase1-prepared-playback-tests";
    fs::create_directories(path);
    return path;
}

juce::AudioBuffer<float> buildReferenceBuffer()
{
    constexpr int frameCount = 480;
    juce::AudioBuffer<float> buffer(2, frameCount);
    buffer.clear();

    for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
    {
        buffer.setSample(0, sampleIndex, 0.25f);
        buffer.setSample(1, sampleIndex, -0.25f);
    }

    return buffer;
}

juce::AudioBuffer<float> buildAlternateReferenceBuffer()
{
    constexpr int frameCount = 480;
    juce::AudioBuffer<float> buffer(2, frameCount);
    buffer.clear();

    for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
    {
        const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
        buffer.setSample(0, sampleIndex, std::sin((phase * juce::MathConstants<float>::twoPi) * 2.0f) * 0.4f);
        buffer.setSample(1, sampleIndex, std::cos((phase * juce::MathConstants<float>::twoPi) * 3.0f) * 0.2f);
    }

    return buffer;
}

void writeAudioFile(const fs::path& filePath,
                    juce::AudioFormat& format,
                    const juce::AudioBuffer<float>& buffer,
                    double sampleRate,
                    const juce::StringPairArray& metadata = {})
{
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open prepared-playback fixture for writing: " + filePath.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(sampleRate)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    for (const auto& key : metadata.getAllKeys())
        options = options.withMetadata(key, metadata[key]);

    auto writerOwner = format.createWriterFor(output, options);
    require(writerOwner != nullptr,
            "Could not create prepared-playback audio writer for: " + filePath.generic_string());
    require(writerOwner->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write prepared-playback fixture audio: " + filePath.generic_string());
}

void applyImportedMetadataToStreamSample(const drs::engine::SampleImportResult& importResult,
                                         drs::engine::RuntimeStreamSampleDefinition& streamSample,
                                         const std::string& sourcePath)
{
    require(importResult.imported,
            "Prepared-playback invalidation coverage requires a decodable source fixture before stream metadata can be updated.");
    streamSample.sourcePath = sourcePath;
    streamSample.sourceChecksumHex = importResult.sample.metadata.sourceChecksumHex;
    streamSample.formatName = importResult.sample.metadata.formatName;
    streamSample.channelLayout = importResult.sample.metadata.channelLayout;
    streamSample.sampleRate = importResult.sample.metadata.sampleRate;
    streamSample.frameCount = importResult.sample.metadata.frameCount;
    streamSample.channelCount = importResult.sample.metadata.channelCount;
    streamSample.rootMidiNotePresent = importResult.sample.metadata.rootMidiNotePresent;
    streamSample.rootMidiNote = importResult.sample.metadata.rootMidiNote;
    streamSample.loopRangePresent = importResult.sample.metadata.loopRangePresent;
    streamSample.loopStartFrame = importResult.sample.metadata.loopStartFrame;
    streamSample.loopEndFrame = importResult.sample.metadata.loopEndFrame;
}

std::string normalizePath(const std::string& pathText)
{
    return fs::path(pathText).lexically_normal().generic_string();
}

const drs::engine::RuntimeStreamSampleDefinition* findStreamSample(
    const drs::engine::RuntimeStreamLoadResult& stream,
    const std::string& sampleId)
{
    const auto iterator = std::find_if(
        stream.container.samples.begin(),
        stream.container.samples.end(),
        [&sampleId](const drs::engine::RuntimeStreamSampleDefinition& sample)
        {
            return sample.sampleId == sampleId;
        });

    return iterator == stream.container.samples.end() ? nullptr : &(*iterator);
}

std::uint64_t computeExpectedStreamedPayloadBytes(const drs::engine::RuntimeStreamSampleDefinition& sample)
{
    return sample.payloadSizeBytes >= sample.prefetchBytes
        ? sample.payloadSizeBytes - sample.prefetchBytes
        : 0;
}

std::uint64_t computeExpectedPreparedSampleDataBytes(const drs::engine::ImmutablePreparedPlayback& prepared)
{
    std::uint64_t sampleDataBytes = 0;

    for (const auto& sample : prepared.samples)
    {
        require(sample.decodedSampleData != nullptr,
                "Prepared sample handles should retain decoded sample data for playback reuse.");

        for (const auto& channel : sample.decodedSampleData->normalizedChannels)
        {
            sampleDataBytes += static_cast<std::uint64_t>(channel.size())
                * static_cast<std::uint64_t>(sizeof(float));
        }
    }

    return sampleDataBytes;
}

std::vector<std::string> collectPreparedCacheKeys(const drs::engine::ImmutablePreparedPlayback& prepared)
{
    std::vector<std::string> cacheKeys;
    cacheKeys.reserve(prepared.samples.size());

    for (const auto& sample : prepared.samples)
        cacheKeys.push_back(sample.cacheKey);

    return cacheKeys;
}

std::size_t countChangedPreparedCacheKeys(const drs::engine::ImmutablePreparedPlayback& before,
                                          const drs::engine::ImmutablePreparedPlayback& after)
{
    require(before.samples.size() == after.samples.size(),
            "Prepared cache-key comparison requires matching sample counts.");

    std::size_t changedCount = 0;
    for (std::size_t index = 0; index < before.samples.size(); ++index)
    {
        if (before.samples[index].cacheKey != after.samples[index].cacheKey)
            ++changedCount;
    }

    return changedCount;
}
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring fixture must load before prepared playback tests run.");

        const auto referenceManifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(referenceManifest.loaded, "Phase 1 reference manifest must load before prepared playback tests run.");

        const auto referenceStream = drs::engine::loadRuntimeStreamContainerForInstrument(referenceManifest);
        require(referenceStream.loaded, "Phase 1 reference stream must load before prepared playback tests run.");

        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        drs::engine::PreparedPlaybackService preparedService;

        const auto firstSnapshotRequest = snapshotBuilder.requestBuild(0, true);
        require(firstSnapshotRequest.accepted, "Initial playback snapshot request should be accepted.");
        const auto firstSnapshot = snapshotBuilder.buildSnapshot(firstSnapshotRequest, phase2Project.project);
        require(firstSnapshot.built, "Initial playback snapshot should build successfully.");

        const auto firstPreparedRequest = preparedService.requestBuild(firstSnapshot, referenceStream);
        require(firstPreparedRequest.accepted, "Prepared playback request should be accepted for a valid snapshot.");
        require(firstPreparedRequest.snapshotBuildId == firstSnapshot.buildId,
                "Prepared playback request should track the immutable snapshot build identity.");
        require(firstPreparedRequest.requestedDraftRevision == firstSnapshot.requestedDraftRevision,
                "Prepared playback request should track the requested draft revision.");
        require(firstPreparedRequest.activationRequested == firstSnapshot.activationRequested,
                "Prepared playback request should preserve whether activation was requested.");
        require(firstPreparedRequest.cancellationId == firstPreparedRequest.buildId,
                "Prepared playback request should seed cancellation identity from its build identity.");
        require(firstPreparedRequest.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::preparing,
                "Accepted prepared playback requests should begin in the preparing state.");
        require(firstPreparedRequest.sampleResolutionReady,
                "Prepared playback requests should carry sample-resolution decisions when a stream container is available.");
        require(firstPreparedRequest.sampleResolutions.size() == firstSnapshot.snapshot.sampleIdentities.size(),
                "Prepared playback requests should carry one sample-resolution entry per immutable snapshot sample.");
        require(!firstPreparedRequest.sampleResolutions[0].selectedStreamSampleId.empty(),
                "Prepared playback sample-resolution entries should select a concrete stream sample id.");
        require(!firstPreparedRequest.sampleResolutions[0].selectedFormatName.empty(),
                "Prepared playback sample-resolution entries should select a concrete format name.");
        require(firstPreparedRequest.sampleResolutions[0].matchedBySourcePath
                    || firstPreparedRequest.sampleResolutions[0].matchedBySampleSourceId,
                "Prepared playback sample-resolution entries should record how the worker request matched the stream sample.");
        const auto firstPrepared = preparedService.prepare(firstPreparedRequest, firstSnapshot, referenceStream);
        require(firstPrepared.built, "Prepared playback should build from the reference snapshot.");
        require(firstPrepared.activationEligible, "Prepared playback should remain activation-eligible for valid content.");
        require(firstPrepared.buildId == firstPreparedRequest.buildId,
                "Prepared playback result should preserve the request build identity.");
        require(firstPrepared.snapshotBuildId == firstPreparedRequest.snapshotBuildId,
                "Prepared playback result should preserve the immutable snapshot build identity.");
        require(firstPrepared.requestedDraftRevision == firstPreparedRequest.requestedDraftRevision,
                "Prepared playback result should preserve the requested draft revision.");
        require(firstPrepared.activationRequested == firstPreparedRequest.activationRequested,
                "Prepared playback result should preserve whether activation was requested.");
        require(firstPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Successful prepared playback should finish in the ready state.");
        require(firstPrepared.buildDurationMicros > 0,
                "Prepared playback result should report a non-zero build duration.");
        require(firstPrepared.metrics.preparedSampleCount == phase2Project.project.sampleSources.size(),
                "Prepared sample count changed unexpectedly.");
        require(firstPrepared.metrics.preparedZoneCount == phase2Project.project.authoring.zones.size(),
                "Prepared zone count changed unexpectedly.");
        require(firstPrepared.metrics.cacheMissCount == phase2Project.project.sampleSources.size(),
                "First prepared playback build should cold-miss every sample handle.");
        require(firstPrepared.metrics.cacheHitCount == 0,
                "First prepared playback build should not report cache hits.");
        require(firstPrepared.metrics.preparedSampleDataBytes
                    == computeExpectedPreparedSampleDataBytes(firstPrepared.prepared),
                "Prepared playback should expose a deterministic prepared sample-data footprint for the built content.");
        require(firstPrepared.metrics.decodedBytes == computeExpectedPreparedSampleDataBytes(firstPrepared.prepared),
                "First prepared playback build should decode every prepared source sample through the preparation service.");
        require(!firstPrepared.prepared.preparedContentDigest.empty(),
                "Prepared playback builds must carry a deterministic content digest.");
        require(firstPrepared.prepared.preparedContentDigest
                    == drs::engine::computePreparedPlaybackContentDigest(firstPrepared.prepared),
                "Prepared playback builds must expose a digest derived from deterministic prepared serialization.");
        require(firstPrepared.prepared.selectedGroupId == firstSnapshot.snapshot.selectedGroupId,
                "Prepared playback should preserve the selected group identity from the immutable snapshot.");
        require(firstPrepared.prepared.groupRoutes.size() == firstSnapshot.snapshot.groupRoutes.size(),
                "Prepared playback should retain one immutable group route per snapshot group.");
        require(firstPrepared.metrics.preparedOwnershipRecordCount == firstPrepared.prepared.ownershipRecords.size(),
                "Prepared playback metrics should expose ownership-record counts.");
        require(firstPrepared.metrics.preparedOwnershipBytes == firstPrepared.metrics.preparedBytes,
                "Prepared playback metrics should expose ownership-safe retained-byte totals.");
        require(firstPrepared.metrics.preparedBytes == firstPrepared.metrics.preparedSampleDataBytes,
                "Prepared playback residency bytes should match the retained decoded sample-data footprint.");
        require(firstPrepared.metrics.activeCachedOwnershipRecordCount == firstPrepared.prepared.ownershipRecords.size(),
                "Prepared playback metrics should expose active cached ownership-record counts.");
        require(firstPrepared.metrics.retiredOwnershipRecordCount == 0,
                "Fresh prepared playback builds should not report retired ownership backlog.");
        require(firstPrepared.metrics.retiredBytesAwaitingCleanup == 0,
                "Fresh prepared playback builds should not report retired ownership bytes.");
        require(firstPrepared.prepared.ownershipRecords.size() == firstPrepared.prepared.samples.size(),
                "Prepared playback should expose one ownership record per prepared cache-backed sample.");
        require(firstPrepared.prepared.samples[0].ownershipToken.find("cache:") == 0,
                "Prepared sample handles should expose an explicit ownership token.");
        const auto firstPreparedSerialization = drs::engine::serializeImmutablePreparedPlayback(firstPrepared.prepared);
        const auto firstPreparedContentSerialization = drs::engine::serializePreparedPlaybackContent(firstPrepared.prepared);
        require(firstPreparedSerialization == drs::engine::serializeImmutablePreparedPlayback(firstPrepared.prepared),
                "Prepared playback serialization should be deterministic when repeated for the same immutable payload.");
        require(firstPreparedContentSerialization == drs::engine::serializePreparedPlaybackContent(firstPrepared.prepared),
                "Prepared playback content serialization should be deterministic when repeated for the same immutable payload.");
        require(firstPreparedSerialization.find("\"groupRoutes\"") != std::string::npos
                    && firstPreparedSerialization.find("\"routingSourceId\"") != std::string::npos
                    && firstPreparedSerialization.find("\"workspaceVisible\"") != std::string::npos,
                "Sprint 3 prepared playback serialization must retain group route metadata.");
        require(firstPrepared.prepared.groupRoutes[0].groupId == firstSnapshot.snapshot.groupRoutes[0].groupId
                    && firstPrepared.prepared.groupRoutes[0].gainDb == firstSnapshot.snapshot.groupRoutes[0].gainDb
                    && firstPrepared.prepared.groupRoutes[0].pan == firstSnapshot.snapshot.groupRoutes[0].pan,
                "Prepared playback should preserve immutable group route mix metadata.");
        require(firstPrepared.prepared.samples[0].canonicalSourcePath
                    == normalizePath(firstPrepared.prepared.samples[0].sourcePath),
                "Prepared sample handles should expose a normalized canonical source path.");
        require(firstPrepared.prepared.samples[0].canonicalSourceIdentity
                    == firstPrepared.prepared.samples[0].sampleSourceId + "|"
                        + firstPrepared.prepared.samples[0].canonicalSourcePath,
                "Prepared sample handles should expose a canonical source identity derived from source id and path.");
        require(!firstPrepared.prepared.samples[0].channelLayout.empty(),
                "Prepared sample handles should expose channel-layout metadata.");
        require(!firstPrepared.prepared.samples[0].sourceFingerprintHex.empty(),
                "Prepared sample handles should expose source fingerprint metadata.");
        require(std::find(firstPrepared.prepared.notes.begin(),
                          firstPrepared.prepared.notes.end(),
                          "Prepared cache key contract: canonical-source-identity + source-fingerprint + decode-policy + compiler-version")
                    != firstPrepared.prepared.notes.end(),
                "Prepared playback should record the prepared cache-key contract in the immutable notes.");

        for (const auto& preparedSample : firstPrepared.prepared.samples)
        {
            const auto* streamSample = findStreamSample(referenceStream, preparedSample.streamSampleId);
            require(streamSample != nullptr,
                    "Prepared sample handles should map back to a runtime stream sample definition.");
            require(preparedSample.ownershipRecordIndex < firstPrepared.prepared.ownershipRecords.size(),
                    "Prepared sample handles should point at a valid ownership record.");
            const auto& ownershipRecord = firstPrepared.prepared.ownershipRecords[preparedSample.ownershipRecordIndex];
            require(ownershipRecord.ownershipToken == preparedSample.ownershipToken,
                    "Prepared sample handles should preserve ownership-token identity through the ownership record.");
            require(ownershipRecord.cacheKey == preparedSample.cacheKey,
                    "Prepared sample handles should preserve cache-key identity through the ownership record.");
            require(ownershipRecord.sampleSourceId == preparedSample.sampleSourceId,
                    "Prepared sample handles should preserve sample-source identity through the ownership record.");
            require(ownershipRecord.streamSampleId == preparedSample.streamSampleId,
                    "Prepared sample handles should preserve stream-sample identity through the ownership record.");
            require(ownershipRecord.lifetimeState == "active-cache-entry",
                    "Fresh prepared ownership records should begin in the active cache-entry state.");
            require(ownershipRecord.retirementToken.empty(),
                    "Active prepared ownership records should not carry a retirement token yet.");
            require(ownershipRecord.preparedBuildId == firstPrepared.buildId,
                    "Fresh ownership records should track the build that created the cache entry.");
            require(ownershipRecord.retiredByBuildId == 0,
                    "Fresh ownership records should not report a retiring build id.");
            require(preparedSample.channelLayout == streamSample->channelLayout,
                    "Prepared sample handles should preserve runtime stream channel-layout metadata.");
            require(preparedSample.sampleRate == streamSample->sampleRate,
                    "Prepared sample handles should preserve runtime stream sample-rate metadata.");
            require(preparedSample.frameCount == streamSample->frameCount,
                    "Prepared sample handles should preserve runtime stream frame-count metadata.");
            require(preparedSample.sourceFingerprintHex == streamSample->sourceChecksumHex,
                    "Prepared sample handles should preserve runtime stream source fingerprint metadata.");
            require(preparedSample.loopRangePresent == streamSample->loopRangePresent,
                    "Prepared sample handles should preserve loop-range presence metadata.");
            require(preparedSample.decodedSampleData != nullptr,
                    "Prepared sample handles should retain immutable decoded sample data.");
            require(preparedSample.decodedSampleData->normalizedChannels.size() == preparedSample.channelCount,
                    "Prepared sample handles should retain one normalized channel per decoded channel.");

            const auto importedPreparedSample = drs::engine::importSampleFile(preparedSample.sourcePath);
            require(importedPreparedSample.imported,
                    "Prepared sample data retention checks must be able to re-import the source sample.");
            require(importedPreparedSample.sample.normalizedChannels
                        == preparedSample.decodedSampleData->normalizedChannels,
                    "Prepared sample handles should retain the decoded normalized channel buffers, not just metadata.");

            if (streamSample->loopRangePresent)
            {
                require(preparedSample.loopStartFrame == streamSample->loopStartFrame,
                        "Prepared sample handles should preserve loop start metadata.");
                require(preparedSample.loopEndFrame == streamSample->loopEndFrame,
                        "Prepared sample handles should preserve loop end metadata.");
                require(preparedSample.loopEndFrame >= preparedSample.loopStartFrame,
                        "Prepared sample loop metadata should preserve a valid loop range.");
            }
        }

        for (const auto& preparedStream : firstPrepared.prepared.streams)
        {
            const auto* streamSample = findStreamSample(referenceStream, preparedStream.streamSampleId);
            require(streamSample != nullptr,
                    "Prepared stream handles should map back to a runtime stream sample definition.");
            require(preparedStream.ownershipRecordIndex < firstPrepared.prepared.ownershipRecords.size(),
                    "Prepared stream handles should point at a valid ownership record.");
            const auto& ownershipRecord = firstPrepared.prepared.ownershipRecords[preparedStream.ownershipRecordIndex];
            require(ownershipRecord.ownershipToken == preparedStream.ownershipToken,
                    "Prepared stream handles should preserve ownership-token identity through the ownership record.");
            require(ownershipRecord.cacheKey == preparedStream.cacheKey,
                    "Prepared stream handles should preserve cache-key identity through the ownership record.");
            require(preparedStream.containerId == referenceStream.container.containerId,
                    "Prepared stream handles should preserve runtime stream container identity.");
            require(preparedStream.containerPath == referenceStream.containerPath,
                    "Prepared stream handles should preserve runtime stream container path.");
            require(preparedStream.payloadEncoding == referenceStream.container.payloadEncoding,
                    "Prepared stream handles should preserve runtime stream payload encoding.");
            require(preparedStream.pageSizeBytes == referenceStream.container.pageSizeBytes,
                    "Prepared stream handles should preserve runtime stream page size.");
            require(preparedStream.payloadOffsetBytes == streamSample->payloadOffsetBytes,
                    "Prepared stream handles should preserve runtime stream payload offset.");
            require(preparedStream.payloadSizeBytes == streamSample->payloadSizeBytes,
                    "Prepared stream handles should preserve runtime stream payload size.");
            require(preparedStream.prefetchBytes == streamSample->prefetchBytes,
                    "Prepared stream handles should preserve runtime stream prefetch size.");
            require(preparedStream.streamedPayloadOffsetBytes
                        == streamSample->payloadOffsetBytes + streamSample->prefetchBytes,
                    "Prepared stream handles should expose the streamed payload start offset.");
            require(preparedStream.streamedPayloadBytes == computeExpectedStreamedPayloadBytes(*streamSample),
                    "Prepared stream handles should expose the streamed payload byte count.");

            if (streamSample->pages.empty())
            {
                require(preparedStream.topologyKind == "explicit-pages",
                        "Prefetch-only runtime stream samples should expose an explicit empty page topology.");
                require(preparedStream.pageCount == 0,
                        "Prepared stream handles should preserve zero page count for prefetch-only samples.");
                require(!preparedStream.pageRangePresent,
                        "Prepared stream handles should not claim a page range when no streamed pages exist.");
            }
            else
            {
                require(preparedStream.topologyKind == "explicit-pages",
                        "Prepared stream handles should expose explicit page topology when page tables are available.");
                require(preparedStream.pageCount == streamSample->pages.size(),
                        "Prepared stream handles should preserve runtime stream page counts.");
                require(preparedStream.pageRangePresent,
                        "Prepared stream handles should expose a page range when streamed pages exist.");
                require(preparedStream.firstPageIndex == streamSample->pages.front().pageIndex,
                        "Prepared stream handles should preserve first page index metadata.");
                require(preparedStream.lastPageIndex == streamSample->pages.back().pageIndex,
                        "Prepared stream handles should preserve last page index metadata.");
                require(preparedStream.firstPageOffsetBytes == streamSample->pages.front().offsetBytes,
                        "Prepared stream handles should preserve first page offset metadata.");
                require(preparedStream.lastPageOffsetBytes == streamSample->pages.back().offsetBytes,
                        "Prepared stream handles should preserve last page offset metadata.");
                require(preparedStream.lastPageSizeBytes == streamSample->pages.back().sizeBytes,
                        "Prepared stream handles should preserve last page size metadata.");
                require(preparedStream.pages.size() == streamSample->pages.size(),
                        "Prepared stream handles should preserve explicit page tables.");

                for (std::size_t pageIndex = 0; pageIndex < streamSample->pages.size(); ++pageIndex)
                {
                    require(preparedStream.pages[pageIndex].pageIndex == streamSample->pages[pageIndex].pageIndex,
                            "Prepared stream handle page indices should preserve runtime stream topology.");
                    require(preparedStream.pages[pageIndex].offsetBytes == streamSample->pages[pageIndex].offsetBytes,
                            "Prepared stream handle page offsets should preserve runtime stream topology.");
                    require(preparedStream.pages[pageIndex].sizeBytes == streamSample->pages[pageIndex].sizeBytes,
                            "Prepared stream handle page sizes should preserve runtime stream topology.");
                }
            }
        }

        const auto secondSnapshotRequest = snapshotBuilder.requestBuild(0, true);
        const auto secondSnapshot = snapshotBuilder.buildSnapshot(secondSnapshotRequest, phase2Project.project);
        const auto secondPreparedRequest = preparedService.requestBuild(secondSnapshot, referenceStream);
        const auto secondPrepared = preparedService.prepare(secondPreparedRequest, secondSnapshot, referenceStream);
        require(secondPrepared.built, "Repeated prepared playback build should still succeed.");
        require(secondPrepared.prepared.preparedContentDigest == firstPrepared.prepared.preparedContentDigest,
                "Repeated preparation of the same snapshot should produce the same prepared digest.");
        require(secondPrepared.prepared.preparedContentDigest
                    == drs::engine::computePreparedPlaybackContentDigest(secondPrepared.prepared),
                "Repeated prepared playback builds must preserve the deterministic prepared digest contract.");
        require(secondPrepared.metrics.cacheHitCount == phase2Project.project.sampleSources.size(),
                "Warm prepared playback build should hit the cache for every sample handle.");
        require(secondPrepared.metrics.cacheMissCount == 0,
                "Warm prepared playback build should not cold-miss unchanged sample handles.");
        require(secondPrepared.metrics.preparedSampleDataBytes == firstPrepared.metrics.preparedSampleDataBytes,
                "Warm prepared playback build should preserve deterministic prepared sample-data bytes.");
        require(secondPrepared.metrics.decodedBytes == 0,
                "Warm prepared playback build should not re-decode unchanged sample handles.");
        require(secondPrepared.prepared.samples[0].canonicalSourceIdentity
                    == firstPrepared.prepared.samples[0].canonicalSourceIdentity,
                "Repeated preparation of the same snapshot should preserve canonical source identity.");
        require(secondPrepared.prepared.samples[0].channelLayout == firstPrepared.prepared.samples[0].channelLayout,
                "Repeated preparation of the same snapshot should preserve channel-layout metadata.");
        require(secondPrepared.prepared.samples[0].ownershipToken == firstPrepared.prepared.samples[0].ownershipToken,
                "Repeated preparation of the same snapshot should preserve ownership-token identity.");
        require(secondPrepared.prepared.streams[0].topologyKind == firstPrepared.prepared.streams[0].topologyKind,
                "Repeated preparation of the same snapshot should preserve stream topology kind.");
        require(secondPrepared.prepared.streams[0].pageCount == firstPrepared.prepared.streams[0].pageCount,
                "Repeated preparation of the same snapshot should preserve stream topology page counts.");
        require(secondPrepared.prepared.ownershipRecords[0].ownershipToken
                    == firstPrepared.prepared.ownershipRecords[0].ownershipToken,
                "Repeated preparation of the same snapshot should preserve ownership-record identity.");
        require(secondPrepared.metrics.preparedOwnershipRecordCount == firstPrepared.metrics.preparedOwnershipRecordCount,
                "Repeated preparation of the same snapshot should preserve ownership-record counts.");
        require(secondPrepared.metrics.preparedSampleCount == firstPrepared.metrics.preparedSampleCount
                    && secondPrepared.metrics.preparedStreamCount == firstPrepared.metrics.preparedStreamCount
                    && secondPrepared.metrics.preparedZoneCount == firstPrepared.metrics.preparedZoneCount,
                "Repeated preparation of the same snapshot should preserve deterministic prepared counts.");
        require(secondPrepared.metrics.preparedBytes == firstPrepared.metrics.preparedBytes
                    && secondPrepared.metrics.preparedOwnershipBytes == firstPrepared.metrics.preparedOwnershipBytes,
                "Repeated preparation of the same snapshot should preserve deterministic retained-byte metrics.");
        require(secondPrepared.metrics.activeCachedOwnershipRecordCount
                    == firstPrepared.metrics.activeCachedOwnershipRecordCount,
                "Repeated preparation of the same snapshot should preserve active cached ownership counts.");
        require(secondPrepared.prepared.samples == firstPrepared.prepared.samples,
                "Repeated preparation of the same snapshot should preserve prepared sample-handle equality.");
        require(secondPrepared.prepared.streams == firstPrepared.prepared.streams,
                "Repeated preparation of the same snapshot should preserve prepared stream-handle equality.");
        require(secondPrepared.prepared.ownershipRecords == firstPrepared.prepared.ownershipRecords,
                "Repeated preparation of the same snapshot should preserve prepared ownership-record equality.");
        require(secondPrepared.prepared.zones == firstPrepared.prepared.zones,
                "Repeated preparation of the same snapshot should preserve prepared zone-handle equality.");
        const auto secondPreparedSerialization = drs::engine::serializeImmutablePreparedPlayback(secondPrepared.prepared);
        const auto secondPreparedContentSerialization = drs::engine::serializePreparedPlaybackContent(secondPrepared.prepared);
        require(secondPreparedSerialization != firstPreparedSerialization,
                "Full prepared playback serialization should preserve unique snapshot-build identity across repeated builds.");
        require(secondPreparedContentSerialization == firstPreparedContentSerialization,
                "Repeated preparation of the same snapshot should preserve deterministic content serialization.");

        auto visibilityProject = phase2Project.project;
        visibilityProject.authoring.groups[0].workspaceVisible = !visibilityProject.authoring.groups[0].workspaceVisible;
        const auto visibilitySnapshotRequest = snapshotBuilder.requestBuild(0, true);
        const auto visibilitySnapshot = snapshotBuilder.buildSnapshot(visibilitySnapshotRequest, visibilityProject);
        require(visibilitySnapshot.built,
                "Visibility-only authored group edits should still produce an immutable snapshot.");
        require(visibilitySnapshot.snapshot.contentDigest == firstSnapshot.snapshot.contentDigest,
                "Group workspace visibility must not alter the immutable snapshot digest.");
        const auto visibilityPreparedRequest = preparedService.requestBuild(visibilitySnapshot, referenceStream);
        require(visibilityPreparedRequest.accepted,
                "Prepared playback should accept visibility-only authored group edits.");
        const auto visibilityPrepared = preparedService.prepare(visibilityPreparedRequest, visibilitySnapshot, referenceStream);
        require(visibilityPrepared.built,
                "Visibility-only authored group edits should still prepare successfully.");
        require(visibilityPrepared.prepared.preparedContentDigest == firstPrepared.prepared.preparedContentDigest,
                "Group workspace visibility must not alter prepared playback content digests.");
        require(visibilityPrepared.metrics.cacheHitCount == phase2Project.project.sampleSources.size()
                    && visibilityPrepared.metrics.cacheMissCount == 0
                    && visibilityPrepared.metrics.decodedBytes == 0,
                "Visibility-only authored group edits should fully reuse warm prepared assets.");
        require(drs::engine::serializeImmutablePreparedPlayback(visibilityPrepared.prepared)
                    != firstPreparedSerialization,
                "Serialized prepared playback should still expose visibility-only group state changes.");

        const auto firstPreparedCacheKeys = collectPreparedCacheKeys(firstPrepared.prepared);

        drs::engine::RuntimeProjectDocumentController zoneOnlyController(phase2Project.project);
        auto zoneOnlyProject = zoneOnlyController.getProject();
        zoneOnlyProject.authoring.zones[0].rootKey += 2;
        zoneOnlyProject.authoring.zones[0].keyLow += 1;
        zoneOnlyProject.authoring.zones[0].keyHigh -= 1;
        zoneOnlyProject.authoring.zones[0].velocityLow += 4;
        zoneOnlyProject.authoring.zones[0].velocityHigh -= 5;
        zoneOnlyProject.authoring.zones[0].gainDb += 1.25;
        zoneOnlyProject.authoring.zones[0].pan = -0.35;
        zoneOnlyProject.authoring.zones[0].sampleStartFrame += 32;
        zoneOnlyProject.authoring.zones[0].loopEnabled = !zoneOnlyProject.authoring.zones[0].loopEnabled;
        const auto zoneOnlyCommit = zoneOnlyController.commitSnapshot(zoneOnlyProject,
                                                                      "Adjust zone-only mapping and mix values",
                                                                      {"authoring.zones[0].rootKey",
                                                                       "authoring.zones[0].keyLow",
                                                                       "authoring.zones[0].keyHigh",
                                                                       "authoring.zones[0].velocityLow",
                                                                       "authoring.zones[0].velocityHigh",
                                                                       "authoring.zones[0].gainDb",
                                                                       "authoring.zones[0].pan",
                                                                       "authoring.zones[0].sampleStartFrame",
                                                                       "authoring.zones[0].loopEnabled"});
        require(zoneOnlyCommit.applied, "Zone-only prepared playback coverage should commit successfully.");
        const auto zoneOnlySnapshotRequest = snapshotBuilder.requestBuild(zoneOnlyCommit.documentState.revision, true);
        const auto zoneOnlySnapshot = snapshotBuilder.buildSnapshot(zoneOnlySnapshotRequest, zoneOnlyController.getProject());
        require(zoneOnlySnapshot.built, "Zone-only prepared playback coverage should still build an immutable snapshot.");
        require(zoneOnlySnapshot.snapshot.contentDigest != firstSnapshot.snapshot.contentDigest,
                "Zone-only authoring edits should change the immutable snapshot digest.");
        const auto zoneOnlyPreparedRequest = preparedService.requestBuild(zoneOnlySnapshot, referenceStream);
        require(zoneOnlyPreparedRequest.accepted,
                "Zone-only prepared playback coverage should accept the rebuilt immutable snapshot.");
        const auto zoneOnlyPrepared = preparedService.prepare(zoneOnlyPreparedRequest, zoneOnlySnapshot, referenceStream);
        require(zoneOnlyPrepared.built && zoneOnlyPrepared.activationEligible,
                "Zone-only authoring edits should still prepare successfully.");
        require(zoneOnlyPrepared.prepared.preparedContentDigest != firstPrepared.prepared.preparedContentDigest,
                "Zone-only authoring edits should change the prepared digest because zone content changed.");
        require(zoneOnlyPrepared.metrics.cacheHitCount == phase2Project.project.sampleSources.size(),
                "Zone-only authoring edits should reuse every prepared sample handle.");
        require(zoneOnlyPrepared.metrics.cacheMissCount == 0,
                "Zone-only authoring edits should not invalidate source-backed prepared assets.");
        require(zoneOnlyPrepared.metrics.decodedBytes == 0,
                "Zone-only authoring edits should not re-decode prepared sample handles.");
        require(zoneOnlyPrepared.metrics.preparedSampleDataBytes == firstPrepared.metrics.preparedSampleDataBytes,
                "Zone-only authoring edits should preserve deterministic prepared sample-data bytes.");
        require(collectPreparedCacheKeys(zoneOnlyPrepared.prepared) == firstPreparedCacheKeys,
                "Zone-only authoring edits should preserve prepared cache-key identity.");
        require(zoneOnlyPrepared.prepared.samples == firstPrepared.prepared.samples,
                "Zone-only authoring edits should preserve prepared sample handles.");
        require(zoneOnlyPrepared.prepared.streams == firstPrepared.prepared.streams,
                "Zone-only authoring edits should preserve prepared stream handles.");
        require(zoneOnlyPrepared.prepared.ownershipRecords == firstPrepared.prepared.ownershipRecords,
                "Zone-only authoring edits should preserve ownership records.");
        require(zoneOnlyPrepared.prepared.zones[0].rootKey == zoneOnlyProject.authoring.zones[0].rootKey
                    && zoneOnlyPrepared.prepared.zones[0].keyLow == zoneOnlyProject.authoring.zones[0].keyLow
                    && zoneOnlyPrepared.prepared.zones[0].keyHigh == zoneOnlyProject.authoring.zones[0].keyHigh
                    && zoneOnlyPrepared.prepared.zones[0].velocityLow == zoneOnlyProject.authoring.zones[0].velocityLow
                    && zoneOnlyPrepared.prepared.zones[0].velocityHigh == zoneOnlyProject.authoring.zones[0].velocityHigh
                    && zoneOnlyPrepared.prepared.zones[0].gainDb == zoneOnlyProject.authoring.zones[0].gainDb
                    && zoneOnlyPrepared.prepared.zones[0].pan == zoneOnlyProject.authoring.zones[0].pan
                    && zoneOnlyPrepared.prepared.zones[0].sampleStartFrame
                        == zoneOnlyProject.authoring.zones[0].sampleStartFrame
                    && zoneOnlyPrepared.prepared.zones[0].loopEnabled == zoneOnlyProject.authoring.zones[0].loopEnabled,
                "Zone-only authoring edits should still flow into prepared zone mapping and mix content.");
        require(zoneOnlyPrepared.prepared.zones != firstPrepared.prepared.zones,
                "Zone-only authoring edits should update prepared zone content even when asset cache keys stay warm.");
        require(countChangedPreparedCacheKeys(firstPrepared.prepared, zoneOnlyPrepared.prepared) == 0,
                "Zone-only cache-correctness coverage should keep every prepared cache key warm.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        auto editedProject = controller.getProject();
        editedProject.sampleSources[1].path = editedProject.sampleSources[0].path;
        const auto commitResult = controller.commitSnapshot(editedProject,
                                                            "Swap the lead source path to invalidate one prepared key",
                                                            {"sampleSources[1].path"});
        require(commitResult.applied, "Edited project revision should commit before prepared playback rebuild.");

        const auto editedSnapshotRequest = snapshotBuilder.requestBuild(commitResult.documentState.revision, true);
        const auto editedSnapshot = snapshotBuilder.buildSnapshot(editedSnapshotRequest, controller.getProject());
        require(editedSnapshot.built, "Edited snapshot should still build successfully.");
        const auto editedPreparedRequest = preparedService.requestBuild(editedSnapshot, referenceStream);
        const auto editedPrepared = preparedService.prepare(editedPreparedRequest, editedSnapshot, referenceStream);
        require(editedPrepared.built, "Edited prepared playback should still succeed.");
        require(editedPrepared.prepared.preparedContentDigest != firstPrepared.prepared.preparedContentDigest,
                "Changing a sample source path should invalidate the prepared digest.");
        require(editedPrepared.prepared.preparedContentDigest
                    == drs::engine::computePreparedPlaybackContentDigest(editedPrepared.prepared),
                "Edited prepared playback builds must recompute the deterministic prepared digest.");
        require(editedPrepared.metrics.cacheHitCount == 1,
                "Changing one source path should preserve exactly one cached prepared asset.");
        require(editedPrepared.metrics.cacheMissCount == 1,
                "Changing one source path should invalidate exactly one cached prepared asset.");
        require(editedPrepared.metrics.preparedSampleDataBytes
                    == computeExpectedPreparedSampleDataBytes(editedPrepared.prepared),
                "Edited prepared playback should preserve deterministic prepared sample-data bytes for the rebuilt content.");
        require(editedPrepared.metrics.decodedBytes > 0,
                "Changing one source path should re-decode the newly prepared cache miss.");
        require(editedPrepared.prepared.samples.size() >= 2,
                "Edited prepared playback coverage expects at least two prepared samples.");
        require(editedPrepared.prepared.samples[0].sourceFingerprintHex
                    == editedPrepared.prepared.samples[1].sourceFingerprintHex,
                "Relinking one source path to another fixture should preserve the same source fingerprint.");
        require(editedPrepared.prepared.samples[0].canonicalSourceIdentity
                    != editedPrepared.prepared.samples[1].canonicalSourceIdentity,
                "Prepared cache identity should distinguish duplicate source files assigned to different sample-source ids.");
        require(editedPrepared.prepared.samples[0].cacheKey != editedPrepared.prepared.samples[1].cacheKey,
                "Prepared cache keys should distinguish canonical source identity even when two sample sources point at the same file.");
        require(countChangedPreparedCacheKeys(firstPrepared.prepared, editedPrepared.prepared) == 1,
                "Relink cache-correctness coverage should change exactly one prepared cache key.");
        require(editedPrepared.metrics.preparedOwnershipRecordCount == editedPrepared.prepared.ownershipRecords.size(),
                "Edited prepared playback metrics should expose ownership-record counts.");
        require(editedPrepared.metrics.activeCachedOwnershipRecordCount == editedPrepared.prepared.ownershipRecords.size(),
                "Edited prepared playback metrics should expose active cached ownership counts.");
        require(editedPrepared.metrics.retiredOwnershipRecordCount >= 1,
                "Edited prepared playback metrics should expose retired ownership backlog after invalidation.");
        require(editedPrepared.metrics.retiredBytesAwaitingCleanup > 0,
                "Edited prepared playback metrics should expose retired ownership bytes after invalidation.");
        require(drs::engine::serializePreparedPlaybackContent(editedPrepared.prepared) != firstPreparedContentSerialization,
                "Changing a sample source path should produce different prepared content serialization text.");

        auto invalidProject = phase2Project.project;
        invalidProject.sampleSources[0].path = invalidProject.contentRootPath + "/Samples/does-not-exist.wav";
        const auto invalidSnapshotRequest = snapshotBuilder.requestBuild(3, true);
        const auto invalidSnapshot = snapshotBuilder.buildSnapshot(invalidSnapshotRequest, invalidProject);
        require(!invalidSnapshot.built, "Invalid snapshot should fail before prepared playback begins.");
        const auto rejectedPreparedRequest = preparedService.requestBuild(invalidSnapshot);
        require(!rejectedPreparedRequest.accepted,
                "Prepared playback request must reject failed immutable snapshots.");
        require(rejectedPreparedRequest.snapshotBuildId == invalidSnapshot.buildId,
                "Rejected prepared playback request should still point at the failed snapshot build identity.");
        require(rejectedPreparedRequest.requestedDraftRevision == invalidSnapshot.requestedDraftRevision,
                "Rejected prepared playback request should preserve the failed snapshot draft revision.");
        require(rejectedPreparedRequest.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Rejected prepared playback request should surface the failed lifecycle state.");
        const auto rejectedPrepared = preparedService.prepare(rejectedPreparedRequest, invalidSnapshot, referenceStream);
        require(!rejectedPrepared.built && !rejectedPrepared.activationEligible,
                "Rejected prepared playback result must not become activation-eligible.");
        require(rejectedPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Rejected prepared playback result should remain in the failed lifecycle state.");
        require(containsFinding(rejectedPrepared,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "missing-sample-source-asset",
                                "sampleSources[0].path"),
                "Rejected prepared playback result should preserve the immutable snapshot findings that caused rejection.");

        const auto missingPreparedPath = scratchDirectory / "prepared-missing-source.wav";
        fs::copy_file(phase2Project.project.sampleSources[0].path,
                      missingPreparedPath,
                      fs::copy_options::overwrite_existing);
        auto missingPreparedProject = phase2Project.project;
        missingPreparedProject.sampleSources[0].path = missingPreparedPath.generic_string();
        drs::engine::PreparedPlaybackService missingPreparedService;
        const auto missingPreparedSnapshotRequest = snapshotBuilder.requestBuild(4, true);
        const auto missingPreparedSnapshot = snapshotBuilder.buildSnapshot(missingPreparedSnapshotRequest,
                                                                           missingPreparedProject);
        require(missingPreparedSnapshot.built,
                "Prepared-playback missing-source coverage should start from a valid immutable snapshot.");
        const auto missingPreparedRequest = missingPreparedService.requestBuild(missingPreparedSnapshot, referenceStream);
        require(missingPreparedRequest.accepted,
                "Prepared-playback missing-source coverage should accept the immutable snapshot before the worker loss occurs.");
        fs::remove(missingPreparedPath);
        const auto missingPreparedResult = missingPreparedService.prepare(missingPreparedRequest,
                                                                         missingPreparedSnapshot,
                                                                         referenceStream);
        require(!missingPreparedResult.built && !missingPreparedResult.activationEligible,
                "Prepared playback should fail when the worker can no longer access the resolved source asset.");
        const auto* missingPreparedFinding = findFinding(missingPreparedResult,
                                                         drs::engine::PlaybackSnapshotFindingSeverity::error,
                                                         "prepared-sample-source-missing",
                                                         "sampleIdentities[0].sourcePath");
        require(missingPreparedFinding != nullptr,
                "Prepared playback should surface a structured source-missing finding when the worker loses the file.");
        require(missingPreparedFinding->message.find("Sample missing") != std::string::npos,
                "Prepared source-missing findings should preserve importer state detail.");

        const auto unsupportedPreparedPath = scratchDirectory / "prepared-unsupported-source.wav";
        fs::copy_file(phase2Project.project.sampleSources[0].path,
                      unsupportedPreparedPath,
                      fs::copy_options::overwrite_existing);
        auto unsupportedPreparedProject = phase2Project.project;
        unsupportedPreparedProject.sampleSources[0].path = unsupportedPreparedPath.generic_string();
        drs::engine::PreparedPlaybackService unsupportedPreparedService;
        const auto unsupportedPreparedSnapshotRequest = snapshotBuilder.requestBuild(5, true);
        const auto unsupportedPreparedSnapshot = snapshotBuilder.buildSnapshot(unsupportedPreparedSnapshotRequest,
                                                                               unsupportedPreparedProject);
        require(unsupportedPreparedSnapshot.built,
                "Prepared-playback unsupported-format coverage should start from a valid immutable snapshot.");
        const auto unsupportedPreparedRequest = unsupportedPreparedService.requestBuild(unsupportedPreparedSnapshot,
                                                                                        referenceStream);
        require(unsupportedPreparedRequest.accepted,
                "Prepared-playback unsupported-format coverage should accept the immutable snapshot before the worker decode.");
        {
            juce::FileOutputStream unsupportedOutput(juce::File(unsupportedPreparedPath.generic_string()));
            require(unsupportedOutput.openedOk(),
                    "Could not rewrite prepared-playback unsupported-format fixture.");
            unsupportedOutput.setPosition(0);
            unsupportedOutput.truncate();
            unsupportedOutput.writeText("not audio", false, false, nullptr);
        }
        const auto unsupportedPreparedResult = unsupportedPreparedService.prepare(unsupportedPreparedRequest,
                                                                                  unsupportedPreparedSnapshot,
                                                                                  referenceStream);
        require(!unsupportedPreparedResult.built && !unsupportedPreparedResult.activationEligible,
                "Prepared playback should fail when the worker sees an unsupported source format.");
        const auto* unsupportedPreparedFinding = findFinding(unsupportedPreparedResult,
                                                             drs::engine::PlaybackSnapshotFindingSeverity::error,
                                                             "prepared-sample-format-unsupported",
                                                             "sampleIdentities[0].sourcePath");
        require(unsupportedPreparedFinding != nullptr,
                "Prepared playback should surface a structured unsupported-format finding.");
        require(unsupportedPreparedFinding->message.find("Sample format unsupported") != std::string::npos,
                "Prepared unsupported-format findings should preserve importer state detail.");

        const auto rejectedDecodePath = scratchDirectory / "prepared-high-rate-source.wav";
        const auto rejectionBuffer = buildReferenceBuffer();
        juce::WavAudioFormat wavFormat;
        writeAudioFile(rejectedDecodePath, wavFormat, rejectionBuffer, 96000.0);
        auto rejectedDecodeProject = phase2Project.project;
        rejectedDecodeProject.sampleSources[0].path = rejectedDecodePath.generic_string();
        drs::engine::PreparedPlaybackService rejectedDecodeService;
        const auto rejectedDecodeSnapshotRequest = snapshotBuilder.requestBuild(6, true);
        const auto rejectedDecodeSnapshot = snapshotBuilder.buildSnapshot(rejectedDecodeSnapshotRequest,
                                                                          rejectedDecodeProject);
        require(rejectedDecodeSnapshot.built,
                "Prepared-playback decode-failure coverage should start from a valid immutable snapshot.");
        const auto rejectedDecodeRequest = rejectedDecodeService.requestBuild(rejectedDecodeSnapshot, referenceStream);
        require(rejectedDecodeRequest.accepted,
                "Prepared-playback decode-failure coverage should accept the immutable snapshot before policy validation.");
        const auto rejectedDecodeResult = rejectedDecodeService.prepare(rejectedDecodeRequest,
                                                                        rejectedDecodeSnapshot,
                                                                        referenceStream);
        require(!rejectedDecodeResult.built && !rejectedDecodeResult.activationEligible,
                "Prepared playback should fail when worker-side decode policy rejects the source.");
        const auto* rejectedDecodeFinding = findFinding(rejectedDecodeResult,
                                                        drs::engine::PlaybackSnapshotFindingSeverity::error,
                                                        "prepared-sample-decode-failed",
                                                        "sampleIdentities[0].sourcePath");
        require(rejectedDecodeFinding != nullptr,
                "Prepared playback should surface a structured decode-failure finding for policy-rejected sources.");
        require(rejectedDecodeFinding->message.find("Phase 1 sample policy rejected") != std::string::npos,
                "Prepared decode-failure findings should preserve policy-rejection detail.");

        auto policyShiftStream = referenceStream;
        policyShiftStream.container.pageSizeBytes += 4096;
        drs::engine::PreparedPlaybackService policyShiftPreparedService;
        const auto policyShiftPreparedRequest = policyShiftPreparedService.requestBuild(firstSnapshot, policyShiftStream);
        require(policyShiftPreparedRequest.accepted,
                "Prepared playback should accept the same immutable snapshot under an alternate decode policy container.");
        const auto policyShiftPrepared = policyShiftPreparedService.prepare(policyShiftPreparedRequest,
                                                                           firstSnapshot,
                                                                           policyShiftStream);
        require(policyShiftPrepared.built,
                "Prepared playback should still build when only the prepared decode policy fingerprint changes.");
        require(policyShiftPrepared.prepared.samples[0].canonicalSourceIdentity
                    == firstPrepared.prepared.samples[0].canonicalSourceIdentity,
                "Decode-policy shifts should preserve canonical source identity.");
        require(policyShiftPrepared.prepared.samples[0].sourceFingerprintHex
                    == firstPrepared.prepared.samples[0].sourceFingerprintHex,
                "Decode-policy shifts should preserve source fingerprint identity.");
        require(policyShiftPrepared.prepared.samples[0].cacheKey != firstPrepared.prepared.samples[0].cacheKey,
                "Prepared cache keys should change when the decode policy fingerprint changes.");
        require(policyShiftPrepared.prepared.streams[0].pageSizeBytes != firstPrepared.prepared.streams[0].pageSizeBytes,
                "Decode-policy shift coverage should mutate the prepared stream page-size metadata.");

        drs::engine::PreparedPlaybackService compilerSaltPreparedService("phase1-prepared-playback-v2-compiler-salt");
        const auto compilerSaltPreparedRequest = compilerSaltPreparedService.requestBuild(firstSnapshot, referenceStream);
        require(compilerSaltPreparedRequest.accepted,
                "Prepared playback should accept the same immutable snapshot under an alternate compiler salt.");
        const auto compilerSaltPrepared = compilerSaltPreparedService.prepare(compilerSaltPreparedRequest,
                                                                             firstSnapshot,
                                                                             referenceStream);
        require(compilerSaltPrepared.built,
                "Prepared playback should still build when only the compiler salt changes.");
        require(compilerSaltPrepared.prepared.samples[0].canonicalSourceIdentity
                    == firstPrepared.prepared.samples[0].canonicalSourceIdentity,
                "Compiler-salt shifts should preserve canonical source identity.");
        require(compilerSaltPrepared.prepared.samples[0].sourceFingerprintHex
                    == firstPrepared.prepared.samples[0].sourceFingerprintHex,
                "Compiler-salt shifts should preserve source fingerprint identity.");
        require(compilerSaltPrepared.prepared.streams[0].pageSizeBytes
                    == firstPrepared.prepared.streams[0].pageSizeBytes,
                "Compiler-salt shifts should not mutate the decode policy metadata.");
        require(compilerSaltPrepared.prepared.samples[0].cacheKey != firstPrepared.prepared.samples[0].cacheKey,
                "Prepared cache keys should change when the compiler/version salt changes.");

        const auto replaceSamplePath = scratchDirectory / "prepared-replace-sample-source.wav";
        juce::WavAudioFormat replaceSampleFormat;
        juce::StringPairArray replaceSampleMetadata;
        replaceSampleMetadata.set("NumSampleLoops", "1");
        replaceSampleMetadata.set("Loop0Start", "48");
        replaceSampleMetadata.set("Loop0End", "216");
        writeAudioFile(replaceSamplePath,
                       replaceSampleFormat,
                       buildAlternateReferenceBuffer(),
                       48000.0,
                       replaceSampleMetadata);
        auto replaceSampleProject = phase2Project.project;
        replaceSampleProject.sampleSources[0].path = replaceSamplePath.generic_string();
        auto replaceSampleStream = referenceStream;
        const auto replaceSampleImport = drs::engine::importSampleFile(replaceSamplePath.generic_string());
        applyImportedMetadataToStreamSample(replaceSampleImport,
                                            replaceSampleStream.container.samples[0],
                                            replaceSamplePath.generic_string());
        drs::engine::PreparedPlaybackService replaceSamplePreparedService;
        const auto replaceBaselinePreparedRequest = replaceSamplePreparedService.requestBuild(firstSnapshot, referenceStream);
        require(replaceBaselinePreparedRequest.accepted,
                "Replace-sample cache-correctness coverage should accept the baseline immutable snapshot.");
        const auto replaceBaselinePrepared = replaceSamplePreparedService.prepare(replaceBaselinePreparedRequest,
                                                                                  firstSnapshot,
                                                                                  referenceStream);
        require(replaceBaselinePrepared.built,
                "Replace-sample cache-correctness coverage should build the baseline prepared state.");
        const auto replaceSampleSnapshotRequest = snapshotBuilder.requestBuild(8, true);
        const auto replaceSampleSnapshot = snapshotBuilder.buildSnapshot(replaceSampleSnapshotRequest,
                                                                         replaceSampleProject);
        require(replaceSampleSnapshot.built,
                "Replace-sample cache-correctness coverage should build a valid immutable snapshot.");
        const auto replaceSamplePreparedRequest = replaceSamplePreparedService.requestBuild(replaceSampleSnapshot,
                                                                                           replaceSampleStream);
        require(replaceSamplePreparedRequest.accepted,
                "Replace-sample cache-correctness coverage should accept the replaced-source immutable snapshot.");
        const auto replaceSamplePrepared = replaceSamplePreparedService.prepare(replaceSamplePreparedRequest,
                                                                               replaceSampleSnapshot,
                                                                               replaceSampleStream);
        require(replaceSamplePrepared.built,
                "Replace-sample cache-correctness coverage should still prepare successfully.");
        require(replaceSamplePrepared.metrics.cacheHitCount == 1,
                "Replace-sample cache-correctness coverage should preserve one warm prepared asset.");
        require(replaceSamplePrepared.metrics.cacheMissCount == 1,
                "Replace-sample cache-correctness coverage should invalidate exactly one prepared asset.");
        require(replaceSamplePrepared.metrics.decodedBytes > 0,
                "Replace-sample cache-correctness coverage should re-decode the replaced prepared asset.");
        require(replaceSamplePrepared.prepared.samples[0].canonicalSourceIdentity
                    != replaceBaselinePrepared.prepared.samples[0].canonicalSourceIdentity,
                "Replace-sample cache-correctness coverage should change canonical source identity for the replaced asset.");
        require(replaceSamplePrepared.prepared.samples[0].sourceFingerprintHex
                    != replaceBaselinePrepared.prepared.samples[0].sourceFingerprintHex,
                "Replace-sample cache-correctness coverage should change the source fingerprint for the replaced asset.");
        require(countChangedPreparedCacheKeys(replaceBaselinePrepared.prepared, replaceSamplePrepared.prepared) == 1,
                "Replace-sample cache-correctness coverage should change exactly one prepared cache key.");
        require(replaceSamplePrepared.prepared.samples[1].cacheKey
                    == replaceBaselinePrepared.prepared.samples[1].cacheKey,
                "Replace-sample cache-correctness coverage should preserve the cache key for unchanged assets.");

        const auto checksumShiftPath = scratchDirectory / "prepared-checksum-shift-source.wav";
        juce::WavAudioFormat wavFormatWithMetadata;
        juce::StringPairArray loopMetadata;
        loopMetadata.set("NumSampleLoops", "1");
        loopMetadata.set("Loop0Start", "96");
        loopMetadata.set("Loop0End", "240");
        writeAudioFile(checksumShiftPath,
                       wavFormatWithMetadata,
                       buildReferenceBuffer(),
                       48000.0,
                       loopMetadata);
        auto checksumShiftProject = phase2Project.project;
        checksumShiftProject.sampleSources[0].path = checksumShiftPath.generic_string();
        auto checksumBaselineStream = referenceStream;
        const auto checksumBaselineImport = drs::engine::importSampleFile(checksumShiftPath.generic_string());
        applyImportedMetadataToStreamSample(checksumBaselineImport,
                                            checksumBaselineStream.container.samples[0],
                                            checksumShiftPath.generic_string());
        drs::engine::PreparedPlaybackService checksumPreparedService;
        const auto checksumBaselineSnapshotRequest = snapshotBuilder.requestBuild(7, true);
        const auto checksumBaselineSnapshot = snapshotBuilder.buildSnapshot(checksumBaselineSnapshotRequest,
                                                                            checksumShiftProject);
        require(checksumBaselineSnapshot.built,
                "Checksum invalidation coverage should start from a valid immutable snapshot.");
        const auto checksumBaselinePreparedRequest = checksumPreparedService.requestBuild(checksumBaselineSnapshot,
                                                                                         checksumBaselineStream);
        require(checksumBaselinePreparedRequest.accepted,
                "Checksum invalidation coverage should accept the baseline immutable snapshot.");
        const auto checksumBaselinePrepared = checksumPreparedService.prepare(checksumBaselinePreparedRequest,
                                                                             checksumBaselineSnapshot,
                                                                             checksumBaselineStream);
        require(checksumBaselinePrepared.built,
                "Checksum invalidation baseline should prepare successfully.");

        writeAudioFile(checksumShiftPath,
                       wavFormatWithMetadata,
                       buildAlternateReferenceBuffer(),
                       48000.0,
                       loopMetadata);
        auto checksumChangedStream = checksumBaselineStream;
        const auto checksumChangedImport = drs::engine::importSampleFile(checksumShiftPath.generic_string());
        applyImportedMetadataToStreamSample(checksumChangedImport,
                                            checksumChangedStream.container.samples[0],
                                            checksumShiftPath.generic_string());
        const auto checksumChangedSnapshotRequest = snapshotBuilder.requestBuild(7, true);
        const auto checksumChangedSnapshot = snapshotBuilder.buildSnapshot(checksumChangedSnapshotRequest,
                                                                           checksumShiftProject);
        require(checksumChangedSnapshot.built,
                "Checksum invalidation coverage should keep the immutable snapshot valid when the source path stays constant.");
        const auto checksumChangedPreparedRequest = checksumPreparedService.requestBuild(checksumChangedSnapshot,
                                                                                        checksumChangedStream);
        require(checksumChangedPreparedRequest.accepted,
                "Checksum invalidation coverage should accept the rebuilt immutable snapshot.");
        const auto checksumChangedPrepared = checksumPreparedService.prepare(checksumChangedPreparedRequest,
                                                                            checksumChangedSnapshot,
                                                                            checksumChangedStream);
        require(checksumChangedPrepared.built,
                "Checksum invalidation coverage should still prepare successfully after content changes.");
        require(checksumChangedPrepared.metrics.cacheHitCount == 1,
                "Changing one source checksum should preserve exactly one warm prepared asset.");
        require(checksumChangedPrepared.metrics.cacheMissCount == 1,
                "Changing one source checksum should invalidate exactly one prepared asset.");
        require(checksumChangedPrepared.metrics.decodedBytes > 0,
                "Changing one source checksum should re-decode the invalidated prepared asset.");
        require(checksumChangedPrepared.prepared.samples[0].canonicalSourceIdentity
                    == checksumBaselinePrepared.prepared.samples[0].canonicalSourceIdentity,
                "Checksum invalidation should preserve canonical source identity when the path stays constant.");
        require(checksumChangedPrepared.prepared.samples[0].sourceFingerprintHex
                    != checksumBaselinePrepared.prepared.samples[0].sourceFingerprintHex,
                "Checksum invalidation should detect a changed source fingerprint.");
        require(checksumChangedPrepared.prepared.samples[0].cacheKey
                    != checksumBaselinePrepared.prepared.samples[0].cacheKey,
                "Checksum invalidation should produce a new prepared cache key for the changed asset.");
        require(checksumChangedPrepared.prepared.samples[1].cacheKey
                    == checksumBaselinePrepared.prepared.samples[1].cacheKey,
                "Checksum invalidation should preserve the cache key for unchanged prepared assets.");

        auto loopPolicyShiftStream = checksumChangedStream;
        loopPolicyShiftStream.container.samples[0].loopRangePresent = false;
        loopPolicyShiftStream.container.samples[0].loopStartFrame = 0;
        loopPolicyShiftStream.container.samples[0].loopEndFrame = 0;
        const auto loopPolicyShiftPreparedRequest = checksumPreparedService.requestBuild(checksumChangedSnapshot,
                                                                                         loopPolicyShiftStream);
        require(loopPolicyShiftPreparedRequest.accepted,
                "Loop-policy invalidation coverage should accept the immutable snapshot.");
        const auto loopPolicyShiftPrepared = checksumPreparedService.prepare(loopPolicyShiftPreparedRequest,
                                                                             checksumChangedSnapshot,
                                                                             loopPolicyShiftStream);
        require(loopPolicyShiftPrepared.built,
                "Loop-policy invalidation coverage should still prepare successfully.");
        require(loopPolicyShiftPrepared.metrics.cacheHitCount == 1,
                "Changing loop-relevant decode policy should preserve exactly one unchanged prepared asset.");
        require(loopPolicyShiftPrepared.metrics.cacheMissCount == 1,
                "Changing loop-relevant decode policy should invalidate exactly one prepared asset.");
        require(loopPolicyShiftPrepared.metrics.decodedBytes > 0,
                "Changing loop-relevant decode policy should force the invalidated asset back through decode.");
        require(loopPolicyShiftPrepared.prepared.samples[0].canonicalSourceIdentity
                    == checksumChangedPrepared.prepared.samples[0].canonicalSourceIdentity,
                "Loop-policy invalidation should preserve canonical source identity.");
        require(loopPolicyShiftPrepared.prepared.samples[0].sourceFingerprintHex
                    == checksumChangedPrepared.prepared.samples[0].sourceFingerprintHex,
                "Loop-policy invalidation should preserve source fingerprint identity.");
        require(loopPolicyShiftPrepared.prepared.samples[0].cacheKey
                    != checksumChangedPrepared.prepared.samples[0].cacheKey,
                "Loop-policy invalidation should produce a new prepared cache key.");
        require(loopPolicyShiftPrepared.prepared.samples[1].cacheKey
                    == checksumChangedPrepared.prepared.samples[1].cacheKey,
                "Loop-policy invalidation should preserve cache keys for unchanged assets.");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before migrated prepared-playback coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate before prepared-playback coverage runs.");

        drs::engine::PlaybackSnapshotBuilder migratedSnapshotBuilder;
        drs::engine::PreparedPlaybackService migratedPreparedService;

        const auto migratedSnapshotRequest = migratedSnapshotBuilder.requestBuild(0, true);
        const auto migratedSnapshot = migratedSnapshotBuilder.buildSnapshot(migratedSnapshotRequest, migratedProject.project);
        require(!migratedSnapshot.built,
                "Migrated Phase 1 project should not build an activation-eligible snapshot before imported zones exist.");
        const auto migratedPreparedRequest = migratedPreparedService.requestBuild(migratedSnapshot);
        require(!migratedPreparedRequest.accepted,
                "Prepared playback request must reject migrated snapshots that still lack playable zones.");
        const auto migratedPreparedRejected = migratedPreparedService.prepare(migratedPreparedRequest,
                                                                             migratedSnapshot,
                                                                             referenceStream);
        require(!migratedPreparedRejected.built,
                "Prepared playback must stay rejected while the migrated project has no imported zones.");
        require(migratedPreparedRejected.snapshotBuildId == migratedSnapshot.buildId,
                "Rejected migrated prepared playback should preserve the snapshot build identity.");
        require(migratedPreparedRejected.requestedDraftRevision == migratedSnapshot.requestedDraftRevision,
                "Rejected migrated prepared playback should preserve the requested draft revision.");
        require(migratedPreparedRejected.prepared.draftRevision == migratedSnapshot.snapshot.draftRevision,
                "Rejected migrated prepared playback should still report the snapshot draft revision.");
        require(migratedPreparedRejected.prepared.samples.empty() && migratedPreparedRejected.prepared.zones.empty(),
                "Rejected migrated prepared playback must not fabricate prepared samples or zones.");
        require(containsFinding(migratedPreparedRejected,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Rejected migrated prepared playback should preserve the structured no-playable-zones finding.");
        require(!containsFinding(migratedPreparedRejected,
                                 drs::engine::PlaybackSnapshotFindingSeverity::error,
                                 "no-sample-identities",
                                 "sampleSources"),
                "Rejected migrated prepared playback should preserve the migrated sample identities while zones are missing.");

        drs::engine::AuthoringSession migratedSession(migratedProject.project);
        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "migrated-import-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "migrated-zone-sine-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Migrated Imported Sine A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto importResult = migratedSession.appendImportedContent({ importedSampleSource },
                                                                        { importedZone },
                                                                        "Import migrated authoring content");
        require(importResult.applied, "Migrated Phase 1 project should accept imported authoring content.");
        require(importResult.documentState.revision == 1,
                "Imported migrated authoring content should advance the document revision.");
        require(migratedSession.getProject().authoring.selectedZoneId == importedZone.id,
                "Imported migrated authoring content should select the imported zone.");

        const auto importedSnapshotRequest = migratedSnapshotBuilder.requestBuild(importResult.documentState.revision, true);
        const auto importedSnapshot = migratedSnapshotBuilder.buildSnapshot(importedSnapshotRequest, migratedSession.getProject());
        require(importedSnapshot.built,
                "Migrated project with imported authoring content should build an immutable snapshot.");
        require(importedSnapshot.snapshot.selectedZoneId == importedZone.id,
                "Imported migrated snapshot should preserve the selected zone.");
        const auto importedPreparedRequest = migratedPreparedService.requestBuild(importedSnapshot);
        require(importedPreparedRequest.accepted,
                "Prepared playback should accept migrated snapshots once imported zones exist.");
        const auto importedPrepared = migratedPreparedService.prepare(importedPreparedRequest, importedSnapshot, referenceStream);
        require(importedPrepared.built && importedPrepared.activationEligible,
                "Prepared playback should succeed for migrated projects once imported authoring content exists.");
        require(importedPrepared.prepared.draftRevision == importResult.documentState.revision,
                "Prepared playback should preserve the imported draft revision.");
        require(importedPrepared.metrics.preparedSampleCount == migratedSession.getProject().sampleSources.size(),
                "Prepared playback should materialize every migrated sample identity after import.");
        require(importedPrepared.metrics.preparedZoneCount == migratedSession.getProject().authoring.zones.size(),
                "Prepared playback should materialize the imported migrated zone.");
        require(importedPrepared.metrics.cacheMissCount == migratedSession.getProject().sampleSources.size(),
                "First successful migrated prepared build should cold-miss every migrated sample handle.");
        require(importedPrepared.metrics.cacheHitCount == 0,
                "First successful migrated prepared build should not report cache hits.");
        require(importedPrepared.metrics.preparedSampleDataBytes
                    == computeExpectedPreparedSampleDataBytes(importedPrepared.prepared),
                "Imported migrated prepared playback should expose deterministic prepared sample-data bytes.");
        require(importedPrepared.metrics.decodedBytes == computeExpectedPreparedSampleDataBytes(importedPrepared.prepared),
                "First successful migrated prepared build should decode imported source samples through the preparation service.");
        require(importedPrepared.prepared.zones.size() == 1,
                "Imported migrated prepared playback should expose exactly one playable zone.");
        require(importedPrepared.prepared.zones[0].zoneId == importedZone.id,
                "Imported migrated prepared playback should preserve the imported zone identity.");

        auto editedImportedZone = *migratedSession.getSelectedZone();
        editedImportedZone.gainDb = 2.5;
        editedImportedZone.pan = -0.2;
        const auto editedZoneResult = migratedSession.updateSelectedZone(editedImportedZone,
                                                                         "Shape imported migrated zone");
        require(editedZoneResult.applied, "Editing the imported migrated zone should commit successfully.");
        require(editedZoneResult.documentState.revision == 2,
                "Editing the imported migrated zone should advance the draft revision.");

        const auto editedImportedSnapshotRequest = migratedSnapshotBuilder.requestBuild(editedZoneResult.documentState.revision,
                                                                                        true);
        const auto editedImportedSnapshot = migratedSnapshotBuilder.buildSnapshot(editedImportedSnapshotRequest,
                                                                                  migratedSession.getProject());
        require(editedImportedSnapshot.built,
                "Edited migrated authoring content should still build an immutable snapshot.");
        require(editedImportedSnapshot.snapshot.contentDigest != importedSnapshot.snapshot.contentDigest,
                "Editing imported migrated authoring content should change the immutable snapshot digest.");
        const auto editedImportedPreparedRequest = migratedPreparedService.requestBuild(editedImportedSnapshot);
        const auto editedImportedPrepared = migratedPreparedService.prepare(editedImportedPreparedRequest,
                                                                           editedImportedSnapshot,
                                                                           referenceStream);
        require(editedImportedPrepared.built,
                "Edited migrated authoring content should still prepare successfully.");
        require(editedImportedPrepared.prepared.preparedContentDigest != importedPrepared.prepared.preparedContentDigest,
                "Editing imported migrated authoring content should change the prepared-playback digest.");
        require(editedImportedPrepared.metrics.cacheHitCount == migratedSession.getProject().sampleSources.size(),
                "Editing zone-only migrated content should reuse every prepared sample handle.");
        require(editedImportedPrepared.metrics.cacheMissCount == 0,
                "Editing zone-only migrated content should not invalidate prepared sample handles.");
        require(editedImportedPrepared.metrics.preparedSampleDataBytes
                    == importedPrepared.metrics.preparedSampleDataBytes,
                "Editing zone-only migrated content should preserve deterministic prepared sample-data bytes.");
        require(editedImportedPrepared.metrics.decodedBytes == 0,
                "Editing zone-only migrated content should not re-decode warm prepared sample handles.");
        require(editedImportedPrepared.prepared.zones[0].gainDb == editedImportedZone.gainDb
                    && editedImportedPrepared.prepared.zones[0].pan == editedImportedZone.pan,
                "Prepared playback should preserve edited migrated zone normalization values.");

        const auto canceledPrepared = preparedService.cancelBuild(firstPreparedRequest);
        require(canceledPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::canceled,
                "Canceled prepared playback should report the canceled lifecycle state.");
        require(!canceledPrepared.activationEligible,
                "Canceled prepared playback result must never become activation-eligible.");
        require(canceledPrepared.metrics.cancellationCount == 1,
                "Canceled prepared playback result should increment the cancellation metric.");
        require(canceledPrepared.buildId == firstPreparedRequest.buildId
                    && canceledPrepared.cancellationId == firstPreparedRequest.cancellationId,
                "Canceled prepared playback result should preserve request and cancellation identities.");

        const auto supersedingPreparedRequest = preparedService.requestBuild(secondSnapshot, referenceStream);
        const auto supersededPrepared = preparedService.supersedeBuild(firstPreparedRequest,
                                                                       supersedingPreparedRequest.buildId);
        require(supersededPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::superseded,
                "Superseded prepared playback should report the superseded lifecycle state.");
        require(!supersededPrepared.activationEligible,
                "Superseded prepared playback result must never become activation-eligible.");
        require(supersededPrepared.cancellationId == supersedingPreparedRequest.buildId,
                "Superseded prepared playback result should point at the replacement build identity.");

        std::cout << "Phase 1 prepared playback tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 prepared playback tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
