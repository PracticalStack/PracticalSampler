#pragma once

#include "drs/engine/VelocityCrossfade.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
enum class ZoneTriggerMode : std::uint8_t
{
    gated,
    oneShot
};

enum class RoundRobinMode : std::uint8_t
{
    sequential,
    random
};

enum class PerformanceEventKind : std::uint8_t
{
    noteOn,
    noteOff,
    release,
    pedalDown,
    pedalUp,
    controllerChange
};

inline constexpr std::size_t kPerformanceEventKindCount = 6;

enum class PerformanceSustainCondition : std::uint8_t
{
    any,
    pedalUp,
    pedalDown
};

enum class PerformancePitchSource : std::uint8_t
{
    eventNote,
    fixedRoot,
    eventKeyFixedPitch
};

enum class ArticulationActivationMode : std::uint8_t
{
    latch
};

enum class RoundRobinResetEvent : std::uint8_t
{
    programActivation,
    articulationChange,
    allNotesOff,
    pedalDown,
    pedalUp
};

struct RoundRobinDescriptor
{
    std::string poolId;
    int slotCount = 0;
    int slotIndex = 0;
    RoundRobinMode mode = RoundRobinMode::sequential;
};

inline bool operator==(const RoundRobinDescriptor& left, const RoundRobinDescriptor& right) noexcept
{
    return left.poolId == right.poolId
        && left.slotCount == right.slotCount
        && left.slotIndex == right.slotIndex
        && left.mode == right.mode;
}

inline bool operator!=(const RoundRobinDescriptor& left, const RoundRobinDescriptor& right) noexcept
{
    return !(left == right);
}

struct RuntimeProjectSampleSource
{
    std::string id;
    std::string path;
    std::string role;
};

struct RuntimeProjectZonePerformanceDefinition
{
    PerformanceEventKind event = PerformanceEventKind::noteOn;
    PerformanceSustainCondition sustain = PerformanceSustainCondition::any;
    PerformancePitchSource pitchSource = PerformancePitchSource::eventNote;
    // Identifies the CC edge that emits a controllerChange trigger. Static
    // controllerConditions remain independent and retain their authored order.
    std::optional<int> triggerControllerNumber;
};

struct RuntimeControllerCondition
{
    int controllerNumber = 0;
    int minimumValue = 0;
    int maximumValue = 127;

    bool operator==(const RuntimeControllerCondition& other) const noexcept
    {
        return controllerNumber == other.controllerNumber
            && minimumValue == other.minimumValue
            && maximumValue == other.maximumValue;
    }
};

struct RuntimeControllerDefault
{
    int controllerNumber = 0;
    int value = 0;

    bool operator==(const RuntimeControllerDefault& other) const noexcept
    {
        return controllerNumber == other.controllerNumber && value == other.value;
    }
};

struct RuntimeProjectZoneDefinition
{
    std::string id;
    std::string sampleSourceId;
    std::string displayName;
    std::string groupId;
    std::string articulationId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    VelocityCrossfadeDescriptor velocityCrossfade;
    double gainDb = 0.0;
    double pan = 0.0;
    std::uint64_t sampleStartFrame = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    double releaseSeconds = 0.0;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
    RuntimeProjectZonePerformanceDefinition performance;
    std::string exclusiveGroupId;
    std::vector<std::string> exclusiveTargetGroupIds;
    std::optional<double> chokeReleaseSeconds;
    double fineTuneCents = 0.0;
    double amplitudeVelocityTracking = 100.0;
    std::vector<RuntimeControllerCondition> controllerConditions;
};

// Sprint 1 stores articulation identity independently from zone membership. The
// activation record is persisted here, but its typed rule compilation/execution
// deliberately begins in Sprint 2 and later.
struct RuntimeProjectArticulationActivationDefinition
{
    PerformanceEventKind event = PerformanceEventKind::noteOn;
    int midiNote = 0;
    ArticulationActivationMode mode = ArticulationActivationMode::latch;
    bool consume = true;
};

struct RuntimeProjectArticulationDefinition
{
    std::string id;
    std::string displayName;
    bool isDefault = false;
    int displayOrder = 0;
    std::optional<RuntimeProjectArticulationActivationDefinition> activation;
};

struct RuntimeProjectRoundRobinResetRuleDefinition
{
    RoundRobinResetEvent event = RoundRobinResetEvent::articulationChange;
    bool targetAll = true;
    std::string targetPoolId;
};

struct RuntimeProjectMacroTargetControlLaw
{
    std::string id;
    std::uint32_t version = 0;
};

