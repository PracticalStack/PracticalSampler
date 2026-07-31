#pragma once

#include "drs/engine/RuntimeModel.h"

#include <algorithm>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace juce
{
class AudioFormatReader;
}

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

struct SampleInspectionResult
{
    bool fileFound = false;
    bool inspected = false;
    bool accepted = false;
    std::string sourcePath;
    std::string state;
    std::vector<std::string> warnings;
    std::vector<std::string> issues;
    ImportedSampleMetadata metadata;
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

struct WaveformPeakPoint
{
    float minValue = 0.0f;
    float maxValue = 0.0f;
};

enum class WaveformPeakChannelReduction
{
    firstChannel,
    averageChannels,
    channelExtrema
};

struct WaveformPeakBuildProgress
{
    std::uint64_t framesProcessed = 0;
    std::uint64_t totalFrames = 0;
    std::size_t pointsCompleted = 0;
    std::size_t totalPointCount = 0;
};

struct WaveformPeakBuildCallbacks
{
    virtual ~WaveformPeakBuildCallbacks() = default;

    virtual bool isCancellationRequested() const;
    virtual void onProgress(const WaveformPeakBuildProgress& progress) const;
};

struct WaveformPeakBuildOptions
{
    std::size_t displayPointCount = 192;
    std::uint64_t chunkFrameCount = 4096;
    WaveformPeakChannelReduction channelReduction = WaveformPeakChannelReduction::channelExtrema;
    const WaveformPeakBuildCallbacks* callbacks = nullptr;
};

struct WaveformPeakBuildResult
{
    bool fileFound = false;
    bool built = false;
    bool canceled = false;
    std::string sourcePath;
    std::string state;
    std::vector<std::string> issues;
    ImportedSampleMetadata metadata;
    std::vector<WaveformPeakPoint> points;
};

struct SampleSourceFingerprintResult
{
    bool fileFound = false;
    bool fingerprinted = false;
    bool canceled = false;
    std::string sourcePath;
    std::string fingerprintHex;
    std::string state;
    std::vector<std::string> issues;
};

struct SampleFingerprintProgress
{
    std::uint64_t bytesProcessed = 0;
    std::uint64_t totalBytes = 0;
};

struct SampleFingerprintCallbacks
{
    virtual ~SampleFingerprintCallbacks() = default;

    virtual bool isCancellationRequested() const;
    virtual void onProgress(const SampleFingerprintProgress& progress) const;
};

struct SampleFingerprintOptions
{
    std::uint64_t chunkSizeBytes = 4096;
    const SampleFingerprintCallbacks* callbacks = nullptr;
};

struct SampleImportIoCounters
{
    std::uint64_t fingerprintOpenCount = 0;
    std::uint64_t readerOpenCount = 0;
    std::uint64_t bytesReadCount = 0;
    std::uint64_t fullFrameReadCount = 0;
    std::uint64_t copyCount = 0;
    std::uint64_t peakChunkReadCount = 0;
};

struct SampleImportHooks
{
    virtual ~SampleImportHooks() = default;

    virtual bool fileExists(const std::string& samplePath) const;
    virtual bool copyFile(const std::string& sourcePath, const std::string& destinationPath) const;
    virtual std::unique_ptr<std::istream> openFingerprintStream(const std::string& samplePath) const;
    virtual std::unique_ptr<juce::AudioFormatReader> createAudioReader(const std::string& samplePath) const;
};

class ScopedSampleImportHooksOverride
{
public:
    explicit ScopedSampleImportHooksOverride(const SampleImportHooks& hooks) noexcept;
    ~ScopedSampleImportHooksOverride();

