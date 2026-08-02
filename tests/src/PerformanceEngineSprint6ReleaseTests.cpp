#include "drs/engine/PerformanceLaneState.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

SamplerRenderEvent event(const SamplerRenderEventType type,
                         const std::uint32_t offset,
                         const std::uint8_t note = 60,
                         const float velocity = 1.0f,
                         const float noteOffVelocity = 0.0f)
{
    SamplerRenderEvent result { type, offset, note, velocity };
    result.noteOffVelocity = noteOffVelocity;
    return result;
}

CompiledPerformanceProgram releaseProgram()
{
    CompiledPerformanceProgram program;
    program.articulationCount = 1;
    program.defaultArticulationIndex = 0;
    program.articulationStableIds = { 0x72656c65617365ull };
    program.triggerRoutes = {
        { 0, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::noteOff, PerformanceSustainCondition::any, PerformancePitchSource::eventNote },
        { 1, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::release, PerformanceSustainCondition::pedalUp, PerformancePitchSource::eventNote }
    };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::noteOff)] = { 0, 1 };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::release)] = { 1, 1 };
    return program;
}

void verifyImmediatePhysicalAndEffectiveRelease()
{
    const auto program = releaseProgram();
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 61);
    require(state.normalize(event(SamplerRenderEventType::noteOn, 1, 60, 0.5f), 61, program, scratch),
            "Attack must normalize.");
    scratch.clear();

    require(state.normalize(event(SamplerRenderEventType::noteOff, 7, 60, 0.0f), 61, program, scratch),
            "Physical key-up must normalize.");
    require(scratch.size() == 3
                && scratch.view()[0].type == SamplerRenderEventType::noteOn
                && scratch.view()[0].performanceEvent == PerformanceEventKind::noteOff
                && scratch.view()[1].type == SamplerRenderEventType::noteOff
                && scratch.view()[2].type == SamplerRenderEventType::noteOn
                && scratch.view()[2].performanceEvent == PerformanceEventKind::release
                && scratch.view()[0].sampleOffset == 7 && scratch.view()[2].sampleOffset == 7
                && std::abs(scratch.view()[0].velocity - 0.5f) < 0.01f
                && scratch.view()[0].articulationIndex == 0,
            "Key-up must emit physical note-off work before one effective release with attack-velocity fallback.");
    const auto held = state.getHeldNote(0, 60);
    require(!held.active && held.releaseEmitted && held.activationGeneration == 61,
            "An immediate source release must be emitted once while retaining its originating generation record.");
}

void verifySustainDeferralAndPedalUpFanout()
{
    const auto program = releaseProgram();
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 62);
    state.normalize(event(SamplerRenderEventType::noteOn, 0, 64, 0.8f), 62, program, scratch);
    scratch.clear();
    state.normalize(event(SamplerRenderEventType::pedalDown, 2, 0, 1.0f), 62, program, scratch);
    scratch.clear();

    require(state.normalize(event(SamplerRenderEventType::noteOff, 3, 64, 0.0f, 0.25f), 62, program, scratch),
            "Sustained key-up must normalize.");
    require(scratch.size() == 2
                && scratch.view()[0].performanceEvent == PerformanceEventKind::noteOff
                && scratch.view()[0].sustainPedalDown
                && scratch.view()[1].type == SamplerRenderEventType::noteOff
                && state.getHeldNote(0, 64).active && state.getHeldNote(0, 64).pedalDeferred,
            "Pedal-held key-up must emit physical work but defer the effective release.");

    scratch.clear();
    require(state.normalize(event(SamplerRenderEventType::pedalUp, 11, 0, 0.0f), 62, program, scratch),
            "Pedal-up must normalize.");
    require(scratch.size() == 2
                && scratch.view()[0].type == SamplerRenderEventType::pedalUp
                && scratch.view()[1].type == SamplerRenderEventType::noteOn
                && scratch.view()[1].performanceEvent == PerformanceEventKind::release
                && scratch.view()[1].midiNote == 64
                && !scratch.view()[1].sustainPedalDown
                && std::abs(scratch.view()[1].velocity - 0.25f) < 0.01f,
            "Pedal-up must release the deferred source before emitting its release-route trigger at the same offset.");
    require(!state.getHeldNote(0, 64).active && state.getHeldNote(0, 64).releaseEmitted,
            "Pedal-up must consume the deferred ownership record exactly once.");

    scratch.clear();
    require(state.normalize(event(SamplerRenderEventType::pedalUp, 12, 0, 0.0f), 62, program, scratch)
                && scratch.size() == 0,
            "Repeated pedal-up must not duplicate effective release triggers.");
}

void verifyAllNotesOffDoesNotCreateReleaseRecursion()
{
    const auto program = releaseProgram();
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 63);
    state.normalize(event(SamplerRenderEventType::noteOn, 0, 67, 0.7f), 63, program, scratch);
    scratch.clear();
    require(state.normalize(event(SamplerRenderEventType::allNotesOff, 4), 63, program, scratch)
                && scratch.size() == 1 && scratch.view()[0].type == SamplerRenderEventType::allNotesOff
                && state.getSnapshot().heldNoteCount == 0,
            "Panic all-notes-off must clear held ownership without spawning authored release triggers.");
}
} // namespace

int main()
{
    try
    {
        verifyImmediatePhysicalAndEffectiveRelease();
        verifySustainDeferralAndPedalUpFanout();
        verifyAllNotesOffDoesNotCreateReleaseRecursion();
        std::cout << "Performance-engine Sprint 6 release tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 6 release tests failed: " << exception.what() << '\n';
        return 1;
    }
}