struct RuntimeProjectMacroTargetDefinition
{
    std::string parameterId;
    std::string parameterPath;
    std::string role;
    // Empty fields preserve the legacy free-form target. A DSP target always carries both
    // stable authored identities plus an explicit mapping range and curve.
    std::string dspSlotId;
    std::string dspParameterId;
    double sourceMinimum = 0.0;
    double sourceMaximum = 1.0;
    double destinationMinimum = 0.0;
    double destinationMaximum = 1.0;
    std::string curve = "linear";
    // Empty means a pre-control-law legacy target. Its persisted curve remains
    // authoritative until the explicit upgrade command is applied.
    RuntimeProjectMacroTargetControlLaw controlLaw;
};

struct RuntimeProjectMacroDefinition
{
    std::string id;
    std::string name;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    std::vector<RuntimeProjectMacroTargetDefinition> targets;
    bool exposedInPerformance = false;
};

struct RuntimeProjectGroupDefinition
{
    std::string id;
    std::string displayName;
    int displayOrder = 0;
    bool workspaceVisible = true;
    double gainDb = 0.0;
    double pan = 0.0;
    std::string routingBusId;
    std::string auditionAnchorZoneId;
};

struct RuntimeProjectFxSlotDefinition
{
    std::string id;
    std::string displayName;
    std::string effectType;
    bool bypassed = false;
    // Version zero means legacy metadata has not yet been mapped to a catalog algorithm.
    std::uint32_t effectVersion = 0;
    struct ParameterValue
    {
        std::string id;
        double value = 0.0;
    };
    // Ordered records deliberately preserve unknown parameters for a later catalog/runtime.
    std::vector<ParameterValue> parameters;
    // Loader-owned compatibility state; never serialized over the authored bypass value.
    bool unavailable = false;
    bool legacyInert = false;
};

struct RuntimeProjectRoutingBusDefinition
{
    std::string id;
    std::string displayName;
    std::string inputSourceId;
    std::vector<std::string> fxSlotIds;
    bool chainBypassed = false;
};

struct RuntimeProjectTriggerSlotDefinition
{
    std::string id;
    std::string displayName;
    std::string triggerEvent;
    std::string targetArticulationId;
    std::string phraseAssetId;
    std::string chordMode;
};

struct RuntimeProjectPhraseNoteDefinition
{
    int midiNote = 60;
    int velocity = 96;
    double startBeat = 0.0;
    double durationBeats = 1.0;
};

struct RuntimeProjectPhraseAssetDefinition
{
    std::string id;
    std::string displayName;
    std::string sourcePath;
    int ticksPerQuarter = 960;
    double lengthBeats = 0.0;
    std::string chordHint;
    std::string normalizationState;
    std::vector<std::string> issues;
    std::vector<RuntimeProjectPhraseNoteDefinition> notes;
};

struct RuntimeProjectPerformanceBankDefinition
{
    std::string id;
    std::string displayName;
    std::vector<RuntimeProjectTriggerSlotDefinition> triggerSlots;
    std::vector<RuntimeProjectPhraseAssetDefinition> phraseAssets;
    std::vector<std::string> notes;
};

struct RuntimeProjectAuthoringState
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string selectedZoneId;
    std::string selectedGroupId;
    std::string selectedPerformanceBankId;
    double masterGainDb = 0.0;
    std::vector<RuntimeProjectArticulationDefinition> articulations;
    std::vector<RuntimeProjectRoundRobinResetRuleDefinition> roundRobinResetRules;
    std::vector<RuntimeControllerDefault> controllerDefaults;
    std::vector<RuntimeProjectZoneDefinition> zones;
    std::vector<RuntimeProjectGroupDefinition> groups;
    std::vector<RuntimeProjectMacroDefinition> macros;
    std::vector<RuntimeProjectFxSlotDefinition> fxSlots;
    std::vector<RuntimeProjectRoutingBusDefinition> routingBuses;
    std::vector<RuntimeProjectPerformanceBankDefinition> performanceBanks;
    std::vector<std::string> notes;
};

struct RuntimeProjectModel
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string projectId;
    std::string displayName;
    std::string contentRootPath;
    std::string defaultInstrumentManifestPath;
    std::vector<RuntimeProjectSampleSource> sampleSources;
    RuntimeProjectAuthoringState authoring;
    std::vector<std::string> notes;
};

struct RuntimeMacroDefinition
{
    std::string id;
    std::string name;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
};

struct RuntimeArticulationDefinition
{
    std::string id;
    std::string name;
    bool isDefault = false;
    std::optional<RuntimeProjectArticulationActivationDefinition> activation;
};

struct RuntimeGroupDefinition
{
    std::string id;
    std::string name;
    std::vector<std::string> articulationIds;
    double gainDb = 0.0;
};

struct RuntimeZoneDefinition
{
    std::string id;
    std::string groupId;
    std::string articulationId;
    std::string samplePath;
    std::string streamAssetPath;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    VelocityCrossfadeDescriptor velocityCrossfade;
    VelocityCrossfadeRuntimeDescriptor velocityCrossfadeRuntime;
    double gainDb = 0.0;
    std::uint64_t sampleStartFrame = 0;
    std::uint64_t streamOffsetBytes = 0;
    std::uint64_t prefetchBytes = 0;
    double releaseSeconds = 0.0;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
    RuntimeProjectZonePerformanceDefinition performance;
    std::string exclusiveGroupId;
    std::vector<std::string> exclusiveTargetGroupIds;
    std::optional<double> chokeReleaseSeconds;
    double fineTuneCents = 0.0;
    double amplitudeVelocityTracking = 100.0;
    std::vector<RuntimeControllerCondition> controllerConditions;
};

