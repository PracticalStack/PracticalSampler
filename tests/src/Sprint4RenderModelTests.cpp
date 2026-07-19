#include "drs/engine/SamplerRenderModel.h"

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct RenderModelFixture
{
    drs::engine::PlaybackActivationLane lane = drs::engine::PlaybackActivationLane::preview;
    drs::engine::PlaybackSnapshotLifecycleState lifecycle
        = drs::engine::PlaybackSnapshotLifecycleState::ready;
    bool activationEligible = true;
    std::size_t revision = 7;
    std::uint64_t snapshotBuildId = 101;
    std::uint64_t preparedBuildId = 202;
    std::string snapshotDigest = "snapshot-digest-7";
    std::string preparedDigest = "prepared-digest-7";
    drs::engine::ImmutablePlaybackSnapshot snapshot;
    drs::engine::ImmutablePreparedPlayback prepared;

    RenderModelFixture()
    {
        snapshot.draftRevision = revision;
        snapshot.contentDigest = snapshotDigest;
        snapshot.zones.push_back({ "zone-a",
                                   "sample-a",
                                   "Zone A",
                                   "group-a",
                                   "sustain",
                                   60,
                                   36,
                                   84,
                                   1,
                                   127,
                                   -3.0,
                                   0.25,
                                   1,
                                   true,
                                   2,
                                   6 });

        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = {
            { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f },
            { 0.0f, -0.1f, -0.2f, -0.3f, -0.4f, -0.5f, -0.6f, -0.7f }
        };

        drs::engine::PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = "sample-a";
        sample.streamSampleId = "stream-a";
        sample.sampleRate = 48000.0;
        sample.frameCount = 8;
        sample.channelCount = 2;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        prepared.snapshotBuildId = snapshotBuildId;
        prepared.snapshotContentDigest = snapshotDigest;
        prepared.draftRevision = revision;
        prepared.preparedContentDigest = preparedDigest;
        prepared.zones.push_back({ "zone-a",
                                   "sample-a",
                                   "stream-a",
                                   0,
                                   0,
                                   60,
                                   36,
                                   84,
                                   1,
                                   127,
                                   -3.0,
                                   0.25,
                                   1,
                                   true,
                                   2,
                                   6 });
    }

    drs::engine::PlaybackActivationPayloadPtr makePayload(bool includeSnapshot = true,
                                                          bool includePrepared = true) const
    {
        auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
        payload->lane = lane;
        payload->revision = revision;
        payload->snapshotBuildId = snapshotBuildId;
        payload->preparedBuildId = preparedBuildId;
        payload->lifecycleState = lifecycle;
        payload->activationEligible = activationEligible;
        payload->snapshotContentDigest = snapshotDigest;
        payload->preparedContentDigest = preparedDigest;
        if (includeSnapshot)
            payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(snapshot);
        if (includePrepared)
            payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(prepared);
        return payload;
    }
};

bool hasFinding(const drs::engine::SamplerRenderModelBuildResult& result,
                const std::string& code)
{
    for (const auto& finding : result.findings)
        if (finding.code == code)
            return true;
    return false;
}

void expectRejected(const std::string& expectedCode,
                    const std::function<void(RenderModelFixture&)>& mutate)
{
    RenderModelFixture fixture;
    mutate(fixture);
    const auto result = drs::engine::buildSamplerRenderModel(fixture.makePayload());
    require(!result.built && result.model == nullptr && hasFinding(result, expectedCode),
            "Expected render-model rejection code: " + expectedCode);
}

