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
    std::uint8_t triggerControllerNumber = 0;
    std::uint8_t triggerControllerMinimum = 0;
    std::uint8_t triggerControllerMaximum = 127;
};

struct CompiledPerformanceRoundRobinReset
{
    RoundRobinResetEvent event = RoundRobinResetEvent::articulationChange;
    std::uint32_t targetPoolIndex = kInvalidPerformanceProgramIndex; // invalid means all pools
};

struct CompiledPerformanceProgram
{
    std::array<CompiledPerformanceEventRange, 6> eventRanges {};
    std::array<CompiledPerformanceActivation, 128> activationByMidiNote {};
    std::array<std::uint8_t, 128> controllerDefaults {};
    std::array<bool, 128> hasControllerDefault {};
    std::vector<CompiledPerformanceTriggerRoute> triggerRoutes;
    std::vector<CompiledPerformanceRoundRobinReset> roundRobinResets;
    // Stable-ID hashes support allocation-free selection migration between published programs.
    std::vector<std::uint64_t> articulationStableIds;
    // Stable group hashes keep choke targets comparable across activation generations.
    std::vector<std::uint64_t> exclusiveGroupStableIds;
    // Stable pool hashes map compiled reset targets to callback-owned RR cursors.
    std::vector<std::uint64_t> roundRobinPoolStableIds;
    // Indexed by the authored/prepared zone order, this bridges render routes to an
    // articulation without retaining string IDs on the callback path.
    std::vector<std::uint32_t> zoneArticulationIndices;
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