    ScopedSampleImportHooksOverride(const ScopedSampleImportHooksOverride&) = delete;
    ScopedSampleImportHooksOverride& operator=(const ScopedSampleImportHooksOverride&) = delete;

private:
    const SampleImportHooks* previousHooks = nullptr;
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
};

struct AuthoringImportQueueItem
{
    std::string id;
    std::string sourcePath;
    std::string knownFingerprintHex;
    AuthoringImportItemState state = AuthoringImportItemState::pending;
    SampleInspectionResult inspectionResult;
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
SampleImportIoCounters getSampleImportIoCounters();
void resetSampleImportIoCounters();
SampleSourceFingerprintResult fingerprintSampleSourceFile(const std::string& samplePath,
                                                          const SampleFingerprintOptions& options = {});
bool copySampleFileForImport(const std::string& sourcePath, const std::string& destinationPath);
SampleInspectionResult inspectSampleFile(const std::string& samplePath,
                                         const std::string& knownFingerprintHex = {});
SampleImportResult importSampleFile(const std::string& samplePath,
                                    const std::string& knownFingerprintHex = {});
WaveformPeakBuildResult buildWaveformPeaks(const std::string& samplePath,
                                           const std::string& knownFingerprintHex = {},
                                           const WaveformPeakBuildOptions& options = {});
void recordWaveformPeakChunkRead(std::uint64_t chunkFrameCount, std::uint32_t channelCount);
SampleRootKeyInferenceResult inferSampleRootKey(const std::string& samplePath,
                                                const ImportedSampleMetadata* metadata = nullptr);
ParsedSampleFilenameHeuristics parseSampleFilenameHeuristics(const std::string& samplePath,
                                                             const ImportedSampleMetadata* metadata = nullptr);
inline void reconcileBatchInferredRoundRobinDescriptors(std::vector<RuntimeProjectZoneDefinition>& zones)
{
    struct PairingKey
    {
        std::string poolId;
        std::string articulationId;
        int rootKey = 60;
        int keyLow = 0;
        int keyHigh = 127;

        bool operator==(const PairingKey& other) const noexcept
        {
            return poolId == other.poolId
                && articulationId == other.articulationId
                && rootKey == other.rootKey
                && keyLow == other.keyLow
                && keyHigh == other.keyHigh;
        }
    };

    struct PairingKeyHash
    {
        std::size_t operator()(const PairingKey& key) const noexcept
        {
            std::size_t hash = std::hash<std::string> {}(key.poolId);
            hash ^= std::hash<std::string> {}(key.articulationId) + 0x9e3779b9 + (hash << 6u) + (hash >> 2u);
            hash ^= std::hash<int> {}(key.rootKey) + 0x9e3779b9 + (hash << 6u) + (hash >> 2u);
            hash ^= std::hash<int> {}(key.keyLow) + 0x9e3779b9 + (hash << 6u) + (hash >> 2u);
            hash ^= std::hash<int> {}(key.keyHigh) + 0x9e3779b9 + (hash << 6u) + (hash >> 2u);
            return hash;
        }
    };

    struct ZoneMember
    {
        std::size_t zoneIndex = 0;
        int originalSlotIndex = 0;
        RoundRobinMode mode = RoundRobinMode::sequential;
    };

    const auto clearRoundRobinDescriptor = [](RuntimeProjectZoneDefinition& zone)
    {
        zone.roundRobin.reset();
        zone.roundRobinLength = 0;
        zone.roundRobinPosition = 0;
    };
    const auto applyRoundRobinDescriptor = [](RuntimeProjectZoneDefinition& zone,
                                              const std::string& poolId,
                                              int slotCount,
                                              int slotIndex,
                                              RoundRobinMode mode)
    {
        zone.roundRobin = RoundRobinDescriptor {
            poolId,
            slotCount,
            slotIndex,
            mode
        };
        zone.roundRobinLength = slotCount;
        zone.roundRobinPosition = slotIndex;
    };
    const auto computeFnv1a64Hex = [](const std::string& text)
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (const auto character : text)
        {
            hash ^= static_cast<unsigned char>(character);
            hash *= 1099511628211ull;
        }

        static constexpr char digits[] = "0123456789abcdef";
        std::string hex(16, '0');
        for (int index = 15; index >= 0; --index)
        {
            hex[static_cast<std::size_t>(index)] = digits[hash & 0x0full];
            hash >>= 4u;
        }
        return hex;
    };
    const auto buildSubgroupPoolId = [&](const PairingKey& key)
    {
        return key.poolId + "-"
            + computeFnv1a64Hex(key.articulationId
                                + "|"
                                + std::to_string(key.rootKey)
                                + "|"
                                + std::to_string(key.keyLow)
                                + "|"
                                + std::to_string(key.keyHigh));
    };