void runImmutableModelContract()
{
    static_assert(!std::is_default_constructible_v<drs::engine::SamplerRenderModel>,
                  "Render models must be created only through the validating factory.");
    static_assert(std::is_const_v<drs::engine::SamplerRenderModelPtr::element_type>,
                  "Renderer consumers must receive a const model.");

    RenderModelFixture fixture;
    auto payload = fixture.makePayload();
    std::weak_ptr<const drs::engine::PlaybackActivationPayload> weakPayload = payload;
    const auto decodedIdentity = payload->prepared->samples.front().decodedSampleData.get();
    auto result = drs::engine::buildSamplerRenderModel(payload);
    require(result.built && result.findings.empty() && result.model != nullptr,
            "Coherent Preview activation should build an immutable render model.");
    require(result.model->getLane() == drs::engine::PlaybackActivationLane::preview
                && result.model->getRevision() == 7
                && result.model->getSnapshotBuildId() == 101
                && result.model->getPreparedBuildId() == 202
                && result.model->getSnapshotContentDigest() == fixture.snapshotDigest
                && result.model->getPreparedContentDigest() == fixture.preparedDigest,
            "Render model must preserve activation identity without mutable aggregate access.");
    require(result.model->getSamples().size() == 1
                && result.model->getSamples().front().decodedSampleData.get() == decodedIdentity
                && result.model->getSamples().front().channelCount == 2
                && result.model->getSamples().front().frameCount == 8,
            "Render model must retain prepared PCM without copying decoded channels.");
    require(result.model->getRoutes().size() == 1
                && result.model->getRoutes().front().zoneId == "zone-a"
                && result.model->getRoutes().front().preparedSampleIndex == 0
                && result.model->getRoutes().front().loopEnabled
                && result.model->getRoutes().front().loopStartFrame == 2
                && result.model->getRoutes().front().loopEndFrame == 6,
            "Render model must expose normalized, prevalidated renderer topology.");

    payload.reset();
    require(!weakPayload.expired()
                && result.model->getRetainedActivationPayload() != nullptr,
            "The immutable model must retain its complete activation lifetime.");
    result.model.reset();
    require(weakPayload.expired(),
            "Activation ownership should release after the final non-audio model owner releases it.");

    RenderModelFixture performanceFixture;
    performanceFixture.lane = drs::engine::PlaybackActivationLane::performance;
    performanceFixture.lifecycle = drs::engine::PlaybackSnapshotLifecycleState::active;
    const auto performance = drs::engine::buildSamplerRenderModel(performanceFixture.makePayload());
    require(performance.built
                && performance.model->getLane() == drs::engine::PlaybackActivationLane::performance,
            "Active Performance payload should use the same immutable model boundary.");
}

void runNonOwningViewContract()
{
    std::array<float, 16> left {};
    std::array<float, 16> right {};
    float* channels[] { left.data(), right.data() };
    drs::engine::SamplerAudioBufferView output { channels, 2, 16 };
    require(output.isValid(), "A complete non-owning stereo output view should be valid.");

    float* invalidChannels[] { left.data(), nullptr };
    require(!drs::engine::SamplerAudioBufferView { invalidChannels, 2, 16 }.isValid()
                && !drs::engine::SamplerAudioBufferView { nullptr, 2, 16 }.isValid()
                && !drs::engine::SamplerAudioBufferView { channels, 0, 16 }.isValid()
                && !drs::engine::SamplerAudioBufferView { channels, 2, 0 }.isValid(),
            "Output views must reject missing storage and zero dimensions.");

    const std::array<drs::engine::SamplerRenderEvent, 2> events {{
        { drs::engine::SamplerRenderEventType::noteOn, 3, 60, 0.75f },
        { drs::engine::SamplerRenderEventType::noteOff, 11, 60, 0.0f }
    }};
    const drs::engine::SamplerRenderEventView eventView { events.data(), events.size() };
    require(eventView.isValid() && eventView.size == 2
                && eventView[0].sampleOffset == 3
                && eventView[1].type == drs::engine::SamplerRenderEventType::noteOff
                && drs::engine::SamplerRenderEventView { nullptr, 0 }.isValid()
                && !drs::engine::SamplerRenderEventView { nullptr, 1 }.isValid(),
            "Event view must remain non-owning and distinguish empty from missing storage.");

    const drs::engine::SamplerRenderRequest request { output, eventView, 48000.0 };
    const drs::engine::SamplerRenderResult result { 16, 2, 1, 1, 0, 0, 0 };
    require(request.output.isValid() && request.events.isValid()
                && request.outputSampleRate == 48000.0
                && result.renderedFrameCount == 16
                && result.consumedEventCount == 2,
            "Render request/result primitives should require no owning callback containers.");
}

