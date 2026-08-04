#pragma once

#include "drs/engine/RuntimeModel.h"
#include "drs/engine/SampleImport.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimeCompileSourceDefinition
{
    std::string id;
    std::string sourcePath;
    std::string role;
    ImportedSampleMetadata metadata;
};

struct RuntimeCompileZoneDefinition
{
    std::string id;
    std::string sourceId;
    std::string groupId;
    std::string articulationId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    VelocityCrossfadeDescriptor velocityCrossfade;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
    RuntimeProjectZonePerformanceDefinition performance;
    std::string exclusiveGroupId;
    std::vector<std::string> exclusiveTargetGroupIds;
    std::optional<double> chokeReleaseSeconds;
    std::uint64_t prefetchBytes = 16384;
};

struct RuntimeCompilePlan
{
    std::string outputProjectPath;
    std::string outputInstrumentPath;
    std::string outputStreamPath;
    std::string projectId;
    std::string projectDisplayName;
    std::string contentRootPath;
    std::string instrumentId;
    std::string instrumentDisplayName;
    std::string defaultLoadProfile;
    std::uint64_t pageSizeBytes = 65536;
    std::vector<RuntimeCompileSourceDefinition> sampleSources;
    std::vector<RuntimeMacroDefinition> macros;
    std::vector<RuntimeArticulationDefinition> articulations;
    std::vector<RuntimeGroupDefinition> groups;
    std::vector<RuntimeCompileZoneDefinition> zones;
    std::vector<RuntimeProjectRoundRobinResetRuleDefinition> roundRobinResetRules;
    std::vector<std::string> projectNotes;
    std::vector<std::string> instrumentValidationNotes;
    std::vector<std::string> streamNotes;
};

struct CompiledStreamPageDefinition
{
    std::uint32_t pageIndex = 0;
    std::uint64_t offsetBytes = 0;
    std::uint64_t sizeBytes = 0;
};

struct CompiledStreamSampleDefinition
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
    std::vector<CompiledStreamPageDefinition> pages;
};

struct RuntimeCompileResult
{
    bool compiled = false;
    std::string state;
    std::vector<std::string> warnings;
    std::vector<std::string> issues;
    RuntimeProjectModel project;
    RuntimeInstrumentModel instrument;
    std::string containerId;
    std::uint64_t pageSizeBytes = 65536;
    std::uint64_t totalPayloadBytes = 0;
    std::uint64_t alignedPayloadBytes = 0;
    std::uint64_t totalPageCount = 0;
    std::string payloadFilePath;
    std::string payloadFileChecksumHex;
    std::vector<CompiledStreamSampleDefinition> streamSamples;
};

struct RuntimeStreamWriteResult
{
    bool written = false;
    std::string state;
    std::vector<std::string> issues;
    std::string containerPath;
    std::string payloadPath;
    std::uint64_t totalPayloadBytes = 0;
    std::uint64_t alignedPayloadBytes = 0;
    std::uint64_t totalPageCount = 0;
    std::string payloadFileChecksumHex;
};

RuntimeCompileResult compileRuntimeInstrument(const RuntimeCompilePlan& plan);
std::string buildCompiledStreamPayloadPath(const std::string& containerPath);
RuntimeStreamWriteResult writeCompiledStreamAssets(RuntimeCompileResult& result);
std::string serializeCompiledStreamIndex(const RuntimeCompileResult& result, const std::string& containerPath);
} // namespace drs::engine
