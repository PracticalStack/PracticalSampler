#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimeProjectSampleSource
{
    std::string id;
    std::string path;
    std::string role;
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
    std::uint64_t streamOffsetBytes = 0;
    std::uint64_t prefetchBytes = 0;
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

struct RuntimeProjectLoadResult
{
    bool manifestFound = false;
    bool loaded = false;
    std::string manifestPath;
    std::string state;
    std::vector<std::string> issues;
    RuntimeProjectModel project;
};
} // namespace drs::engine