void runPayloadIdentityFailures()
{
    const auto missing = drs::engine::buildSamplerRenderModel(nullptr);
    require(!missing.built && hasFinding(missing, "render-model-payload-missing"),
            "Missing activation payload must be actionable.");

    RenderModelFixture fixture;
    fixture.activationEligible = false;
    auto result = drs::engine::buildSamplerRenderModel(fixture.makePayload());
    require(!result.built && hasFinding(result, "render-model-payload-ineligible"),
            "Ineligible activation must not build a model.");

    fixture = RenderModelFixture {};
    result = drs::engine::buildSamplerRenderModel(fixture.makePayload(false, true));
    require(!result.built && hasFinding(result, "render-model-snapshot-missing"),
            "Missing snapshot must be actionable.");
    result = drs::engine::buildSamplerRenderModel(fixture.makePayload(true, false));
    require(!result.built && hasFinding(result, "render-model-prepared-missing"),
            "Missing prepared resources must be actionable.");

    expectRejected("render-model-lifecycle-invalid", [](RenderModelFixture& value)
    {
        value.lifecycle = drs::engine::PlaybackSnapshotLifecycleState::failed;
    });
    expectRejected("render-model-revision-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.draftRevision += 1;
    });
    expectRejected("render-model-snapshot-build-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.snapshotBuildId += 1;
    });
    expectRejected("render-model-snapshot-digest-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.snapshotContentDigest = "wrong-snapshot";
    });
    expectRejected("render-model-prepared-digest-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.preparedContentDigest = "wrong-prepared";
    });
}

void runSampleTopologyFailures()
{
    expectRejected("render-model-sample-rate-invalid", [](RenderModelFixture& value)
    {
        value.prepared.samples[0].sampleRate = 0.0;
    });
    expectRejected("render-model-channel-count-unsupported", [](RenderModelFixture& value)
    {
        value.prepared.samples[0].channelCount = 3;
    });
    expectRejected("render-model-decoded-data-missing", [](RenderModelFixture& value)
    {
        value.prepared.samples[0].decodedSampleData.reset();
    });
    expectRejected("render-model-channel-layout-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.samples[0].channelCount = 1;
    });
    expectRejected("render-model-channel-frames-truncated", [](RenderModelFixture& value)
    {
        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>(
            *value.prepared.samples[0].decodedSampleData);
        decoded->normalizedChannels[1].resize(3);
        value.prepared.samples[0].decodedSampleData = std::move(decoded);
    });
}

void runRouteTopologyFailures()
{
    expectRejected("render-model-sample-index-invalid", [](RenderModelFixture& value)
    {
        value.prepared.zones[0].preparedSampleIndex = 8;
    });
    expectRejected("render-model-zone-sample-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.zones[0].sampleSourceId = "other-sample";
    });
    expectRejected("render-model-root-key-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].rootKey = 128;
        value.prepared.zones[0].rootKey = 128;
    });
    expectRejected("render-model-key-range-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].keyLow = 90;
        value.prepared.zones[0].keyLow = 90;
    });
    expectRejected("render-model-velocity-range-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].velocityLow = 0;
        value.prepared.zones[0].velocityLow = 0;
    });
    expectRejected("render-model-gain-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].gainDb = std::numeric_limits<double>::infinity();
        value.prepared.zones[0].gainDb = std::numeric_limits<double>::infinity();
    });
    expectRejected("render-model-pan-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].pan = 1.5;
        value.prepared.zones[0].pan = 1.5;
    });
    expectRejected("render-model-start-frame-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].sampleStartFrame = 8;
        value.prepared.zones[0].sampleStartFrame = 8;
    });
    expectRejected("render-model-loop-range-invalid", [](RenderModelFixture& value)
    {
        value.snapshot.zones[0].loopEndFrame = 9;
        value.prepared.zones[0].loopEndFrame = 9;
    });
    expectRejected("render-model-route-topology-mismatch", [](RenderModelFixture& value)
    {
        value.prepared.zones[0].pan = -0.5;
    });
    expectRejected("render-model-zone-id-duplicate", [](RenderModelFixture& value)
    {
        value.snapshot.zones.push_back(value.snapshot.zones[0]);
        value.prepared.zones.push_back(value.prepared.zones[0]);
    });
}
} // namespace

int main()
{
    try
    {
        runImmutableModelContract();
        runNonOwningViewContract();
        runPayloadIdentityFailures();
        runSampleTopologyFailures();
        runRouteTopologyFailures();
        std::cout << "Sprint 4.1 immutable render-model boundary matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
