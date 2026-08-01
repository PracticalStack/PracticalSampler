#pragma once

#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PerformancePublishController.h"
#include "drs/engine/PreparedPlayback.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace drs::engine
{
struct PerformancePublishPresentationSnapshot final
{
    std::uint64_t publicationSequence = 0;
    bool projectOpen = false;
    bool canPublish = false;
    bool dirty = false;
    bool hasRequestedPublish = false;
    bool hasActivePublished = false;
    bool hasLastKnownGood = false;
    bool hasFailure = false;
    PerformancePublishPresentationState state = PerformancePublishPresentationState::idle;
    std::string stateLabel;
    std::string guidance;
    std::string progressLabel;
    double progress = 0.0;

    std::size_t draftRevision = 0;
    std::string draftContentDigest;
    std::size_t previewRevision = 0;
    std::string previewContentDigest;
    std::size_t requestedPublishRevision = 0;
    std::string requestedPublishDigest;
    std::size_t activePublishedRevision = 0;
    std::string activePublishedDigest;
    std::size_t failedRevision = 0;
    std::string failedDigest;
    std::size_t lastKnownGoodRevision = 0;
    std::string lastKnownGoodDigest;
    std::string activeMacroSchemaDigest;

    std::string findingCode;
    std::string findingPath;
    std::string findingMessage;
    std::size_t exposedMacroCount = 0;
    std::size_t hiddenMacroCount = 0;
    std::size_t assignedMacroCount = 0;
    std::size_t unassignedMacroCount = 0;
    std::size_t availableHostSlotCount = 0;
    std::size_t activeHostSlotCount = 0;
    std::uint64_t preparedBuildId = 0;
    std::uint64_t activePayloadBytes = 0;
    std::size_t pendingWorkCount = 0;
    std::size_t inFlightWorkCount = 0;
    std::uint64_t lastPreparationMicros = 0;
    std::uint64_t lastRequestToReadyMicros = 0;
    std::uint64_t lastRequestToActiveMicros = 0;
};

PerformancePublishPresentationSnapshot buildPerformancePublishPresentationSnapshot(
    const DraftPlaybackStatus& draft,
    const PerformancePublishControllerSnapshot& controller,
    const PreparedPlaybackWorkerStatus& worker,
    std::uint64_t publicationSequence);

const char* toString(PerformancePublishPresentationState state) noexcept;
} // namespace drs::engine
