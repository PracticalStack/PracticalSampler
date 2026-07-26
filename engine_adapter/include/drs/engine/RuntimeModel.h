#pragma once

#include "drs/engine/VelocityCrossfade.h"

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
    sequential
};

struct RoundRobinDescriptor
{
    std::string poolId;
    int slotCount = 0;
    int slotIndex = 0;
    RoundRobinMode mode = RoundRobinMode::sequential;
};

struct RuntimeProjectSampleSource
{
    std::string id;
    std::string path;
    std::string role;
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
};

struct RuntimeProjectMacroTargetDefinition
{
    std::string parameterId;
    std::string parameterPath;
    std::string role;
};

struct RuntimeProjectMacroDefinition
{
    std::string id;
    std::string name;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    std::vector<RuntimeProjectMacroTargetDefinition> targets;
};

struct RuntimeProjectFxSlotDefinition
{
    std::string id;
    std::string displayName;
    std::string effectType;
    bool bypassed = false;
};

struct RuntimeProjectRoutingBusDefinition
{
    std::string id;
    std::string displayName;
    std::string inputSourceId;
    std::vector<std::string> fxSlotIds;
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
    std::string selectedPerformanceBankId;
    std::vector<RuntimeProjectZoneDefinition> zones;
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
};

struct RuntimeGroupDefinition
{
    std::string id;
    std::string name;
    std::vector<std::string> articulationIds;
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
    std::uint64_t streamOffsetBytes = 0;
    std::uint64_t prefetchBytes = 0;
    double releaseSeconds = 0.0;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
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
