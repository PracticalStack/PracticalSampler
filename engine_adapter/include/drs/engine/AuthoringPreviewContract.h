#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace drs::engine
{
enum class AuthoringPreviewScope : std::uint8_t
{
    selectedZone = 0,
    currentDraft
};

enum class AuthoringPreviewPreparationState : std::uint8_t
{
    idle = 0,
    queued,
    preparing,
    ready,
    failed,
    canceled,
    superseded
};

enum class AuthoringPreviewActivationState : std::uint8_t
{
    noActivation = 0,
    pending,
    active
};

enum class AuthoringPreviewPresentationState : std::uint8_t
{
    idle = 0,
    queued,
    preparing,
    ready,
    activating,
    active,
    stale,
    failed,
    canceled,
    superseded
};

enum class AuthoringPreviewAuditionSource : std::uint8_t
{
    summaryPreview = 0,
    authoringKeyboard,
    zoneMap,
    inspector
};

enum class AuthoringPreviewRequestReason : std::uint8_t
{
    projectOpened = 0,
    authoringChanged,
    selectionChanged,
    explicitSelectedZoneAudition,
    explicitCurrentDraftAudition,
    recovery
};

enum class AuthoringPreviewInvalidationCategory : std::uint8_t
{
    unknown = 0,
    mapping,
    gain,
    pan,
    rootKey,
    keyBounds,
    velocityRange,
    sampleStartOffset,
    loop,
    sourceAssignment,
    selection,
    previewScope,
    authoredTopology
};

enum class AuthoringPreviewNotePolicyEvent : std::uint8_t
{
    activationReplacement = 0,
    selectionChange,
    draftBecomesStale,
    previewStop,
    projectClose,
    deviceRestart
};

enum class AuthoringPreviewNotePolicyAction : std::uint8_t
{
    allowOldVoicesToFinish = 0,
    releasePreviewVoices,
    resetPreviewVoicesAndClearActivation,
    resetPreviewVoicesAndPreserveActivation
};

struct AuthoringPreviewRequestIdentity
{
    std::uint64_t requestId = 0;
    std::uint64_t cancellationGeneration = 0;
    std::size_t draftRevision = 0;
    AuthoringPreviewScope scope = AuthoringPreviewScope::selectedZone;
    std::string selectedZoneId;

    bool operator==(const AuthoringPreviewRequestIdentity& other) const noexcept
    {
        return requestId == other.requestId
            && cancellationGeneration == other.cancellationGeneration
            && draftRevision == other.draftRevision
            && scope == other.scope
            && selectedZoneId == other.selectedZoneId;
    }

    bool operator!=(const AuthoringPreviewRequestIdentity& other) const noexcept
    {
        return !(*this == other);
    }
};

struct AuthoringPreviewRequest
{
    AuthoringPreviewRequestIdentity identity;
    AuthoringPreviewRequestReason reason = AuthoringPreviewRequestReason::authoringChanged;
    AuthoringPreviewInvalidationCategory invalidationCategory
        = AuthoringPreviewInvalidationCategory::unknown;
    std::string requestSignature;
};

inline std::string buildAuthoringPreviewRequestSignature(
    AuthoringPreviewScope scope,
    const std::string& selectedZoneId,
    AuthoringPreviewInvalidationCategory invalidationCategory,
    const std::string& authoredContentFingerprint)
{
    return std::to_string(static_cast<unsigned int>(scope)) + "|"
        + selectedZoneId + "|"
        + std::to_string(static_cast<unsigned int>(invalidationCategory)) + "|"
        + authoredContentFingerprint;
}

constexpr bool authoringPreviewScopeIsEligible(AuthoringPreviewScope scope,
                                                bool hasSelectedZone) noexcept
{
    return scope == AuthoringPreviewScope::currentDraft || hasSelectedZone;
}

constexpr bool isAuthoringPreviewPreparationTransitionAllowed(
    AuthoringPreviewPreparationState from,
    AuthoringPreviewPreparationState to) noexcept
{
    using State = AuthoringPreviewPreparationState;
    switch (from)
    {
        case State::idle:
            return to == State::queued;
        case State::queued:
            return to == State::preparing || to == State::canceled || to == State::superseded;
        case State::preparing:
            return to == State::ready || to == State::failed
                || to == State::canceled || to == State::superseded;
        case State::ready:
            return to == State::idle || to == State::queued
                || to == State::failed || to == State::superseded;
        case State::failed:
        case State::canceled:
        case State::superseded:
            return to == State::idle || to == State::queued;
    }
    return false;
}

constexpr AuthoringPreviewNotePolicyAction authoringPreviewNotePolicyFor(
    AuthoringPreviewNotePolicyEvent event) noexcept
{
    using Event = AuthoringPreviewNotePolicyEvent;
    using Action = AuthoringPreviewNotePolicyAction;
    switch (event)
    {
        case Event::activationReplacement:
        case Event::selectionChange:
        case Event::draftBecomesStale:
            return Action::allowOldVoicesToFinish;
        case Event::previewStop:
            return Action::releasePreviewVoices;
        case Event::projectClose:
            return Action::resetPreviewVoicesAndClearActivation;
        case Event::deviceRestart:
            return Action::resetPreviewVoicesAndPreserveActivation;
    }
    return Action::allowOldVoicesToFinish;
}
} // namespace drs::engine
