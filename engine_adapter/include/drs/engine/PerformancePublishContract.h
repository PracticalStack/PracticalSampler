#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
enum class PerformancePublishCommandType : std::uint8_t
{
    publishCurrentDraft = 0
};

struct PerformancePublishCommand
{
    PerformancePublishCommandType type = PerformancePublishCommandType::publishCurrentDraft;
};

enum class PerformancePublishPreparationState : std::uint8_t
{
    idle = 0,
    queued,
    preparing,
    ready,
    failed,
    canceled,
    superseded
};

enum class PerformancePublishActivationState : std::uint8_t
{
    noActivation = 0,
    pending,
    active
};

enum class PerformancePublishPresentationState : std::uint8_t
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

enum class PerformancePublishFindingSeverity : std::uint8_t
{
    information = 0,
    warning,
    error
};

struct PerformancePublishFinding
{
    PerformancePublishFindingSeverity severity = PerformancePublishFindingSeverity::information;
    std::string code;
    std::string path;
    std::string message;
};

struct PerformancePublishRequestIdentity
{
    std::uint64_t requestId = 0;
    std::uint64_t cancellationGeneration = 0;
    std::uint64_t projectGeneration = 0;
    std::size_t draftRevision = 0;
    std::string authoredContentDigest;
    std::string macroSchemaDigest;

    bool operator==(const PerformancePublishRequestIdentity& other) const noexcept
    {
        return requestId == other.requestId
            && cancellationGeneration == other.cancellationGeneration
            && projectGeneration == other.projectGeneration
            && draftRevision == other.draftRevision
            && authoredContentDigest == other.authoredContentDigest
            && macroSchemaDigest == other.macroSchemaDigest;
    }

    bool operator!=(const PerformancePublishRequestIdentity& other) const noexcept
    {
        return !(*this == other);
    }
};

struct PerformancePublishRequest
{
    PerformancePublishRequestIdentity identity;
};

struct PerformancePublishResult
{
    PerformancePublishRequestIdentity identity;
    bool completeProject = false;
    bool activationEligible = false;
    std::uint64_t preparedBuildId = 0;
    std::string preparedContentDigest;
    std::string preparedMacroSchemaDigest;
    std::vector<PerformancePublishFinding> findings;
};

enum class PerformancePublishCompletionDisposition : std::uint8_t
{
    preserveLastKnownGood = 0,
    stageEligiblePayload
};

enum class PerformancePublishVoicePolicyEvent : std::uint8_t
{
    activationReplacement = 0,
    existingVoiceNoteOff,
    retriggerAfterActivation,
    performanceStopAll
};

enum class PerformancePublishVoicePolicyAction : std::uint8_t
{
    retainOriginalGeneration = 0,
    routeToOwningGeneration,
    useActiveGeneration,
    releaseAllPerformanceGenerations
};

enum class PerformancePublishMacroMigrationCase : std::uint8_t
{
    compatibleStableId = 0,
    addedStableId,
    removedStableId,
    changedRange,
    reorderedStableId
};

enum class PerformancePublishMacroMigrationAction : std::uint8_t
{
    preserveAndClampByStableId = 0,
    useAuthoredDefault,
    retireBinding,
    preserveByStableId
};

constexpr bool isPerformancePublishPreparationTransitionAllowed(
    PerformancePublishPreparationState from,
    PerformancePublishPreparationState to) noexcept
{
    using State = PerformancePublishPreparationState;
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
        case State::failed:
        case State::canceled:
        case State::superseded:
            return to == State::idle || to == State::queued;
    }
    return false;
}

inline bool performancePublishResultIsEligible(
    const PerformancePublishRequestIdentity& current,
    const PerformancePublishResult& result) noexcept
{
    return result.identity == current
        && result.completeProject
        && result.activationEligible
        && result.preparedBuildId != 0
        && !result.preparedContentDigest.empty()
        && !result.preparedMacroSchemaDigest.empty()
        && result.preparedMacroSchemaDigest == current.macroSchemaDigest;
}

inline PerformancePublishCompletionDisposition performancePublishCompletionDisposition(
    const PerformancePublishRequestIdentity& current,
    const PerformancePublishResult& result) noexcept
{
    return performancePublishResultIsEligible(current, result)
        ? PerformancePublishCompletionDisposition::stageEligiblePayload
        : PerformancePublishCompletionDisposition::preserveLastKnownGood;
}

constexpr PerformancePublishVoicePolicyAction performancePublishVoicePolicyFor(
    PerformancePublishVoicePolicyEvent event) noexcept
{
    using Event = PerformancePublishVoicePolicyEvent;
    using Action = PerformancePublishVoicePolicyAction;
    switch (event)
    {
        case Event::activationReplacement:
            return Action::retainOriginalGeneration;
        case Event::existingVoiceNoteOff:
            return Action::routeToOwningGeneration;
        case Event::retriggerAfterActivation:
            return Action::useActiveGeneration;
        case Event::performanceStopAll:
            return Action::releaseAllPerformanceGenerations;
    }
    return Action::retainOriginalGeneration;
}

constexpr PerformancePublishMacroMigrationAction performancePublishMacroMigrationFor(
    PerformancePublishMacroMigrationCase migrationCase) noexcept
{
    using Case = PerformancePublishMacroMigrationCase;
    using Action = PerformancePublishMacroMigrationAction;
    switch (migrationCase)
    {
        case Case::compatibleStableId:
        case Case::changedRange:
            return Action::preserveAndClampByStableId;
        case Case::addedStableId:
            return Action::useAuthoredDefault;
        case Case::removedStableId:
            return Action::retireBinding;
        case Case::reorderedStableId:
            return Action::preserveByStableId;
    }
    return Action::preserveByStableId;
}
} // namespace drs::engine
