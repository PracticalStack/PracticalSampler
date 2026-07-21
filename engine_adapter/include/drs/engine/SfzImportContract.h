#pragma once

#include <cstddef>
#include <cstdint>
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
