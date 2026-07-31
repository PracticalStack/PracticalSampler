#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace drs::engine
{
enum class SfzImportCommandType : std::uint8_t
{
    analyzeDocument = 0,
    commitReviewedImport
};

struct SfzImportCommand
{
    SfzImportCommandType type = SfzImportCommandType::analyzeDocument;
};

enum class SfzImportStage : std::uint8_t
{
    idle = 0,
    discovering,
    parsing,
    normalizing,
    validating,
    classifying,
    projected,
    reviewReady,
    blocked,
    committed,
    canceled
};

// Engine work is intentionally expressed in terms of a small, typed outcome
// rather than making callers infer cancellation or failure from display text.
// The values are shared by parsing, normalization, reporting, and projection
// so a service can carry one disposition across the entire pipeline.
enum class SfzImportExecutionDisposition : std::uint8_t
{
    none = 0,
    completed,
    canceled,
    failed
};

enum class SfzImportCancellationReason : std::uint8_t
{
    none = 0,
    requested,
    shutdown,
    superseded,
    probeException
};

enum class SfzImportFailureReason : std::uint8_t
{
    none = 0,
    sourceMissing,
    malformedInput,
    unsupportedInput,
    internalError,
    budgetExceeded
};

struct SfzImportBudgetLimits
{
    // These defaults admit the checked-in real-world corpus while bounding
    // hostile recursive or diagnostic-heavy documents well below process-
    // threatening sizes.
    std::size_t maximumTotalSourceBytes = 16u * 1024u * 1024u;
    std::size_t maximumIncludeCount = 256;
    std::size_t maximumIncludeDepth = 32;
    std::size_t maximumSectionCount = 100000;
    std::size_t maximumRegionCount = 65536;
    std::size_t maximumFindingCount = 512;
};

struct SfzImportExecutionState
{
    SfzImportExecutionDisposition disposition = SfzImportExecutionDisposition::none;
    SfzImportCancellationReason cancellationReason = SfzImportCancellationReason::none;
    SfzImportFailureReason failureReason = SfzImportFailureReason::none;

    bool completed() const noexcept
    {
        return disposition == SfzImportExecutionDisposition::completed;
    }

    bool canceled() const noexcept
    {
        return disposition == SfzImportExecutionDisposition::canceled;
    }

    bool failed() const noexcept
    {
        return disposition == SfzImportExecutionDisposition::failed;
    }
};

struct SfzImportProgress
{
    SfzImportStage stage = SfzImportStage::idle;
    float progress01 = 0.0f;
};

// The callbacks are optional and are deliberately value types so engine code
// has no dependency on JUCE or a shell-owned object. The first sink form is
// convenient for lightweight callers; the event sink is available when the
// caller wants one typed payload. Exceptions from cancellation probes become
// typed probeException cancellation; progress observers are advisory.
using SfzImportCancellationProbe = std::function<bool()>;
using SfzImportCancellationReasonProbe = std::function<SfzImportCancellationReason()>;
using SfzImportProgressSink = std::function<void(SfzImportStage, float)>;
using SfzImportProgressEventSink = std::function<void(const SfzImportProgress&)>;

struct SfzImportExecutionContext
{
    SfzImportCancellationProbe cancellationProbe;
    SfzImportProgressSink progressSink;
    SfzImportCancellationReasonProbe cancellationReasonProbe;
    SfzImportProgressEventSink progressEventSink;
    SfzImportBudgetLimits budgets;

    // Polling is intentionally noexcept. A throwing callback is unsafe on a
    // parser worker, so it becomes a typed cancellation instead.
    SfzImportCancellationReason pollCancellation() const noexcept
    {
        if (stickyCancellationReason != SfzImportCancellationReason::none)
            return stickyCancellationReason;

        try
        {
            if (cancellationReasonProbe)
            {
                const auto reason = cancellationReasonProbe();
                if (reason != SfzImportCancellationReason::none)
                {
                    stickyCancellationReason = reason;
                    return reason;
                }
            }

            if (cancellationProbe && cancellationProbe())
            {
                stickyCancellationReason = SfzImportCancellationReason::requested;
                return SfzImportCancellationReason::requested;
            }
        }
        catch (...)
        {
            stickyCancellationReason = SfzImportCancellationReason::probeException;
            return SfzImportCancellationReason::probeException;
        }

        return SfzImportCancellationReason::none;
    }

    bool isCancellationRequested() const noexcept
    {
        return pollCancellation() != SfzImportCancellationReason::none;
    }

    void resetProgress() const noexcept
    {
        lastProgress01 = 0.0f;
        stickyCancellationReason = SfzImportCancellationReason::none;
    }

    // Progress is best effort and monotonic for one context. The sink receives
    // a clamped value, never a value outside [0, 1].
    void reportProgress(const SfzImportStage stage, const float progress01) const noexcept
    {
        const auto clamped = progress01 < 0.0f ? 0.0f : (progress01 > 1.0f ? 1.0f : progress01);
        if (clamped < lastProgress01)
            return;

        lastProgress01 = clamped;
        try
        {
            if (progressSink)
                progressSink(stage, clamped);
            if (progressEventSink)
                progressEventSink({ stage, clamped });
        }
        catch (...)
        {
            // Progress observers are advisory and must never take down an
            // engine operation. Cancellation remains the only worker control
            // path and is polled independently.
        }
    }

    // Mutable because callbacks are const-facing and contexts are commonly
    // shared by the parser/report/projector stages.
    mutable float lastProgress01 = 0.0f;
    mutable SfzImportCancellationReason stickyCancellationReason = SfzImportCancellationReason::none;
};

inline const SfzImportExecutionContext& defaultSfzImportExecutionContext() noexcept
{
    static const SfzImportExecutionContext context;
    return context;
}

enum class SfzOpcodeScope : std::uint8_t
{
    unknown = 0,
    control,
    global,
    master,
    group,
    region,
    curve,
    effect,
    midi,
    sample
};

enum class SfzImportFindingSeverity : std::uint8_t
{
    information = 0,
    warning,
    error
};

enum class SfzImportSupportDisposition : std::uint8_t
{
    converted = 0,
    approximated,
    reportedOnly,
    blocking
};

enum class SfzImportReviewDisposition : std::uint8_t
{
    noneRequired = 0,
    confirmationRequired,
    blocked
};

struct SfzImportSourceLocation
{
    std::string sourcePath;
    std::size_t lineNumber = 0;
    std::size_t columnNumber = 0;
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string opcode;
};

struct SfzImportFinding
{
    SfzImportFindingSeverity severity = SfzImportFindingSeverity::information;
    SfzImportSupportDisposition disposition = SfzImportSupportDisposition::converted;
    std::string code;
    std::string summary;
    std::string detail;
    SfzImportSourceLocation location;
};

struct SfzFirstFixtureCharacterization
{
    static constexpr std::size_t expectedControlHeaderCount = 1;
    static constexpr std::size_t expectedMasterHeaderCount = 1;
    static constexpr std::size_t expectedGroupHeaderCount = 5;
    static constexpr std::size_t expectedCurveHeaderCount = 1;
    static constexpr std::size_t expectedRegionCount = 225;
    static constexpr std::size_t expectedRoundRobinDepth = 3;
    static constexpr std::size_t expectedVelocityLayerCount = 5;
};

constexpr bool sfzImportCommandRequiresReviewedReport(SfzImportCommandType type) noexcept
{
    return type == SfzImportCommandType::commitReviewedImport;
}

constexpr bool isSfzImportStageTransitionAllowed(SfzImportStage from, SfzImportStage to) noexcept
{
    using Stage = SfzImportStage;
    switch (from)
    {
        case Stage::idle:
            return to == Stage::discovering;
        case Stage::discovering:
            return to == Stage::parsing || to == Stage::blocked || to == Stage::canceled;
        case Stage::parsing:
            return to == Stage::normalizing || to == Stage::blocked || to == Stage::canceled;
        case Stage::normalizing:
            return to == Stage::validating || to == Stage::blocked || to == Stage::canceled;
        case Stage::validating:
            return to == Stage::classifying || to == Stage::blocked || to == Stage::canceled;
        case Stage::classifying:
            return to == Stage::projected || to == Stage::blocked || to == Stage::canceled;
        case Stage::projected:
            return to == Stage::reviewReady || to == Stage::blocked || to == Stage::canceled;
        case Stage::reviewReady:
            return to == Stage::committed || to == Stage::canceled || to == Stage::discovering;
        case Stage::blocked:
        case Stage::committed:
        case Stage::canceled:
            return to == Stage::idle || to == Stage::discovering;
    }
    return false;
}

inline SfzImportReviewDisposition sfzImportReviewDispositionFor(
    const std::vector<SfzImportFinding>& findings) noexcept
{
    auto requiresConfirmation = false;
    for (const auto& finding : findings)
    {
        if (finding.severity == SfzImportFindingSeverity::error
            || finding.disposition == SfzImportSupportDisposition::blocking)
            return SfzImportReviewDisposition::blocked;

        if (finding.disposition == SfzImportSupportDisposition::approximated
            || finding.disposition == SfzImportSupportDisposition::reportedOnly
            || finding.severity == SfzImportFindingSeverity::warning)
            requiresConfirmation = true;
    }

    return requiresConfirmation
        ? SfzImportReviewDisposition::confirmationRequired
        : SfzImportReviewDisposition::noneRequired;
}
} // namespace drs::engine
