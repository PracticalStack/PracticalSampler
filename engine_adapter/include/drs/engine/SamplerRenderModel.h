#pragma once

#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PerformanceProgram.h"
#include "drs/engine/SampleDataSource.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace drs::engine
{
enum class SamplerRenderModelFindingSeverity
{
    warning,
    error
};

struct SamplerRenderModelFinding
{
    SamplerRenderModelFindingSeverity severity = SamplerRenderModelFindingSeverity::error;
    std::string code;
    std::string path;
    std::string message;
};

enum class SamplerRenderEventType : std::uint8_t
{
    noteOn,
    noteOff,
    sustainPedal,
    pedalDown,
    pedalUp,
    allNotesOff,
    roundRobinReset,
    reset,
    controllerChange
};

struct SamplerRenderEvent
{
    SamplerRenderEventType type = SamplerRenderEventType::noteOn;
    std::uint32_t sampleOffset = 0;
    std::uint8_t midiNote = 60;
    float velocity = 1.0f;
    // Kept now for future per-channel routing; v1 continues to route omni.
    std::uint8_t midiChannel = 0;
    float noteOffVelocity = 0.0f;
    std::uint32_t inputSequence = 0;
    std::uint32_t articulationIndex = kInvalidPerformanceProgramIndex;
    PerformanceEventKind performanceEvent = PerformanceEventKind::noteOn;
    bool sustainPedalDown = false;
    std::uint8_t controllerNumber = 0;
    std::uint8_t controllerValue = 0;
    RoundRobinResetEvent roundRobinResetEvent = RoundRobinResetEvent::articulationChange;
    bool hasPlaybackRegionOverride = false;
    std::uint64_t playbackStartFrameOverride = 0;
    std::uint64_t playbackEndFrameExclusiveOverride = 0;
    bool loopOverrideEnabled = false;
    std::uint64_t loopStartFrameOverride = 0;
    std::uint64_t loopEndFrameExclusiveOverride = 0;
};

// Non-owning callback views. Their backing storage is owned and bounded by the playback context.
struct SamplerRenderEventView
{
    const SamplerRenderEvent* data = nullptr;
    std::size_t size = 0;

    bool isValid() const noexcept { return data != nullptr || size == 0; }
    const SamplerRenderEvent& operator[](std::size_t index) const noexcept { return data[index]; }
};

struct SamplerAudioBufferView
{
    float* const* channels = nullptr;
    std::uint32_t channelCount = 0;
    std::uint32_t frameCount = 0;

    bool isValid() const noexcept;
};

struct SamplerRenderRequest
{
    SamplerAudioBufferView output;
    SamplerRenderEventView events;
    double outputSampleRate = 0.0;
};

struct SamplerRenderControlValues
{
    struct TransportView
    {
        double tempoBpm = 120.0;
        std::int64_t samplePosition = 0;
        std::int32_t timeSignatureNumerator = 4;
        std::int32_t timeSignatureDenominator = 4;
        bool valid = false;
        bool isPlaying = false;
        bool hasTempo = false;
        bool hasSamplePosition = false;
        bool hasTimeSignature = false;
    } transport;
    bool overrideMidiNoteOffset = false;
    int midiNoteOffset = 0;
    bool overrideFixedVelocity = false;
    int fixedVelocity = 0;
};

struct SamplerRenderResult
{
    std::uint32_t renderedFrameCount = 0;
    std::uint32_t consumedEventCount = 0;
    std::uint32_t startedVoiceCount = 0;
    std::uint32_t releasedVoiceCount = 0;
    std::uint32_t completedVoiceCount = 0;
    std::uint32_t stolenVoiceCount = 0;
    std::uint32_t generationStealCount = 0;
    std::uint32_t releasingVoiceStealCount = 0;
    std::uint32_t droppedEventCount = 0;
    std::uint32_t crossfadeStartedVoiceCount = 0;
    std::uint32_t crossfadeOverlapHitCount = 0;
    std::uint32_t crossfadeFallbackCount = 0;
    std::uint32_t roundRobinPoolHitCount = 0;
    std::uint32_t roundRobinPoolMissCount = 0;
    std::uint32_t roundRobinFallbackCount = 0;
    std::uint32_t chokedVoiceCount = 0;
    std::uint32_t pageMissCount = 0;
    std::uint32_t underrunFrameCount = 0;
    std::uint32_t pageRecoveryCount = 0;
};

struct SamplerRenderSample
{
    std::size_t preparedSampleIndex = 0;
    std::string sampleSourceId;
    std::string streamSampleId;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    SampleDataSourceDescriptor sourceDescriptor;
    SampleDataSourcePtr dataSource;
    // Temporary bounded resident compatibility ownership; render voices use dataSource.
    std::shared_ptr<const PreparedPlaybackDecodedSampleData> decodedSampleData;
};

struct SamplerRenderRoute
{
    std::size_t preparedZoneIndex = 0;
    std::size_t preparedSampleIndex = 0;
    std::string zoneId;
    std::string sampleSourceId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    double gainDb = 0.0;
    double pan = 0.0;
    std::uint64_t sampleStartFrame = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    double releaseSeconds = 0.0;
    double releaseShape = 0.0;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
    VelocityCrossfadeDescriptor velocityCrossfade;
    VelocityCrossfadeRuntimeDescriptor velocityCrossfadeRuntime;
    std::uint32_t performanceArticulationIndex = kInvalidPerformanceProgramIndex;
    PerformanceEventKind performanceEvent = PerformanceEventKind::noteOn;
    PerformanceSustainCondition performanceSustain = PerformanceSustainCondition::any;
    PerformancePitchSource performancePitchSource = PerformancePitchSource::eventNote;
    std::uint64_t performanceExclusiveGroupStableId = 0;
    std::uint64_t performanceChokeTargetMask = 0;
    float performanceChokeReleaseSeconds = 0.0f;
    double fineTuneCents = 0.0;
    double amplitudeVelocityTracking = 100.0;
    std::vector<RuntimeControllerCondition> controllerConditions;
    ContinuousDamperDefinition damper;
    RegionLoopMode loopMode = RegionLoopMode::noLoop;
    std::uint64_t sampleEndFrame = 0;
    std::uint64_t loopCrossfadeFrames = 0;
};

struct SamplerRenderModelBuildResult;

struct SamplerRenderModelBuildOptions
{
    std::string selectedZoneId;
    std::string selectedGroupId;
    std::string selectedArticulationId;
    std::vector<std::string> retainedZoneIds;
    bool auditionSelectedZone = false;
    int midiNoteOffset = 0;
    int fixedVelocity = 0;
};

class SamplerRenderModel final
{
public:
    PlaybackActivationLane getLane() const noexcept { return lane; }
    std::size_t getRevision() const noexcept { return revision; }
    std::uint64_t getSnapshotBuildId() const noexcept { return snapshotBuildId; }
    std::uint64_t getPreparedBuildId() const noexcept { return preparedBuildId; }
    const std::string& getSnapshotContentDigest() const noexcept { return snapshotContentDigest; }
    const std::string& getPreparedContentDigest() const noexcept { return preparedContentDigest; }
    const std::vector<SamplerRenderSample>& getSamples() const noexcept { return samples; }
    const std::vector<SamplerRenderRoute>& getRoutes() const noexcept { return routes; }
    const CompiledPerformanceProgram& getPerformanceProgram() const noexcept { return performanceProgram; }
    int getMidiNoteOffset() const noexcept { return midiNoteOffset; }
    int getFixedVelocity() const noexcept { return fixedVelocity; }
    bool usesContinuousDamper() const noexcept { return continuousDamperEnabled; }
    int getSustainControllerNumber() const noexcept { return sustainControllerNumber; }
    double getSustainThreshold() const noexcept { return sustainThreshold; }
    const PlaybackActivationPayloadPtr& getRetainedActivationPayload() const noexcept
    {
        return retainedActivationPayload;
    }

private:
    SamplerRenderModel() = default;
    friend SamplerRenderModelBuildResult buildSamplerRenderModel(
        const PlaybackActivationPayloadPtr& payload,
        const SamplerRenderModelBuildOptions& options);

    PlaybackActivationLane lane = PlaybackActivationLane::preview;
    std::size_t revision = 0;
    std::uint64_t snapshotBuildId = 0;
    std::uint64_t preparedBuildId = 0;
    std::string snapshotContentDigest;
    std::string preparedContentDigest;
    std::vector<SamplerRenderSample> samples;
    std::vector<SamplerRenderRoute> routes;
    CompiledPerformanceProgram performanceProgram;
    int midiNoteOffset = 0;
    int fixedVelocity = 0;
    bool continuousDamperEnabled = false;
    int sustainControllerNumber = legacySustainControllerNumber;
    double sustainThreshold = legacySustainThreshold;
    PlaybackActivationPayloadPtr retainedActivationPayload;
};

using SamplerRenderModelPtr = std::shared_ptr<const SamplerRenderModel>;

struct SamplerRenderModelBuildResult
{
    bool built = false;
    SamplerRenderModelPtr model;
    std::vector<SamplerRenderModelFinding> findings;
};

// Message/worker-owned construction seam. Validation and model allocation must never run in audio.
SamplerRenderModelBuildResult buildSamplerRenderModel(
    const PlaybackActivationPayloadPtr& payload,
    const SamplerRenderModelBuildOptions& options = {});
} // namespace drs::engine