    std::unordered_map<PairingKey, std::vector<ZoneMember>, PairingKeyHash> groupedZones;
    std::unordered_map<std::string, std::size_t> poolSubgroupCounts;

    for (std::size_t zoneIndex = 0; zoneIndex < zones.size(); ++zoneIndex)
    {
        auto& zone = zones[zoneIndex];
        if (!zone.roundRobin.has_value())
        {
            if (zone.roundRobinLength > 0 || zone.roundRobinPosition > 0)
                clearRoundRobinDescriptor(zone);
            continue;
        }

        const auto& roundRobin = *zone.roundRobin;
        if (roundRobin.poolId.empty() || roundRobin.slotCount <= 0 || roundRobin.slotIndex <= 0)
        {
            clearRoundRobinDescriptor(zone);
            continue;
        }

        PairingKey key;
        key.poolId = roundRobin.poolId;
        key.articulationId = zone.articulationId;
        key.rootKey = zone.rootKey;
        key.keyLow = zone.keyLow;
        key.keyHigh = zone.keyHigh;

        groupedZones[key].push_back({ zoneIndex, roundRobin.slotIndex, roundRobin.mode });
    }

    if (groupedZones.empty())
        return;

    for (const auto& [key, members] : groupedZones)
        ++poolSubgroupCounts[key.poolId];

    for (auto& [key, members] : groupedZones)
    {
        const auto clearGroup = [&]()
        {
            for (const auto& member : members)
                clearRoundRobinDescriptor(zones[member.zoneIndex]);
        };

        if (members.size() < 2)
        {
            clearGroup();
            continue;
        }

        std::sort(members.begin(),
                  members.end(),
                  [](const ZoneMember& left, const ZoneMember& right)
                  {
                      if (left.originalSlotIndex != right.originalSlotIndex)
                          return left.originalSlotIndex < right.originalSlotIndex;

                      return left.zoneIndex < right.zoneIndex;
                  });

        bool invalidSlotLayout = false;
        for (std::size_t memberIndex = 0; memberIndex < members.size(); ++memberIndex)
        {
            const auto& member = members[memberIndex];
            if (member.originalSlotIndex <= 0)
            {
                invalidSlotLayout = true;
                break;
            }

            if (memberIndex > 0
                && members[memberIndex - 1].originalSlotIndex == member.originalSlotIndex)
            {
                invalidSlotLayout = true;
                break;
            }
        }

        if (invalidSlotLayout)
        {
            clearGroup();
            continue;
        }

        const auto poolId = poolSubgroupCounts[key.poolId] > 1
            ? buildSubgroupPoolId(key)
            : key.poolId;

        for (std::size_t memberIndex = 0; memberIndex < members.size(); ++memberIndex)
        {
            auto& zone = zones[members[memberIndex].zoneIndex];
            applyRoundRobinDescriptor(zone,
                                      poolId,
                                      static_cast<int>(members.size()),
                                      static_cast<int>(memberIndex) + 1,
                                      members[memberIndex].mode);
        }
    }
}
AuthoringImportQueue createAuthoringImportQueue(const std::vector<std::string>& samplePaths,
                                                const std::string& contentRootPath);
AuthoringImportProcessResult processNextAuthoringImportQueueItem(AuthoringImportQueue& queue);
bool cancelAuthoringImportQueueItem(AuthoringImportQueue& queue, const std::string& itemId);
bool acceptAuthoringImportQueueItem(AuthoringImportQueue& queue, const std::string& itemId);
} // namespace drs::engine
