#pragma once

#include "drs/engine/RuntimeModel.h"
#include "drs/engine/RuntimeStreamingService.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
enum class RuntimeVoiceLifecycleState
{
    idle,
    active,
    waitingForPage,
    releasing,
    finished,
    failed
};

struct RuntimeMacroValueSnapshot
{
    std::string id;
    double value = 0.0;
};

struct RuntimeVoiceAllocationRequest
{
    std::uint64_t voiceId = 0;
    std::string zoneId;
    int midiNote = 60;
    int velocity = 100;
    std::vector<RuntimeMacroValueSnapshot> macroValues;
    std::string articulationId;
};

struct RuntimeVoiceRouteResolution
{
    bool resolved = false;
    std::string state;
    std::string zoneId;
    std::string groupId;
    std::string articulationId;
    std::string sampleId;
    bool usedDefaultArticulation = false;
};

struct RuntimeVoiceStreamCursor
{
    std::string sampleId;
    std::uint64_t totalFrameCount = 0;
    std::uint64_t currentFrameIndex = 0;
    std::uint64_t bytesPerFrame = 0;
    std::uint64_t payloadRelativeOffsetBytes = 0;
    std::uint64_t prefetchBytes = 0;
    std::int32_t currentPageIndex = -1;
    std::uint64_t currentLeaseId = 0;
};

struct RuntimeVoiceSnapshot
{
    RuntimeVoiceLifecycleState state = RuntimeVoiceLifecycleState::idle;
    std::uint64_t voiceId = 0;
    int midiNote = 0;
    int velocity = 0;
    std::string zoneId;
    std::string groupId;
    std::string articulationId;
    std::string sampleId;
    bool releaseRequested = false;
    RuntimeVoiceStreamCursor cursor;
    std::vector<RuntimeMacroValueSnapshot> macroValues;
};

struct RuntimeVoiceAdvanceResult
{
    bool advanced = false;
    std::uint64_t framesAdvanced = 0;
    bool queuedPageRead = false;
    bool acquiredPageLease = false;
    bool releasedPageLease = false;
    bool waitingForPage = false;
    bool reachedSampleEnd = false;
    bool voiceFinished = false;
    std::int32_t currentPageIndex = -1;
    RuntimeVoiceLifecycleState state = RuntimeVoiceLifecycleState::idle;
};

RuntimeVoiceRouteResolution resolveRuntimeVoiceRoute(const RuntimeInstrumentModel& instrument,
                                                     const RuntimeStreamContainerModel& streamContainer,
                                                     const RuntimeVoiceAllocationRequest& request);

class RuntimeVoice
{
public:
    bool allocate(const RuntimeInstrumentModel& instrument,
                  const RuntimeStreamContainerModel& streamContainer,
                  const RuntimeVoiceAllocationRequest& request,
                  std::string& errorMessage);
    RuntimeVoiceAdvanceResult advanceFrames(std::uint64_t framesToAdvance,
                                           RuntimeStreamingService& streamingService);
    void beginRelease();
    void reset(RuntimeStreamingService& streamingService);
    RuntimeVoiceSnapshot getSnapshot() const;

private:
    void clearLocalState();
    void releaseCurrentLease(RuntimeStreamingService& streamingService, RuntimeVoiceAdvanceResult* result);
    void unregisterFromStreamingService(RuntimeStreamingService& streamingService);

    RuntimeVoiceLifecycleState state = RuntimeVoiceLifecycleState::idle;
    std::uint64_t voiceId = 0;
    int midiNote = 0;
    int velocity = 0;
    std::string zoneId;
    std::string groupId;
    std::string articulationId;
    std::string sampleId;
    bool releaseRequested = false;
    RuntimeVoiceStreamCursor cursor;
    std::vector<RuntimeMacroValueSnapshot> macroValues;
    const RuntimeStreamContainerModel* streamContainer = nullptr;
    const RuntimeStreamSampleDefinition* streamSample = nullptr;
    bool registeredWithStreamingService = false;
};
} // namespace drs::engine
