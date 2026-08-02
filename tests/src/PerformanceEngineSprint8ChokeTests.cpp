#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/SamplerVoicePool.h"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace drs::engine;

constexpr std::uint64_t openHatGroup = 0x6f70656e2d686174ull;
constexpr std::uint64_t closedHatGroup = 0x636c6f7365642d68ull;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

SamplerRenderModelPtr hiHatModel()
{
    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 8;
    snapshot.contentDigest = "sprint8-hihat-snapshot";
    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 801;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint8-hihat-prepared";

    const std::array<std::string, 2> names { "open-hat", "closed-hat" };
    const std::array<int, 2> notes { 60, 62 };
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = names[index];
        snapshotZone.sampleSourceId = names[index] + "-sample";
        snapshotZone.displayName = names[index];
        snapshotZone.groupId = "hats";
        snapshotZone.articulationId = "kit";
        snapshotZone.rootKey = notes[index];
        snapshotZone.keyLow = notes[index];
        snapshotZone.keyHigh = notes[index];
        snapshotZone.velocityLow = 1;
        snapshotZone.velocityHigh = 127;
        snapshotZone.triggerMode = index == 0 ? ZoneTriggerMode::gated : ZoneTriggerMode::oneShot;
        snapshot.zones.push_back(std::move(snapshotZone));

        auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float>(4096, index == 0 ? 0.5f : 0.75f) };
        PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = names[index] + "-sample";
        sample.streamSampleId = names[index] + "-stream";
        sample.sampleRate = 48000.0;
        sample.frameCount = 4096;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        PreparedPlaybackZoneHandle zone;
        zone.zoneId = names[index];
        zone.sampleSourceId = names[index] + "-sample";
        zone.streamSampleId = names[index] + "-stream";
        zone.preparedSampleIndex = index;
        zone.preparedStreamIndex = index;
        zone.rootKey = notes[index];
        zone.keyLow = notes[index];
        zone.keyHigh = notes[index];
        zone.velocityLow = 1;
        zone.velocityHigh = 127;
        zone.triggerMode = index == 0 ? ZoneTriggerMode::gated : ZoneTriggerMode::oneShot;
        prepared.zones.push_back(std::move(zone));
    }

    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "hats";
    snapshotGroup.articulationIds = { "kit" };
    snapshotGroup.zoneIds = { "open-hat", "closed-hat" };
    snapshot.groupRoutes.push_back(snapshotGroup);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = snapshotGroup.groupId;
    preparedGroup.articulationIds = snapshotGroup.articulationIds;
    preparedGroup.zoneIds = snapshotGroup.zoneIds;
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    prepared.performanceProgram.articulationCount = 1;
    prepared.performanceProgram.defaultArticulationIndex = 0;
    prepared.performanceProgram.articulationStableIds = { 0x6b6974ull };
    prepared.performanceProgram.exclusiveGroupCount = 2;
    prepared.performanceProgram.exclusiveGroupStableIds = { openHatGroup, closedHatGroup };
    prepared.performanceProgram.zoneArticulationIndices = { 0, 0 };
    prepared.performanceProgram.triggerRoutes = {
        { 0, 0, 0, 0, 0.0f,
          PerformanceEventKind::noteOn, PerformanceSustainCondition::any, PerformancePitchSource::eventNote },
        { 1, 0, 1, 1, 0.002f,
          PerformanceEventKind::noteOn, PerformanceSustainCondition::any, PerformancePitchSource::eventNote }
    };
    prepared.performanceProgram.eventRanges[static_cast<std::size_t>(PerformanceEventKind::noteOn)] = { 0, 2 };

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::performance;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 802;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto result = buildSamplerRenderModel(payload);
    require(result.built && result.model != nullptr, "Hi-hat choke fixture must validate.");
    return result.model;
}

SamplerRenderEvent noteOn(const int note)
{
    SamplerRenderEvent event { SamplerRenderEventType::noteOn, 0, static_cast<std::uint8_t>(note), 1.0f };
    event.articulationIndex = 0;
    event.performanceEvent = PerformanceEventKind::noteOn;
    return event;
}

void verifyChokeAndOneShotLifecycle()
{
    const auto model = hiHatModel();
    SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0, 81), "Hi-hat pool must prepare.");
    std::array<float, 32> audio {};
    float* channels[] { audio.data() };

    const auto open = noteOn(60);
    const auto openResult = pool.renderBlock({ channels, 1, static_cast<std::uint32_t>(audio.size()) }, { &open, 1 });
    require(openResult.render.startedVoiceCount == 1 && pool.getSlotSnapshot(0).state == SamplerVoiceSlotState::active,
            "Open hi-hat must start an active source voice.");

    const auto closed = noteOn(62);
    const auto closeResult = pool.renderBlock({ channels, 1, static_cast<std::uint32_t>(audio.size()) }, { &closed, 1 });
    require(closeResult.render.startedVoiceCount == 1 && closeResult.render.chokedVoiceCount == 1
                && pool.getSlotSnapshot(0).state == SamplerVoiceSlotState::releasing
                && pool.getSlotSnapshot(0).sustainDeferred == false
                && pool.getSlotSnapshot(1).state == SamplerVoiceSlotState::active,
            "Closed hi-hat must choke the pre-existing open voice before allocating its own one-shot.");

    SamplerRenderEvent closedOff { SamplerRenderEventType::noteOff, 0, 62, 0.0f };
    const auto noteOffResult = pool.renderBlock({ channels, 1, static_cast<std::uint32_t>(audio.size()) }, { &closedOff, 1 });
    require(noteOffResult.render.releasedVoiceCount == 0 && pool.getSlotSnapshot(1).state == SamplerVoiceSlotState::active,
            "A one-shot pedal/mechanics-style source must ignore ordinary note-off after it starts.");
}
} // namespace

int main()
{
    try
    {
        verifyChokeAndOneShotLifecycle();
        std::cout << "Performance-engine Sprint 8 choke tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 8 choke tests failed: " << exception.what() << '\n';
        return 1;
    }
}
