#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/SamplerRenderModel.h"
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

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(const float actual, const float expected, const std::string& message)
{
    if (std::abs(actual - expected) > 1.0e-5f)
        throw std::runtime_error(message + " (actual=" + std::to_string(actual)
                                 + ", expected=" + std::to_string(expected) + ")");
}

struct StereoOutput
{
    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> pointers;

    explicit StereoOutput(const std::size_t frames)
        : left(frames, 0.0f), right(frames, 0.0f), pointers { left.data(), right.data() }
    {
    }

    SamplerAudioBufferView view()
    {
        return { pointers.data(), 2, static_cast<std::uint32_t>(left.size()) };
    }
};

SamplerRenderEvent noteOn(const std::uint32_t offset, const int velocity)
{
    SamplerRenderEvent event;
    event.type = SamplerRenderEventType::noteOn;
    event.sampleOffset = offset;
    event.midiNote = 60;
    event.velocity = static_cast<float>(velocity) / 127.0f;
    return event;
}

SamplerRenderEvent controllerChange(const std::uint32_t offset,
                                    const std::uint8_t controller,
                                    const std::uint8_t value)
{
    SamplerRenderEvent event;
    event.type = SamplerRenderEventType::controllerChange;
    event.sampleOffset = offset;
    event.controllerNumber = controller;
    event.controllerValue = value;
    return event;
}

struct LayerFixture
{
    ImmutablePlaybackSnapshot snapshot;
    ImmutablePreparedPlayback prepared;
    PlaybackActivationPayloadPtr payload;
};

