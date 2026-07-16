#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct ImportedSampleMetadata
{
    std::string sourcePath;
    std::string formatName;
    std::string sourceChecksumHex;
    std::string channelLayout;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    std::uint32_t bitsPerSample = 0;
    bool usesFloatingPointData = false;
    double durationSeconds = 0.0;
    bool rootMidiNotePresent = false;
    int rootMidiNote = 60;
    bool loopRangePresent = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
};

struct ImportedSampleData
{
    ImportedSampleMetadata metadata;
    std::vector<std::vector<float>> normalizedChannels;
};

struct SampleImportPolicyReport
{
    bool accepted = false;
    std::string state;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct SampleImportResult
{
    bool fileFound = false;
    bool imported = false;
    std::string sourcePath;
    std::string state;
    std::vector<std::string> warnings;
    std::vector<std::string> issues;
    ImportedSampleData sample;
};

enum class SampleFilenameTokenKind
{
    unknown,
    text,
    rootNote,
    velocity,
    roundRobin,
    articulation
};

enum class AuthoringImportFindingSeverity
{
    info,
    warning,
    error
};

enum class AuthoringImportItemState
{
    pending,
    parsing,
    inferred,
    warning,
    failed,
    canceled,
    accepted
};

struct SampleFilenameToken
{
    SampleFilenameTokenKind kind = SampleFilenameTokenKind::unknown;
    std::string text;
    std::string normalizedText;
    std::string canonicalValue;
    int numericValue = 0;
};

struct AuthoringImportFinding
{
    AuthoringImportFindingSeverity severity = AuthoringImportFindingSeverity::info;
    std::string code;
    std::string summary;
    std::string detail;
    bool requiresConfirmation = false;
    std::vector<std::string> relatedTokens;
};

struct AuthoringImportZoneSuggestion
{
    bool suggested = false;
    std::string sourceSampleId;
    RuntimeProjectZoneDefinition zone;
    std::string rootKeySource;
    std::string velocitySource;
    int roundRobinIndex = 0;
};

struct AuthoringImportQueueItem
{
    std::string id;
    std::string sourcePath;
    AuthoringImportItemState state = AuthoringImportItemState::pending;
    SampleImportResult importResult;
    std::vector<SampleFilenameToken> filenameTokens;
    std::vector<AuthoringImportFinding> findings;
    AuthoringImportZoneSuggestion suggestedZone;
};

struct AuthoringImportQueueMetrics
{
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
    std::string state;
};

struct AuthoringImportQueue
{
    std::string contentRootPath;
    std::vector<AuthoringImportQueueItem> items;
    AuthoringImportQueueMetrics metrics;
};

struct AuthoringImportProcessResult
{
    bool processed = false;
    bool queueDrained = false;
    std::string itemId;
    std::string state;
    std::uint64_t durationMicros = 0;
    std::vector<std::string> issues;
};

struct ParsedSampleFilenameHeuristics
{
    std::vector<SampleFilenameToken> tokens;
    std::vector<AuthoringImportFinding> findings;
    AuthoringImportZoneSuggestion suggestedZone;
};

struct SampleRootKeyInferenceResult
{
    bool resolved = false;
    int rootKey = 60;
    std::string source = "manual";
    std::vector<AuthoringImportFinding> findings;
};

SampleImportPolicyReport evaluatePhase1SamplePolicy(const ImportedSampleMetadata& metadata,
                                                    const std::string& contentRootPath = {});
SampleImportResult importSampleFile(const std::string& samplePath);
SampleRootKeyInferenceResult inferSampleRootKey(const std::string& samplePath,
                                                const ImportedSampleMetadata* metadata = nullptr);
ParsedSampleFilenameHeuristics parseSampleFilenameHeuristics(const std::string& samplePath,
                                                             const ImportedSampleMetadata* metadata = nullptr);
AuthoringImportQueue createAuthoringImportQueue(const std::vector<std::string>& samplePaths,
                                                const std::string& contentRootPath);
AuthoringImportProcessResult processNextAuthoringImportQueueItem(AuthoringImportQueue& queue);
bool cancelAuthoringImportQueueItem(AuthoringImportQueue& queue, const std::string& itemId);
bool acceptAuthoringImportQueueItem(AuthoringImportQueue& queue, const std::string& itemId);
} // namespace drs::engine
