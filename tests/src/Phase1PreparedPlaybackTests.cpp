#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"

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

void writeAudioFile(const fs::path& filePath,
                    juce::AudioFormat& format,
                    const juce::AudioBuffer<float>& buffer,
                    double sampleRate)
{
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open prepared-playback fixture for writing: " + filePath.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(sampleRate)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    auto writerOwner = format.createWriterFor(output, options);
    require(writerOwner != nullptr,
            "Could not create prepared-playback audio writer for: " + filePath.generic_string());
    require(writerOwner->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write prepared-playback fixture audio: " + filePath.generic_string());
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
        sampleDataBytes += static_cast<std::uint64_t>(sample.channelCount)
            * sample.frameCount
            * static_cast<std::uint64_t>(sizeof(float));
    }

    return sampleDataBytes;
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
        require(firstPrepared.metrics.preparedOwnershipRecordCount == firstPrepared.prepared.ownershipRecords.size(),
                "Prepared playback metrics should expose ownership-record counts.");
        require(firstPrepared.metrics.preparedOwnershipBytes == firstPrepared.metrics.preparedBytes,
                "Prepared playback metrics should expose ownership-safe retained-byte totals.");
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
