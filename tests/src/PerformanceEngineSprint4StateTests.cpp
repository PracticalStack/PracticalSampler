#include "drs/engine/PerformanceLaneState.h"

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

SamplerRenderEvent event(const SamplerRenderEventType type, const std::uint32_t offset,
                         const std::uint8_t note = 60, const float velocity = 1.0f,
                         const std::uint8_t channel = 0, const float noteOffVelocity = 0.0f)
{
    SamplerRenderEvent result { type, offset, note, velocity };
    result.midiChannel = channel;
    result.noteOffVelocity = noteOffVelocity;
    return result;
}

CompiledPerformanceProgram makeProgram()
{
    CompiledPerformanceProgram program;
    program.articulationCount = 2;
    program.defaultArticulationIndex = 1;
    program.articulationStableIds = { 0xabcdu, 0x1234u };
    return program;
}

void verifyStateVectors()
{
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    const auto program = makeProgram();
    state.migrateProgram(program, 7);
    require(state.getSnapshot().selectedArticulationIndex == 1, "New lanes must choose the compiled default articulation.");
    require(state.normalize(event(SamplerRenderEventType::noteOn, 3, 61, 0.75f, 2), 7, program, scratch), "Note-on must normalize.");
    const auto attack = state.getHeldNote(2, 61);
    require(attack.active && attack.physicalKeyDown && attack.attackVelocity == 95
                && attack.articulationAtAttack == 1 && attack.activationGeneration == 7,
            "Held notes must retain attack velocity, articulation, channel, and generation.");
    require(state.normalize(event(SamplerRenderEventType::sustainPedal, 5, 0, 1.0f, 2), 7, program, scratch), "CC64 down must normalize.");
    require(state.normalize(event(SamplerRenderEventType::sustainPedal, 6, 0, 1.0f, 2), 7, program, scratch), "Repeated CC64 is valid.");
    require(state.normalize(event(SamplerRenderEventType::noteOff, 7, 61, 0.0f, 2, 0.5f), 7, program, scratch), "Note-off must normalize.");
    const auto release = state.getHeldNote(2, 61);
    require(!release.physicalKeyDown && release.pedalDeferred && release.noteOffVelocity == 64,
            "Held records must retain physical release and pedal deferral without a release trigger.");
    require(state.normalize(event(SamplerRenderEventType::sustainPedal, 9, 0, 0.0f, 2), 7, program, scratch), "CC64 up must normalize.");
    require(scratch.size() == 4 && scratch.view()[0].sampleOffset == 3
                && scratch.view()[1].type == SamplerRenderEventType::pedalDown
                && scratch.view()[1].sampleOffset == 5
                && scratch.view()[2].type == SamplerRenderEventType::noteOff
                && scratch.view()[3].type == SamplerRenderEventType::pedalUp
                && scratch.view()[3].sampleOffset == 9,
            "Normalization must preserve accepted ordering and offsets while suppressing duplicate CC64 edges.");
    require(!state.getHeldNote(2, 61).pedalDeferred
                && state.getSnapshot().semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::pedalDown)] == 1
                && state.getSnapshot().semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::pedalUp)] == 1,
            "Pedal transitions must be emitted exactly once.");
}

void verifyIsolationMigrationAndOverflow()
{
    PerformanceLaneState preview, performance;
    PerformanceActionScratch previewScratch, performanceScratch;
    const auto program = makeProgram();
    preview.migrateProgram(program, 10);
    performance.migrateProgram(program, 20);
    preview.normalize(event(SamplerRenderEventType::noteOn, 0, 60), 10, program, previewScratch);
    performance.normalize(event(SamplerRenderEventType::sustainPedal, 0, 0, 1.0f), 20, program, performanceScratch);
    require(preview.getSnapshot().heldNoteCount == 1 && !preview.getSnapshot().pedalDown
                && performance.getSnapshot().heldNoteCount == 0 && performance.getSnapshot().pedalDown,
            "Preview and Performance mutable state must be isolated.");
    auto replacement = makeProgram();
    replacement.defaultArticulationIndex = 0;
    replacement.articulationStableIds = { 0x1234u, 0x7777u };
    preview.migrateProgram(replacement, 30);
    require(preview.getSnapshot().selectedArticulationIndex == 0
                && preview.getHeldNote(0, 60).activationGeneration == 10,
            "Publish migration must map selection by stable ID and retain held-note generation.");
    PerformanceLaneState overflow;
    PerformanceActionScratch scratch;
    overflow.migrateProgram(makeProgram(), 1);
    for (std::size_t index = 0; index < PerformanceActionScratch::capacity; ++index)
        require(overflow.normalize(event(SamplerRenderEventType::noteOn, static_cast<std::uint32_t>(index),
                                         static_cast<std::uint8_t>(index & 0x7f)), 1, program, scratch),
                "Scratch must accept its exact capacity.");
    require(!overflow.normalize(event(SamplerRenderEventType::noteOn, 127, 127), 1, program, scratch)
                && overflow.getSnapshot().actionOverflowCount == 1
                && overflow.getSnapshot().heldNoteCount == PerformanceLaneState::noteCount,
            "Overflow must reject an entire event without partial state.");
}
} // namespace

int main()
{
    try
    {
        verifyStateVectors();
        verifyIsolationMigrationAndOverflow();
        std::cout << "Performance-engine Sprint 4 state tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 4 state tests failed: " << exception.what() << '\n';
        return 1;
    }
}
