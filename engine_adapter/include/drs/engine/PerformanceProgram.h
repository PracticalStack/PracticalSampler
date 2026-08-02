#pragma once

#include "drs/engine/RuntimeModel.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
constexpr std::uint32_t kInvalidPerformanceProgramIndex = 0xffffffffu;

struct CompiledPerformanceEventRange
{
    std::uint32_t firstRoute = 0;
    std::uint32_t routeCount = 0;
};

// These records are intentionally string-free. They are retained by the playback context and
// are the only performance-rule records intended for a future audio-thread evaluator.
struct CompiledPerformanceActivation
{
    std::uint32_t articulationIndex = kInvalidPerformanceProgramIndex;
    bool consume = false;
};

struct CompiledPerformanceTriggerRoute
{
    std::uint32_t zoneIndex = 0;
    std::uint32_t articulationIndex = kInvalidPerformanceProgramIndex;
    std::uint32_t exclusiveGroupIndex = kInvalidPerformanceProgramIndex;
    std::uint64_t chokeTargetMask = 0;
    float chokeReleaseSeconds = 0.0f;
    PerformanceEventKind event = PerformanceEventKind::noteOn;
    PerformanceSustainCondition sustain = PerformanceSustainCondition::any;
    PerformancePitchSource pitchSource = PerformancePitchSource::eventNote;
};

struct CompiledPerformanceRoundRobinReset
{
    RoundRobinResetEvent event = RoundRobinResetEvent::articulationChange;
    std::uint32_t targetPoolIndex = kInvalidPerformanceProgramIndex; // invalid means all pools
};

struct CompiledPerformanceProgram
{
    std::array<CompiledPerformanceEventRange, 5> eventRanges {};
    std::array<CompiledPerformanceActivation, 128> activationByMidiNote {};
    std::vector<CompiledPerformanceTriggerRoute> triggerRoutes;
    std::vector<CompiledPerformanceRoundRobinReset> roundRobinResets;
    // Stable-ID hashes support allocation-free selection migration between published programs.
    std::vector<std::uint64_t> articulationStableIds;
    std::uint32_t articulationCount = 0;
    std::uint32_t defaultArticulationIndex = kInvalidPerformanceProgramIndex;
    std::uint32_t exclusiveGroupCount = 0;
    std::uint32_t roundRobinPoolCount = 0;
    std::uint64_t retainedBytes = 0;
};

struct CompiledPerformanceProgramResult
{
    bool compiled = false;
    CompiledPerformanceProgram program;
    std::vector<std::string> issues;
};

CompiledPerformanceProgramResult compilePerformanceProgram(const RuntimeProjectAuthoringState& authoring);
std::string serializeCompiledPerformanceProgram(const CompiledPerformanceProgram& program);
} // namespace drs::engine