LayerFixture makeFixture(const RuntimeProjectLayerCrossfadeDefinition crossfade,
                         const double layerGainDb = 0.0,
                         const double layerPan = 0.0)
{
    LayerFixture fixture;
    fixture.snapshot.draftRevision = 1;
    fixture.snapshot.contentDigest = "layer-runtime-snapshot";
    fixture.snapshot.masterGainDb = 0.0;
    fixture.prepared.snapshotBuildId = 11;
    fixture.prepared.snapshotContentDigest = fixture.snapshot.contentDigest;
    fixture.prepared.draftRevision = fixture.snapshot.draftRevision;
    fixture.prepared.preparedContentDigest = "layer-runtime-prepared";
    fixture.snapshot.selectedLayerId = "layer-a";
    fixture.prepared.selectedLayerId = "layer-a";

    PlaybackSnapshotLayerRoute snapshotLayer;
    snapshotLayer.layerId = "layer-a";
    snapshotLayer.displayName = "Layer A";
    snapshotLayer.routingSourceId = "layers/layer-a";
    snapshotLayer.gainDb = layerGainDb;
    snapshotLayer.pan = layerPan;
    snapshotLayer.crossfade = crossfade;
    PreparedPlaybackLayerRoute preparedLayer;
    preparedLayer.layerId = snapshotLayer.layerId;
    preparedLayer.displayName = snapshotLayer.displayName;
    preparedLayer.routingSourceId = snapshotLayer.routingSourceId;
    preparedLayer.gainDb = snapshotLayer.gainDb;
    preparedLayer.pan = snapshotLayer.pan;
    preparedLayer.crossfade = snapshotLayer.crossfade;

    const std::array groups { std::string("group-a"), std::string("group-b") };
    const std::array values { 1.0f, 2.0f };
    for (std::size_t index = 0; index < groups.size(); ++index)
    {
        const auto sampleId = "layer-sample-" + std::to_string(index);
        const auto zoneId = "layer-zone-" + std::to_string(index);

        PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = zoneId;
        snapshotZone.sampleSourceId = sampleId;
        snapshotZone.displayName = zoneId;
        snapshotZone.groupId = groups[index];
        snapshotZone.articulationId = "sustain";
        snapshotZone.rootKey = 60;
        snapshotZone.keyLow = 60;
        snapshotZone.keyHigh = 60;
        snapshotZone.velocityLow = 1;
        snapshotZone.velocityHigh = 127;
        fixture.snapshot.zones.push_back(snapshotZone);

        auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float> { values[index] } };
        PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sampleId;
        sample.streamSampleId = "layer-stream-" + std::to_string(index);
        sample.sampleRate = 48000.0;
        sample.frameCount = 1;
        sample.channelCount = 1;
        sample.decodedSampleData = decoded;
        fixture.prepared.samples.push_back(sample);

        PreparedPlaybackZoneHandle preparedZone;
        preparedZone.zoneId = zoneId;
        preparedZone.sampleSourceId = sampleId;
        preparedZone.streamSampleId = sample.streamSampleId;
        preparedZone.preparedSampleIndex = index;
        preparedZone.rootKey = 60;
        preparedZone.keyLow = 60;
        preparedZone.keyHigh = 60;
        preparedZone.velocityLow = 1;
        preparedZone.velocityHigh = 127;
        fixture.prepared.zones.push_back(preparedZone);

        PlaybackSnapshotGroupRoute snapshotGroup;
        snapshotGroup.groupId = groups[index];
        snapshotGroup.displayName = "Group " + std::to_string(index);
        snapshotGroup.routingSourceId = "groups/" + groups[index];
        snapshotGroup.layerId = snapshotLayer.layerId;
        snapshotGroup.zoneIds = { zoneId };
        fixture.snapshot.groupRoutes.push_back(snapshotGroup);

        PreparedPlaybackGroupRoute preparedGroup;
        preparedGroup.groupId = snapshotGroup.groupId;
        preparedGroup.displayName = snapshotGroup.displayName;
        preparedGroup.routingSourceId = snapshotGroup.routingSourceId;
        preparedGroup.layerId = snapshotGroup.layerId;
        preparedGroup.zoneIds = snapshotGroup.zoneIds;
        fixture.prepared.groupRoutes.push_back(preparedGroup);

        snapshotLayer.groupIds.push_back(groups[index]);
        snapshotLayer.zoneIds.push_back(zoneId);
        preparedLayer.groupIds.push_back(groups[index]);
        preparedLayer.zoneIds.push_back(zoneId);
    }

    fixture.snapshot.layerRoutes.push_back(snapshotLayer);
    fixture.prepared.layerRoutes.push_back(preparedLayer);
    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::preview;
    payload->revision = 1;
    payload->snapshotBuildId = fixture.prepared.snapshotBuildId;
    payload->preparedBuildId = 12;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = fixture.snapshot.contentDigest;
    payload->preparedContentDigest = fixture.prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(fixture.snapshot);
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(fixture.prepared);
    fixture.payload = std::move(payload);
    return fixture;
}

SamplerRenderModelPtr buildModel(const RuntimeProjectLayerCrossfadeDefinition crossfade,
                                 const double layerGainDb = 0.0,
                                 const double layerPan = 0.0)
{
    auto fixture = makeFixture(crossfade, layerGainDb, layerPan);
    const auto result = buildSamplerRenderModel(fixture.payload);
    require(result.built && result.model != nullptr, "Layer runtime fixture must build a render model.");
    require(result.model->getRoutes().size() == 2, "Layer runtime fixture must retain both group routes.");
    for (const auto& route : result.model->getRoutes())
        require(route.layerId == "layer-a", "Every child route must retain its authored layer identity.");
    return result.model;
}

void verifyRouteComposition()
{
    const auto model = buildModel({}, 6.0, 0.25);
    requireNear(static_cast<float>(model->getRoutes().front().gainDb), 6.0f,
                "Layer gain must compose independently into the render route.");
    requireNear(static_cast<float>(model->getRoutes().front().pan), 0.25f,
                "Layer pan must compose independently into the render route.");
}

