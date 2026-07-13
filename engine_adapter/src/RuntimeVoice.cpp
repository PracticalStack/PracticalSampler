#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimeVoice.h"

#include "drs/engine/RuntimeStream.h"

#include <algorithm>

namespace drs::engine
{
namespace
{
const RuntimeZoneDefinition* findZone(const RuntimeInstrumentModel& instrument, const std::string& zoneId)
{
    const auto iterator = std::find_if(instrument.zones.begin(),
                                       instrument.zones.end(),
                                       [&](const RuntimeZoneDefinition& zone)
                                       {
                                           return zone.id == zoneId;
                                       });

    return iterator != instrument.zones.end() ? &(*iterator) : nullptr;
}

const RuntimeArticulationDefinition* findArticulation(const RuntimeInstrumentModel& instrument,
                                                      const std::string& articulationId)
{
    const auto iterator = std::find_if(instrument.articulations.begin(),
                                       instrument.articulations.end(),
                                       [&](const RuntimeArticulationDefinition& articulation)
                                       {
                                           return articulation.id == articulationId;
                                       });

    return iterator != instrument.articulations.end() ? &(*iterator) : nullptr;
}

const RuntimeArticulationDefinition* findDefaultArticulation(const RuntimeInstrumentModel& instrument)
{
    const auto iterator = std::find_if(instrument.articulations.begin(),
                                       instrument.articulations.end(),
                                       [](const RuntimeArticulationDefinition& articulation)
                                       {
                                           return articulation.isDefault;
                                       });

    return iterator != instrument.articulations.end() ? &(*iterator) : nullptr;
}

const RuntimeGroupDefinition* findGroup(const RuntimeInstrumentModel& instrument, const std::string& groupId)
{
    const auto iterator = std::find_if(instrument.groups.begin(),
                                       instrument.groups.end(),
                                       [&](const RuntimeGroupDefinition& group)
                                       {
                                           return group.id == groupId;
                                       });

    return iterator != instrument.groups.end() ? &(*iterator) : nullptr;
}

bool hasGroup(const RuntimeInstrumentModel& instrument, const std::string& groupId)
{
    return findGroup(instrument, groupId) != nullptr;
}

bool hasArticulation(const RuntimeInstrumentModel& instrument, const std::string& articulationId)
{
    return findArticulation(instrument, articulationId) != nullptr;
}

const RuntimeStreamSampleDefinition* findStreamSample(const RuntimeStreamContainerModel& streamContainer,
                                                      const RuntimeZoneDefinition& zone)
{
    const auto iterator = std::find_if(streamContainer.samples.begin(),
                                       streamContainer.samples.end(),
                                       [&](const RuntimeStreamSampleDefinition& sample)
                                       {
                                           return sample.sourcePath == zone.samplePath
                                               || sample.payloadOffsetBytes == zone.streamOffsetBytes;
                                       });

    return iterator != streamContainer.samples.end() ? &(*iterator) : nullptr;
}

bool groupSupportsArticulation(const RuntimeGroupDefinition& group, const std::string& articulationId)
{
    return std::find(group.articulationIds.begin(), group.articulationIds.end(), articulationId)
        != group.articulationIds.end();
}

bool zoneMatchesTrigger(const RuntimeZoneDefinition& zone, int midiNote, int velocity)
{
    return midiNote >= zone.keyLow
        && midiNote <= zone.keyHigh
        && velocity >= zone.velocityLow
        && velocity <= zone.velocityHigh;
}

int absoluteDifference(int left, int right)
{
    return left > right ? (left - right) : (right - left);
}

bool isBetterZoneCandidate(const RuntimeZoneDefinition& candidate,
                           const RuntimeZoneDefinition& currentBest,
                           int midiNote)
{
    const auto candidateRootDistance = absoluteDifference(candidate.rootKey, midiNote);
    const auto currentRootDistance = absoluteDifference(currentBest.rootKey, midiNote);
    if (candidateRootDistance != currentRootDistance)
        return candidateRootDistance < currentRootDistance;

    const auto candidateKeySpan = candidate.keyHigh - candidate.keyLow;
    const auto currentKeySpan = currentBest.keyHigh - currentBest.keyLow;
    if (candidateKeySpan != currentKeySpan)
        return candidateKeySpan < currentKeySpan;

    const auto candidateVelocitySpan = candidate.velocityHigh - candidate.velocityLow;
    const auto currentVelocitySpan = currentBest.velocityHigh - currentBest.velocityLow;
    if (candidateVelocitySpan != currentVelocitySpan)
        return candidateVelocitySpan < currentVelocitySpan;

    return candidate.id < currentBest.id;
}

std::uint64_t floorDivide(std::uint64_t numerator, std::uint64_t denominator)
{
    return denominator == 0 ? 0 : numerator / denominator;
}
} // namespace

RuntimeVoiceRouteResolution resolveRuntimeVoiceRoute(const RuntimeInstrumentModel& instrument,
                                                     const RuntimeStreamContainerModel& streamContainer,
                                                     const RuntimeVoiceAllocationRequest& request)
{
    RuntimeVoiceRouteResolution result;
    result.state = "Voice route unresolved.";

    if (request.midiNote < 0 || request.midiNote > 127)
    {
        result.state = "Voice route requested MIDI note outside the supported 0-127 range.";
        return result;
    }

    if (request.velocity < 1 || request.velocity > 127)
    {
        result.state = "Voice route requested velocity outside the supported 1-127 range.";
        return result;
    }

    const RuntimeZoneDefinition* selectedZone = nullptr;
    if (!request.zoneId.empty())
    {
        selectedZone = findZone(instrument, request.zoneId);
        if (selectedZone == nullptr)
        {
            result.state = "Voice allocation requested unknown zone '" + request.zoneId + "'.";
            return result;
        }

        if (!request.articulationId.empty() && request.articulationId != selectedZone->articulationId)
        {
            result.state = "Voice allocation requested zone '" + request.zoneId
                + "' with mismatched articulation '" + request.articulationId + "'.";
            return result;
        }
    }
    else
    {
        std::string targetArticulationId = request.articulationId;
        if (targetArticulationId.empty())
        {
            const auto* defaultArticulation = findDefaultArticulation(instrument);
            if (defaultArticulation == nullptr)
            {
                result.state = "Voice route could not resolve a default articulation for trigger-based allocation.";
                return result;
            }

            targetArticulationId = defaultArticulation->id;
            result.usedDefaultArticulation = true;
        }
        else if (!hasArticulation(instrument, targetArticulationId))
        {
            result.state = "Voice route requested unknown articulation '" + targetArticulationId + "'.";
            return result;
        }

        for (const auto& zone : instrument.zones)
        {
            if (zone.articulationId != targetArticulationId || !zoneMatchesTrigger(zone, request.midiNote, request.velocity))
                continue;

            if (selectedZone == nullptr || isBetterZoneCandidate(zone, *selectedZone, request.midiNote))
                selectedZone = &zone;
        }

        if (selectedZone == nullptr)
        {
            result.state = "Voice route could not find a matching zone for articulation '" + targetArticulationId
                + "', note " + std::to_string(request.midiNote)
                + ", velocity " + std::to_string(request.velocity) + ".";
            return result;
        }
    }

    const auto* group = findGroup(instrument, selectedZone->groupId);
    if (group == nullptr)
    {
        result.state = "Voice allocation requested zone '" + selectedZone->id
            + "' with unknown group '" + selectedZone->groupId + "'.";
        return result;
    }

    if (!groupSupportsArticulation(*group, selectedZone->articulationId))
    {
        result.state = "Voice allocation requested zone '" + selectedZone->id
            + "' whose group '" + selectedZone->groupId
            + "' does not expose articulation '" + selectedZone->articulationId + "'.";
        return result;
    }

    if (!hasArticulation(instrument, selectedZone->articulationId))
    {
        result.state = "Voice allocation requested zone '" + selectedZone->id
            + "' with unknown articulation '" + selectedZone->articulationId + "'.";
        return result;
    }

    const auto* sample = findStreamSample(streamContainer, *selectedZone);
    if (sample == nullptr)
    {
        result.state = "Voice allocation could not map zone '" + selectedZone->id + "' to a stream-container sample.";
        return result;
    }

    result.resolved = true;
    result.state = request.zoneId.empty()
        ? "Voice route resolved from note trigger."
        : "Voice route resolved from explicit zone.";
    result.zoneId = selectedZone->id;
    result.groupId = selectedZone->groupId;
    result.articulationId = selectedZone->articulationId;
    result.sampleId = sample->sampleId;
    return result;
}

bool RuntimeVoice::allocate(const RuntimeInstrumentModel& instrument,
                            const RuntimeStreamContainerModel& streamContainerIn,
                            const RuntimeVoiceAllocationRequest& request,
                            std::string& errorMessage)
{
    if (cursor.currentLeaseId != 0)
    {
        errorMessage = "Voice cannot allocate a new zone while a stream-page lease is still held.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    const auto route = resolveRuntimeVoiceRoute(instrument, streamContainerIn, request);
    if (!route.resolved)
    {
        errorMessage = route.state;
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    const auto* zone = findZone(instrument, route.zoneId);
    if (zone == nullptr)
    {
        errorMessage = "Voice allocation resolved zone '" + route.zoneId + "' but could not look it up again.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    if (!hasGroup(instrument, zone->groupId))
    {
        errorMessage = "Voice allocation requested zone '" + request.zoneId + "' with unknown group '" + zone->groupId + "'.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    if (!hasArticulation(instrument, zone->articulationId))
    {
        errorMessage = "Voice allocation requested zone '" + request.zoneId + "' with unknown articulation '" + zone->articulationId + "'.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    const auto* sample = findStreamSample(streamContainerIn, *zone);
    if (sample == nullptr)
    {
        errorMessage = "Voice allocation could not map zone '" + request.zoneId + "' to a stream-container sample.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    const auto bytesPerFrame = static_cast<std::uint64_t>(sample->channelCount) * sizeof(float);
    if (bytesPerFrame == 0)
    {
        errorMessage = "Voice allocation found a stream-container sample with zero bytes per frame.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    const auto loadProfile = findPhase1RuntimeLoadProfile(instrument.defaultLoadProfile);
    if (!loadProfile.has_value())
    {
        errorMessage = "Voice allocation requested unknown load profile '" + instrument.defaultLoadProfile + "'.";
        state = RuntimeVoiceLifecycleState::failed;
        return false;
    }

    clearLocalState();
    streamContainer = &streamContainerIn;
    streamSample = sample;

    state = RuntimeVoiceLifecycleState::active;
    voiceId = request.voiceId;
    midiNote = request.midiNote;
    velocity = request.velocity;
    zoneId = zone->id;
    groupId = zone->groupId;
    articulationId = zone->articulationId;
    sampleId = sample->sampleId;
    releaseRequested = false;
    macroValues = request.macroValues;

    cursor.sampleId = sample->sampleId;
    cursor.totalFrameCount = sample->frameCount;
    cursor.currentFrameIndex = 0;
    cursor.bytesPerFrame = bytesPerFrame;
    cursor.payloadRelativeOffsetBytes = 0;
    cursor.prefetchBytes = clampPrefetchBytesForLoadProfile(*loadProfile,
                                                            zone->prefetchBytes,
                                                            sample->prefetchBytes);
    cursor.currentPageIndex = -1;
    cursor.currentLeaseId = 0;

    errorMessage.clear();
    return true;
}

RuntimeVoiceAdvanceResult RuntimeVoice::advanceFrames(std::uint64_t framesToAdvance,
                                                      RuntimeStreamingService& streamingService)
{
    RuntimeVoiceAdvanceResult result;
    result.state = state;
    result.currentPageIndex = cursor.currentPageIndex;

    if (state == RuntimeVoiceLifecycleState::idle
        || state == RuntimeVoiceLifecycleState::finished
        || state == RuntimeVoiceLifecycleState::failed
        || streamContainer == nullptr
        || streamSample == nullptr)
    {
        return result;
    }

    auto framesRemaining = framesToAdvance;
    if (!registeredWithStreamingService && voiceId != 0)
    {
        streamingService.registerActiveVoice(voiceId);
        registeredWithStreamingService = true;
    }

    while (framesRemaining > 0)
    {
        if (cursor.currentFrameIndex >= cursor.totalFrameCount
            || cursor.payloadRelativeOffsetBytes >= streamSample->payloadSizeBytes)
        {
            result.reachedSampleEnd = true;
            releaseCurrentLease(streamingService, &result);
            unregisterFromStreamingService(streamingService);
            state = RuntimeVoiceLifecycleState::finished;
            result.voiceFinished = true;
            break;
        }

        const auto readResult = resolveRuntimeStreamRead(*streamContainer,
                                                         cursor.sampleId,
                                                         cursor.payloadRelativeOffsetBytes);

        if (!readResult.resolved)
        {
            unregisterFromStreamingService(streamingService);
            state = RuntimeVoiceLifecycleState::failed;
            result.state = state;
            break;
        }

        if (readResult.inPageTable)
        {
            const RuntimeStreamPageRequest request { cursor.sampleId, readResult.pageIndex };

            if (cursor.currentPageIndex != static_cast<std::int32_t>(readResult.pageIndex))
                releaseCurrentLease(streamingService, &result);

            if (cursor.currentLeaseId == 0)
            {
                const auto lease = streamingService.tryAcquirePageLease(request);
                if (!lease.has_value())
                {
                    streamingService.recordPageMiss(request);
                    const auto enqueueResult = streamingService.enqueuePageRead(request);
                    result.queuedPageRead = result.queuedPageRead || enqueueResult.accepted;
                    state = releaseRequested
                        ? RuntimeVoiceLifecycleState::releasing
                        : RuntimeVoiceLifecycleState::waitingForPage;
                    result.waitingForPage = true;
                    result.state = state;
                    result.currentPageIndex = static_cast<std::int32_t>(readResult.pageIndex);
                    break;
                }

                cursor.currentLeaseId = lease->leaseId;
                cursor.currentPageIndex = static_cast<std::int32_t>(lease->pageIndex);
                result.acquiredPageLease = true;
            }
        }
        else
        {
            releaseCurrentLease(streamingService, &result);
            cursor.currentPageIndex = -1;
        }

        const auto readableFrames = floorDivide(readResult.readableBytes, cursor.bytesPerFrame);
        if (readableFrames == 0)
        {
            state = RuntimeVoiceLifecycleState::failed;
            result.state = state;
            break;
        }

        const auto framesThisStep = std::min(framesRemaining, readableFrames);
        cursor.currentFrameIndex += framesThisStep;
        cursor.payloadRelativeOffsetBytes += framesThisStep * cursor.bytesPerFrame;
        framesRemaining -= framesThisStep;

        if (readResult.inPrefetchHead)
            streamingService.recordHeadUsage(framesThisStep, framesThisStep * cursor.bytesPerFrame);

        result.advanced = result.advanced || (framesThisStep > 0);
        result.framesAdvanced += framesThisStep;

        if (cursor.currentFrameIndex >= cursor.totalFrameCount
            || cursor.payloadRelativeOffsetBytes >= streamSample->payloadSizeBytes)
        {
            result.reachedSampleEnd = true;
            releaseCurrentLease(streamingService, &result);
            unregisterFromStreamingService(streamingService);
            state = RuntimeVoiceLifecycleState::finished;
            result.voiceFinished = true;
            break;
        }

        if (readResult.inPageTable)
        {
            const auto pageRelativeOffset = readResult.absoluteOffsetBytes - streamSample->payloadOffsetBytes;
            const auto pageEndRelativeOffset = pageRelativeOffset + readResult.readableBytes;
            if (cursor.payloadRelativeOffsetBytes >= pageEndRelativeOffset)
                releaseCurrentLease(streamingService, &result);
        }
    }

    if (state != RuntimeVoiceLifecycleState::finished
        && state != RuntimeVoiceLifecycleState::failed
        && !result.waitingForPage)
    {
        state = releaseRequested
            ? RuntimeVoiceLifecycleState::releasing
            : RuntimeVoiceLifecycleState::active;
    }

    result.state = state;
    result.currentPageIndex = cursor.currentPageIndex;
    return result;
}

void RuntimeVoice::beginRelease()
{
    releaseRequested = true;
    if (state == RuntimeVoiceLifecycleState::active)
        state = RuntimeVoiceLifecycleState::releasing;
}

void RuntimeVoice::reset(RuntimeStreamingService& streamingService)
{
    RuntimeVoiceAdvanceResult ignored;
    releaseCurrentLease(streamingService, &ignored);
    unregisterFromStreamingService(streamingService);
    clearLocalState();
}

RuntimeVoiceSnapshot RuntimeVoice::getSnapshot() const
{
    return {
        state,
        voiceId,
        midiNote,
        velocity,
        zoneId,
        groupId,
        articulationId,
        sampleId,
        releaseRequested,
        cursor,
        macroValues
    };
}

void RuntimeVoice::clearLocalState()
{
    state = RuntimeVoiceLifecycleState::idle;
    voiceId = 0;
    midiNote = 0;
    velocity = 0;
    zoneId.clear();
    groupId.clear();
    articulationId.clear();
    sampleId.clear();
    releaseRequested = false;
    cursor = {};
    macroValues.clear();
    streamContainer = nullptr;
    streamSample = nullptr;
    registeredWithStreamingService = false;
}

void RuntimeVoice::releaseCurrentLease(RuntimeStreamingService& streamingService,
                                       RuntimeVoiceAdvanceResult* result)
{
    if (cursor.currentLeaseId == 0)
        return;

    streamingService.releasePageLease(cursor.currentLeaseId);
    cursor.currentLeaseId = 0;
    cursor.currentPageIndex = -1;

    if (result != nullptr)
    {
        result->releasedPageLease = true;
        result->currentPageIndex = cursor.currentPageIndex;
    }
}

void RuntimeVoice::unregisterFromStreamingService(RuntimeStreamingService& streamingService)
{
    if (!registeredWithStreamingService)
        return;

    streamingService.unregisterActiveVoice(voiceId);
    registeredWithStreamingService = false;
}
} // namespace drs::engine
