#include "drs/engine/PerformanceLaneState.h"
#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/SamplerVoicePool.h"

#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

SamplerRenderEvent event(const SamplerRenderEventType type,
                         const std::uint32_t offset,
                         const std::uint8_t note,
                         const float velocity = 1.0f)
{
    return { type, offset, note, velocity };
}

CompiledPerformanceProgram stringsProgram()
{
    CompiledPerformanceProgram program;
    program.articulationCount = 2;
    program.defaultArticulationIndex = 0;
    program.articulationStableIds = { 0x5355535441494eull, 0x535441434341544full };
    // C0 latches Sustain, D0 latches Staccato. Both are consumed key-switches.
    program.activationByMidiNote[24] = { 0, true };
    program.activationByMidiNote[26] = { 1, true };
    return program;
}

SamplerRenderModelPtr stringsModel()
{
    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 5;
    snapshot.contentDigest = "sprint5-keyswitch-snapshot";

    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 501;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint5-keyswitch-prepared";
    prepared.performanceProgram = stringsProgram();
    prepared.performanceProgram.zoneArticulationIndices = { 0, 1 };

    for (std::size_t index = 0; index < 2; ++index)
    {
        const auto articulationId = index == 0 ? "sustain" : "staccato";
        const auto zoneId = std::string("strings-") + articulationId;
        const auto sampleId = std::string("sample-") + articulationId;
        auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float>(64, index == 0 ? 0.25f : 0.75f) };

        PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = zoneId;
        snapshotZone.sampleSourceId = sampleId;
        snapshotZone.displayName = articulationId;
        snapshotZone.groupId = "strings";
        snapshotZone.articulationId = articulationId;
        snapshotZone.rootKey = 60;
        snapshotZone.keyLow = 0;
        snapshotZone.keyHigh = 127;
        snapshotZone.velocityLow = 1;
        snapshotZone.velocityHigh = 127;
        snapshot.zones.push_back(std::move(snapshotZone));

        PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sampleId;
        sample.streamSampleId = sampleId + "-stream";
        sample.sampleRate = 48000.0;
        sample.frameCount = 64;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        PreparedPlaybackZoneHandle zone;
        zone.zoneId = zoneId;
        zone.sampleSourceId = sampleId;
        zone.streamSampleId = sampleId + "-stream";
        zone.preparedSampleIndex = index;
        zone.preparedStreamIndex = index;
        zone.rootKey = 60;
        zone.keyLow = 0;
        zone.keyHigh = 127;
        zone.velocityLow = 1;
        zone.velocityHigh = 127;
        prepared.zones.push_back(std::move(zone));
    }

    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "strings";
    snapshotGroup.articulationIds = { "sustain", "staccato" };
    snapshotGroup.zoneIds = { "strings-sustain", "strings-staccato" };
    snapshot.groupRoutes.push_back(snapshotGroup);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = snapshotGroup.groupId;
    preparedGroup.articulationIds = snapshotGroup.articulationIds;
    preparedGroup.zoneIds = snapshotGroup.zoneIds;
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::performance;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 502;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto result = buildSamplerRenderModel(payload);
    if (!result.built)
    {
        std::string details;
        for (const auto& finding : result.findings)
            details += finding.code + ": " + finding.message + " ";
        throw std::runtime_error("Strings key-switch fixture model must validate: " + details);
    }
    require(result.built && result.model != nullptr, "Strings key-switch fixture model must validate.");
    return result.model;
}

void verifyKeySwitchSelectionAndConsumption()
{
    const auto program = stringsProgram();
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 41);

    require(state.normalize(event(SamplerRenderEventType::noteOn, 5, 24), 41, program, scratch),
            "C0 key-switch note-on must be accepted.");
    require(scratch.size() == 0 && state.getSnapshot().selectedArticulationIndex == 0
                && state.getHeldNote(0, 24).active && state.getHeldNote(0, 24).consumed,
            "C0 must select Sustain without forwarding a sample-starting event.");

    scratch.clear(); // Separate host block.
    require(state.normalize(event(SamplerRenderEventType::noteOff, 2, 24, 0.0f), 41, program, scratch),
            "C0 key-switch note-off must be accepted in a later block.");
    require(scratch.size() == 0 && !state.getHeldNote(0, 24).active,
            "The matching C0 key-switch note-off must remain consumed across blocks.");

    require(state.normalize(event(SamplerRenderEventType::noteOn, 7, 26), 41, program, scratch),
            "D0 key-switch note-on must be accepted.");
    require(scratch.size() == 0 && state.getSnapshot().selectedArticulationIndex == 1
                && state.getHeldNote(0, 26).consumed,
            "D0 must select Staccato without forwarding a sample-starting event.");

    scratch.clear();
    require(state.normalize(event(SamplerRenderEventType::noteOn, 7, 60, 0.8f), 41, program, scratch),
            "Playable note must be accepted after the key-switch.");
    require(scratch.size() == 1 && scratch.view()[0].type == SamplerRenderEventType::noteOn
                && scratch.view()[0].sampleOffset == 7 && scratch.view()[0].articulationIndex == 1
                && state.getHeldNote(0, 60).articulationAtAttack == 1,
            "A playable note must route using the newly latched Staccato articulation at its input offset.");

    scratch.clear();
    require(state.normalize(event(SamplerRenderEventType::noteOff, 1, 26, 0.0f), 41, program, scratch),
            "D0 key-switch note-off must be accepted.");
    require(scratch.size() == 0 && !state.getHeldNote(0, 26).active,
            "The matching D0 key-switch note-off must not leak into voice release routing.");
}

void verifySelectedArticulationReachesVoiceRouting()
{
    const auto model = stringsModel();
    require(model->getRoutes().size() == 2
                && model->getRoutes()[0].performanceArticulationIndex == 0
                && model->getRoutes()[1].performanceArticulationIndex == 1,
            "The prepared zone topology must retain numeric articulation routing identities.");

    SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0, 51), "Voice pool must prepare the key-switch fixture.");
    std::array<float, 16> audio {};
    float* channels[] { audio.data() };
    SamplerRenderEvent playable = event(SamplerRenderEventType::noteOn, 0, 60, 1.0f);
    playable.articulationIndex = 1;
    const auto result = pool.renderBlock({ channels, 1, static_cast<std::uint32_t>(audio.size()) }, { &playable, 1 });
    require(result.render.startedVoiceCount == 1 && std::abs(audio[0] - 0.75f) < 0.0001f,
            "A post-D0 playable note must start only the Staccato route, never the Sustain route (voices="
                + std::to_string(result.render.startedVoiceCount) + ", sample=" + std::to_string(audio[0]) + ").");
}
} // namespace

int main()
{
    try
    {
        verifyKeySwitchSelectionAndConsumption();
        verifySelectedArticulationReachesVoiceRouting();
        std::cout << "Performance-engine Sprint 5 key-switch tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 5 key-switch tests failed: " << exception.what() << '\n';
        return 1;
    }
}