struct RuntimeInstrumentModel
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string instrumentId;
    std::string displayName;
    std::string sourceProjectPath;
    std::string compiledStreamAssetPath;
    std::string defaultLoadProfile;
    std::vector<RuntimeMacroDefinition> macros;
    std::vector<RuntimeArticulationDefinition> articulations;
    std::vector<RuntimeGroupDefinition> groups;
    std::vector<RuntimeZoneDefinition> zones;
    std::vector<RuntimeProjectRoundRobinResetRuleDefinition> roundRobinResetRules;
    std::vector<RuntimeControllerDefault> controllerDefaults;
    std::vector<std::string> validationNotes;
};

struct RuntimeStreamPageDefinition
{
    std::uint32_t pageIndex = 0;
    std::uint64_t offsetBytes = 0;
    std::uint64_t sizeBytes = 0;
};

struct RuntimeStreamSampleDefinition
{
    std::string sampleId;
    std::string sourcePath;
    std::string sourceChecksumHex;
    std::string payloadChecksumHex;
    std::string formatName;
    std::string role;
    std::string channelLayout;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    std::uint64_t payloadOffsetBytes = 0;
    std::uint64_t payloadSizeBytes = 0;
    std::uint64_t prefetchBytes = 0;
    bool rootMidiNotePresent = false;
    int rootMidiNote = 60;
    bool loopRangePresent = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    std::vector<RuntimeStreamPageDefinition> pages;
};

struct RuntimeStreamContainerModel
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string containerId;
    std::uint64_t pageSizeBytes = 0;
    std::string payloadEncoding;
    std::uint64_t totalPayloadBytes = 0;
    std::string payloadAssetPath;
    std::uint64_t payloadFileBytes = 0;
    std::string payloadFileChecksumHex;
    bool payloadEmbedded = false;
    std::vector<std::uint8_t> embeddedPayloadBytes;
    std::vector<RuntimeStreamSampleDefinition> samples;
    std::vector<std::string> notes;
};

struct RuntimeLoadMetrics
{
    std::size_t macroCount = 0;
    std::size_t articulationCount = 0;
    std::size_t groupCount = 0;
    std::size_t zoneCount = 0;
    std::size_t referencedSampleCount = 0;
    std::uint64_t totalPrefetchBytes = 0;
    std::uint64_t manifestSizeBytes = 0;
    std::uint64_t loadDurationMicros = 0;
    bool usesStreaming = false;
    bool sourceProjectResolved = false;
    bool compiledStreamAssetResolved = false;
};

struct RuntimeStreamLoadMetrics
{
    std::size_t sampleCount = 0;
    std::size_t pageCount = 0;
    std::size_t checksumValidatedCount = 0;
    std::size_t payloadChecksumValidatedCount = 0;
    bool payloadAssetResolved = false;
    bool payloadLayoutValidated = false;
    bool pageTableValidated = false;
};

struct RuntimeManifestLoadResult
{
    bool manifestFound = false;
    bool loaded = false;
    std::string manifestPath;
    std::string state;
    std::vector<std::string> issues;
    RuntimeInstrumentModel instrument;
    RuntimeLoadMetrics metrics;
};

struct RuntimeStreamLoadResult
{
    bool containerFound = false;
    bool loaded = false;
    std::string containerPath;
    std::string state;
    std::vector<std::string> issues;
    RuntimeStreamContainerModel container;
    RuntimeStreamLoadMetrics metrics;
};

struct RuntimeStreamReadResult
{
    bool resolved = false;
    bool inPrefetchHead = false;
    bool inPageTable = false;
    std::string state;
    std::string sampleId;
    std::uint64_t payloadRelativeOffsetBytes = 0;
    std::uint64_t absoluteOffsetBytes = 0;
    std::uint64_t readableBytes = 0;
    std::uint32_t pageIndex = 0;
};

struct RuntimeProjectLoadResult
{
    bool manifestFound = false;
    bool loaded = false;
    std::string manifestPath;
    std::string state;
    std::vector<std::string> issues;
    std::vector<std::string> warnings;
    RuntimeProjectModel project;
};

struct RuntimeProjectValidationResult
{
    bool valid = false;
    std::string state;
    std::vector<std::string> issues;
};

struct RuntimeProjectMigrationResult
{
    bool valid = false;
    bool migrated = false;
    std::string state;
    std::vector<std::string> issues;
    RuntimeProjectModel project;
};
} // namespace drs::engine