void verifyControllerCrossfade()
{
    RuntimeProjectLayerCrossfadeDefinition crossfade;
    crossfade.source = LayerCrossfadeSource::controller;
    crossfade.controllerNumber = 1;
    crossfade.low = 0;
    crossfade.high = 127;
    const auto model = buildModel(crossfade);

    SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Layer crossfade pool must prepare.");
    const std::array events { controllerChange(0, 1, 64), noteOn(1, 127) };
    StereoOutput output(2);
    const auto result = pool.renderBlock(output.view(), { events.data(), events.size() });
    require(result.accepted && result.render.startedVoiceCount == 2,
            "Controller layer crossfade must start all child groups in the layer.");
    requireNear(output.left[1], 3.0f * (64.0f / 127.0f),
                "Controller layer crossfade must weight all child group routes equally.");
}

void verifyVelocityCrossfade()
{
    RuntimeProjectLayerCrossfadeDefinition crossfade;
    crossfade.source = LayerCrossfadeSource::velocity;
    crossfade.low = 32;
    crossfade.high = 96;
    const auto model = buildModel(crossfade);
    SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Velocity layer crossfade pool must prepare.");
    const std::array events { noteOn(0, 64) };
    StereoOutput output(1);
    const auto result = pool.renderBlock(output.view(), { events.data(), events.size() });
    require(result.accepted && result.render.startedVoiceCount == 2,
            "Velocity layer crossfade must start all child groups in the layer.");
    requireNear(output.left.front(), 1.5f * (64.0f / 127.0f),
                "Velocity layer crossfade must apply its bounded linear weight.");
}

void verifyLayerGraph()
{
    ImmutablePlaybackSnapshot snapshot;
    snapshot.dspGraphDigest = "layer-graph";
    snapshot.layerRoutes.push_back({ "layer-a", { "group-a" }, {}, "Layer A", 0,
                                      "layers/layer-a", true, 0.0, 0.0, {}, {}, {} });
    snapshot.groupRoutes.push_back({ "group-a", {}, {}, "Group A", 0, "groups/group-a", true,
                                     0.0, 0.0, {}, {}, "layer-a" });
    PlaybackSnapshotFxSlotReference slot;
    slot.id = "layer-gain";
    slot.effectType = "drs.gain";
    slot.effectVersion = 1;
    slot.catalogResolved = true;
    slot.parameters = { { "gainDb", 0.0 } };
    slot.cost.costUnits = 1;
    snapshot.fxSlots.push_back(slot);
    snapshot.routingBuses.push_back({ "layer-bus", "Layer", "layers/layer-a", { "layer-gain" } });
    const auto result = compileDspGraphPlan(snapshot);
    require(result.compiled && result.plan.nodes.size() == 1,
            "Layer-owned DSP must compile as a first-class graph owner.");
    require(result.plan.nodes.front().ownerKind == DspGraphOwnerKind::layer
                && result.plan.nodes.front().outputDestinationId == "master",
            "Layer-owned DSP must route from the layer to master.");
}

void verifyScopedRetention()
{
    RuntimeProjectLayerCrossfadeDefinition crossfade;
    crossfade.source = LayerCrossfadeSource::controller;
    crossfade.controllerNumber = 1;
    auto fixture = makeFixture(crossfade);
    PlaybackSnapshotBuildResult source;
    source.built = true;
    source.activationEligible = true;
    source.lifecycleState = PlaybackSnapshotLifecycleState::ready;
    source.snapshot = fixture.snapshot;
    PlaybackPreparationScopeRequest request;
    request.scope = PlaybackPreparationScope::selectedGroup;
    request.selectedGroupId = "group-a";
    const auto scoped = scopePlaybackSnapshotForPreparation(source, request);
    require(scoped.built && scoped.snapshot.zones.size() == 2,
            "Selected-group preparation must retain sibling groups in an active layer crossfade.");
    require(scoped.snapshot.layerRoutes.size() == 1
                && scoped.snapshot.layerRoutes.front().zoneIds.size() == 2,
            "Scoped preparation must retain the containing layer's complete membership.");
}
} // namespace

int main()
{
    try
    {
        verifyRouteComposition();
        verifyControllerCrossfade();
        verifyVelocityCrossfade();
        verifyLayerGraph();
        verifyScopedRetention();
        std::cout << "Layer runtime tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Layer runtime tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
