#pragma once

#include "drs/engine/AuthoringPreviewContract.h"
#include "drs/engine/AuthoringPreviewRecovery.h"
#include "drs/engine/RuntimeModel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::app
{
struct AuthoringWaveformPreviewPoint
{
    float minValue = 0.0f;
    float maxValue = 0.0f;
};

enum class AuthoringWaveformPresentationState
{
    idle,
    loading,
    partial,
    ready,
    staleCompatible,
    missingSource,
    failed
};

struct AuthoringWaveformPreview
{
    bool available = false;
    AuthoringWaveformPresentationState presentationState
        = AuthoringWaveformPresentationState::idle;
    std::string state;
    std::string sourceIdentity;
    std::string sourcePath;
    std::string formatName;
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    bool loopEnabled = false;
    drs::engine::RegionLoopMode loopMode = drs::engine::RegionLoopMode::noLoop;
    std::uint64_t playbackStartFrame = 0;
    std::uint64_t playbackEndFrameExclusive = 0;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    std::uint64_t loopCrossfadeFrames = 0;
    double releaseSeconds = 0.0;
    std::uint64_t viewportStartFrame = 0;
    std::uint64_t viewportEndFrameExclusive = 0;
    std::uint64_t detailStartFrame = 0;
    std::uint64_t detailEndFrameExclusive = 0;
    std::vector<AuthoringWaveformPreviewPoint> detailPoints;
    bool selectionActive = false;
    std::uint64_t selectionStartFrame = 0;
    std::uint64_t selectionEndFrameExclusive = 0;
    bool playheadVisible = false;
    std::uint64_t playheadFrame = 0;
    std::string regionProvenance;
    bool detailCacheHit = false;
    std::size_t peakCacheEntryCount = 0;
    std::size_t peakCacheBytes = 0;
    std::uint64_t peakCacheEvictionCount = 0;
    std::vector<AuthoringWaveformPreviewPoint> points;
};

struct AuthoringPreviewStatusSnapshot
{
    bool available = false;
    drs::engine::AuthoringPreviewPresentationState presentationState
        = drs::engine::AuthoringPreviewPresentationState::idle;
    drs::engine::AuthoringPreviewPreparationState preparationState
        = drs::engine::AuthoringPreviewPreparationState::idle;
    drs::engine::AuthoringPreviewActivationState activationState
        = drs::engine::AuthoringPreviewActivationState::noActivation;
    drs::engine::AuthoringPreviewScope scope
        = drs::engine::AuthoringPreviewScope::selectedZone;
    std::uint64_t requestId = 0;
    std::uint64_t cancellationGeneration = 0;
    std::size_t draftRevision = 0;
    std::size_t activeRevision = 0;
    std::size_t pendingRevision = 0;
    std::size_t requestedRevision = 0;
    std::size_t failedRevision = 0;
    std::size_t audibleRevision = 0;
    bool auditionAvailable = false;
    bool stopAvailable = false;
    bool usingLastKnownGood = false;
    std::string selectedZoneId;
    std::uint64_t requestedPreparedBuildId = 0;
    std::uint64_t activePreparedBuildId = 0;
    std::string requestedSnapshotDigest;
    std::string requestedPreparedDigest;
    std::string activeSnapshotDigest;
    std::string activePreparedDigest;
    std::vector<drs::engine::AuthoringPreviewFailureFinding> findings;
    std::string stateLabel;
    std::string creatorGuidance;
    std::string failureState;
    std::string failureFamily;
    std::string failureCode;
    std::string failurePath;
    std::string blockingPrerequisite;
    std::string blockingGuidance;
    std::uint64_t lastRequestToLaunchMicros = 0;
    std::uint64_t maxRequestToLaunchMicros = 0;
    std::uint64_t lastPreparationMicros = 0;
    std::uint64_t maxPreparationMicros = 0;
    std::uint64_t lastReadyToActivationMicros = 0;
    std::uint64_t maxReadyToActivationMicros = 0;
    std::uint64_t lastRequestToAudibleMicros = 0;
    std::uint64_t maxRequestToAudibleMicros = 0;
    std::uint64_t lastCancellationMicros = 0;
    std::uint64_t maxCancellationMicros = 0;
    std::size_t coalescedCount = 0;
    std::size_t canceledCount = 0;
    std::size_t pendingDepth = 0;
    std::size_t maximumPendingDepth = 0;
};

struct AuthoringImportResponsivenessSnapshot
{
    bool available = false;
    std::string state;
    std::size_t totalItemCount = 0;
    std::size_t pendingCount = 0;
    std::size_t processedCount = 0;
    std::size_t warningItemCount = 0;
    std::size_t failedItemCount = 0;
    std::size_t canceledItemCount = 0;
    std::size_t acceptedItemCount = 0;
    std::uint64_t lastProcessDurationMicros = 0;
    std::uint64_t averageProcessDurationMicros = 0;
    std::uint64_t maxProcessDurationMicros = 0;
    std::string lastProcessedItemId;
};

struct AuthoringSourceValidationSnapshot
{
    bool available = false;
    std::string state;
    std::size_t totalItemCount = 0;
    std::size_t processedCount = 0;
    std::size_t warningItemCount = 0;
    std::size_t failedItemCount = 0;
    std::size_t canceledItemCount = 0;
    std::uint64_t totalBytesProcessed = 0;
    std::uint64_t totalBytesExpected = 0;
    std::uint64_t totalDurationMicros = 0;
    std::string currentSourceId;
    std::string currentSourcePath;
};
} // namespace drs::app
