#include "Sprint4OfflineRenderHarness.h"

#include "drs/engine/SamplerRenderModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef DRS_SPRINT4_OFFLINE_BASELINE_PATH
#error DRS_SPRINT4_OFFLINE_BASELINE_PATH must identify the reviewed offline baseline.
#endif

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(double actual, double expected, double tolerance, const std::string& message)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
    {
        std::ostringstream detail;
        detail << std::setprecision(10) << message
               << " expected=" << expected << " actual=" << actual
               << " tolerance=" << tolerance;
        throw std::runtime_error(detail.str());
    }
}

struct ModelSpec
{
    std::vector<std::vector<float>> channels;
    double sourceSampleRate = 48000.0;
    std::string groupId = "offline-group";
    double groupGainDb = 0.0;
    double groupPan = 0.0;
    int rootKey = 60;
    double gainDb = 0.0;
    double pan = 0.0;
    std::uint64_t sampleStartFrame = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
};

drs::engine::SamplerRenderModelPtr makeGroupedModel(const std::string& id,
                                                    const std::vector<ModelSpec>& specs)
{
    require(!specs.empty(), id + " requires at least one zone.");

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 1;
    snapshot.contentDigest = "offline-snapshot-" + id;

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 1001;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = 1;
    prepared.preparedContentDigest = "offline-prepared-" + id;

    std::map<std::string, std::size_t> groupRouteIndices;
    for (std::size_t index = 0; index < specs.size(); ++index)
    {
        const auto& spec = specs[index];
        require(!spec.channels.empty() && spec.channels.size() <= 2 && !spec.channels.front().empty(),
                id + " requires one or two non-empty sample channels.");
        const auto frameCount = spec.channels.front().size();
        for (const auto& channel : spec.channels)
            require(channel.size() == frameCount, id + " sample channels must have equal frame counts.");

        const auto zoneId = "zone-" + id + "-" + std::to_string(index + 1);
        const auto sampleId = "sample-" + id + "-" + std::to_string(index + 1);
        const auto streamId = "stream-" + id + "-" + std::to_string(index + 1);

        auto groupRoute = groupRouteIndices.find(spec.groupId);
        if (groupRoute == groupRouteIndices.end())
        {
            drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
            snapshotGroup.groupId = spec.groupId;
            snapshotGroup.articulationIds = { "offline-articulation" };
            snapshotGroup.displayName = spec.groupId;
            snapshotGroup.routingSourceId = "groups/" + spec.groupId;
            snapshotGroup.gainDb = spec.groupGainDb;
            snapshotGroup.pan = spec.groupPan;
            groupRoute = groupRouteIndices.emplace(spec.groupId, snapshot.groupRoutes.size()).first;
            snapshot.groupRoutes.push_back(std::move(snapshotGroup));

            drs::engine::PreparedPlaybackGroupRoute preparedGroup;
            preparedGroup.groupId = spec.groupId;
            preparedGroup.articulationIds = { "offline-articulation" };
            preparedGroup.displayName = spec.groupId;
            preparedGroup.routingSourceId = "groups/" + spec.groupId;
            preparedGroup.gainDb = spec.groupGainDb;
            preparedGroup.pan = spec.groupPan;
            prepared.groupRoutes.push_back(std::move(preparedGroup));
        }

        snapshot.groupRoutes[groupRoute->second].zoneIds.push_back(zoneId);
        prepared.groupRoutes[groupRoute->second].zoneIds.push_back(zoneId);

        drs::engine::PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = zoneId;
        snapshotZone.sampleSourceId = sampleId;
        snapshotZone.displayName = zoneId;
        snapshotZone.groupId = spec.groupId;
        snapshotZone.articulationId = "offline-articulation";
        snapshotZone.rootKey = spec.rootKey;
        snapshotZone.keyLow = 0;
        snapshotZone.keyHigh = 127;
        snapshotZone.velocityLow = 1;
        snapshotZone.velocityHigh = 127;
        snapshotZone.gainDb = spec.gainDb;
        snapshotZone.pan = spec.pan;
        snapshotZone.sampleStartFrame = spec.sampleStartFrame;
        snapshotZone.loopEnabled = spec.loopEnabled;
        snapshotZone.loopStartFrame = spec.loopStartFrame;
        snapshotZone.loopEndFrame = spec.loopEndFrame;
        snapshot.zones.push_back(std::move(snapshotZone));

        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = spec.channels;
        drs::engine::PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sampleId;
        sample.streamSampleId = streamId;
        sample.sampleRate = spec.sourceSampleRate;
        sample.frameCount = frameCount;
        sample.channelCount = spec.channels.size();
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        drs::engine::PreparedPlaybackZoneHandle preparedZone;
        preparedZone.zoneId = zoneId;
        preparedZone.sampleSourceId = sampleId;
        preparedZone.streamSampleId = streamId;
        preparedZone.preparedSampleIndex = index;
        preparedZone.preparedStreamIndex = index;
        preparedZone.rootKey = spec.rootKey;
        preparedZone.keyLow = 0;
        preparedZone.keyHigh = 127;
        preparedZone.velocityLow = 1;
        preparedZone.velocityHigh = 127;
        preparedZone.gainDb = spec.gainDb;
        preparedZone.pan = spec.pan;
        preparedZone.sampleStartFrame = spec.sampleStartFrame;
        preparedZone.loopEnabled = spec.loopEnabled;
        preparedZone.loopStartFrame = spec.loopStartFrame;
        preparedZone.loopEndFrame = spec.loopEndFrame;
        prepared.zones.push_back(std::move(preparedZone));
    }

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::performance;
    payload->revision = 1;
    payload->snapshotBuildId = 1001;
    payload->preparedBuildId = 2001;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->retainedPreparedBytes = 0;
    for (const auto& spec : specs)
        payload->retainedPreparedBytes += spec.channels.front().size() * spec.channels.size() * sizeof(float);
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));

    const auto result = drs::engine::buildSamplerRenderModel(payload);
    require(result.built && result.model != nullptr,
            id + " should build a valid immutable offline render model.");
    return result.model;
}

drs::engine::SamplerRenderModelPtr makeModel(const std::string& id, const ModelSpec& spec)
{
    return makeGroupedModel(id, { spec });
}

drs::tests::OfflineTimelineEvent noteOn(std::uint64_t frame, int note = 60, int velocity = 127)
{
    return { frame,
             drs::engine::SamplerRenderEventType::noteOn,
             static_cast<std::uint8_t>(note),
             static_cast<float>(velocity) / 127.0f };
}

drs::tests::OfflineTimelineEvent noteOff(std::uint64_t frame, int note = 60)
{
    return { frame, drs::engine::SamplerRenderEventType::noteOff,
             static_cast<std::uint8_t>(note), 0.0f };
}

drs::tests::OfflineTimelineEvent command(std::uint64_t frame,
                                         drs::engine::SamplerRenderEventType type)
{
    return { frame, type, 0, 0.0f };
}

drs::tests::OfflineRenderArtifact render(const std::string& id,
                                         const ModelSpec& spec,
                                         std::uint64_t frames,
                                         std::vector<drs::tests::OfflineTimelineEvent> events,
                                         std::uint32_t partition = 64)
{
    return drs::tests::renderOffline({ id, makeModel(id, spec), 48000.0, frames,
                                       partition, std::move(events) });
}

void requireFrame(const drs::tests::OfflineRenderArtifact& artifact,
                  std::size_t channel,
                  std::size_t frame,
                  double expected,
                  const std::string& behavior)
{
    require(channel < artifact.channels.size() && frame < artifact.channels[channel].size(),
            behavior + " requested an invalid output frame.");
    requireNear(artifact.channels[channel][frame], expected,
                drs::tests::offlineSampleTolerance, behavior);
}

std::vector<drs::tests::OfflineRenderArtifact> runGoldenBehaviorMatrix()
{
    std::vector<drs::tests::OfflineRenderArtifact> artifacts;

    ModelSpec ramp { { { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } } };
    auto silence = render("silence", ramp, 8, {});
    require(silence.summary.firstNonZeroFrame == -1 && silence.summary.peak == 0.0,
            "Silence timeline should remain exactly silent.");
    artifacts.push_back(std::move(silence));

    ModelSpec constant { { std::vector<float>(32, 1.0f) } };
    auto timing = render("sample-accurate-timing", constant, 12, { noteOn(3) });
    require(timing.summary.firstNonZeroFrame == 3, "Note-on should begin at its exact global frame.");
    requireFrame(timing, 0, 2, 0.0, "Timing pre-roll");
    requireFrame(timing, 0, 3, 1.0, "Timing onset");
    artifacts.push_back(std::move(timing));

    auto mono = render("mono-root-unity", ramp, 7, { noteOn(0) });
    requireFrame(mono, 0, 1, 0.25, "Mono unity-pitch left");
    requireFrame(mono, 1, 1, 0.25, "Mono duplication right");
    artifacts.push_back(std::move(mono));

    ModelSpec stereo { { { 0.2f, 0.4f, 0.6f }, { -0.1f, -0.3f, -0.5f } } };
    auto stereoArtifact = render("stereo-channels", stereo, 5, { noteOn(0) });
    requireFrame(stereoArtifact, 0, 1, 0.4, "Stereo left channel");
    requireFrame(stereoArtifact, 1, 1, -0.3, "Stereo right channel");
    artifacts.push_back(std::move(stereoArtifact));

    ModelSpec startOffset = ramp;
    startOffset.sampleStartFrame = 2;
    auto offset = render("sample-start-offset", startOffset, 5, { noteOn(0) });
    requireFrame(offset, 0, 0, 0.5, "Authored sample start offset");
    requireFrame(offset, 0, 2, 1.0, "Authored sample start progression");
    artifacts.push_back(std::move(offset));

    ModelSpec octave { { { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f } } };
    auto octaveArtifact = render("octave-pitch", octave, 5, { noteOn(0, 72) });
    requireFrame(octaveArtifact, 0, 1, 0.2, "Octave pitch increment");
    requireFrame(octaveArtifact, 0, 3, 0.6, "Octave final-frame traversal");
    artifacts.push_back(std::move(octaveArtifact));

    auto velocity = render("velocity-scaling", constant, 4, { noteOn(0, 60, 64) });
    requireFrame(velocity, 0, 0, 64.0 / 127.0, "Velocity scaling");
    artifacts.push_back(std::move(velocity));

    ModelSpec gain = constant;
    gain.gainDb = -6.0;
    auto gainArtifact = render("gain-scaling", gain, 4, { noteOn(0) });
    requireFrame(gainArtifact, 0, 0, std::pow(10.0, -6.0 / 20.0), "Gain scaling");
    artifacts.push_back(std::move(gainArtifact));

    ModelSpec pan = constant;
    pan.pan = 0.5;
    auto panArtifact = render("pan-balance", pan, 4, { noteOn(0) });
    requireFrame(panArtifact, 0, 0, 0.5, "Positive pan left attenuation");
    requireFrame(panArtifact, 1, 0, 1.0, "Positive pan right preservation");
    artifacts.push_back(std::move(panArtifact));

    const auto grouped = drs::tests::renderOffline(
        { "grouped-mix-balance",
          makeGroupedModel("grouped-mix-balance",
                           {
                               ModelSpec { { std::vector<float>(8, 1.0f) }, 48000.0, "stack", -6.0, 0.5 },
                               ModelSpec { { std::vector<float>(8, 1.0f) }, 48000.0, "stack", -6.0, 0.5 },
                               ModelSpec { { std::vector<float>(8, 1.0f) }, 48000.0, "solo", 0.0, 0.0 }
                           }),
          48000.0,
          4,
          64,
          { noteOn(0) } });
    requireFrame(grouped, 0, 0, 1.0 + 2.0 * std::pow(10.0, -6.0 / 20.0) * 0.5,
                 "Group gain/pan left mix");
    requireFrame(grouped, 1, 0, 1.0 + 2.0 * std::pow(10.0, -6.0 / 20.0),
                 "Group gain/pan right mix");
    artifacts.push_back(std::move(grouped));

    ModelSpec shortSample { { { 1.0f, 0.5f, 0.25f } } };
    auto completion = render("sample-completion", shortSample, 7, { noteOn(0) });
    requireFrame(completion, 0, 2, 0.25, "Final sample frame");
    requireFrame(completion, 0, 3, 0.0, "Post-completion silence");
    require(completion.summary.counters.completedVoiceCount == 1
                && completion.summary.finishedVoiceCount == 1,
            "Sample completion should finish exactly one voice.");
    artifacts.push_back(std::move(completion));

    auto accumulation = render("mixed-accumulation", constant, 4,
                               { noteOn(0), noteOn(0) });
    requireFrame(accumulation, 0, 0, 2.0, "Two-voice mixed accumulation");
    require(accumulation.summary.counters.startedVoiceCount == 2,
            "Mixed accumulation should start two voices.");
    artifacts.push_back(std::move(accumulation));

    ModelSpec loopConstant { { std::vector<float>(32, 1.0f) } };
    loopConstant.loopEnabled = true;
    loopConstant.loopStartFrame = 1;
    loopConstant.loopEndFrame = 32;
    auto release = render("note-off-release", loopConstant, 2060,
                          { noteOn(0), noteOff(4) }, 127);
    requireFrame(release, 0, 4, 1.0, "Release first sample");
    requireFrame(release, 0, 5, 2047.0 / 2048.0, "Release envelope progression");
    requireFrame(release, 0, 2052, 0.0, "Release completion silence");
    require(release.summary.counters.releasedVoiceCount == 1
                && release.summary.counters.completedVoiceCount == 1,
            "Note-off should release and complete one voice.");
    artifacts.push_back(std::move(release));

    ModelSpec loop { { { 0.0f, 0.2f, 0.4f, 0.6f, 0.8f } } };
    loop.loopEnabled = true;
    loop.loopStartFrame = 1;
    loop.loopEndFrame = 4;
    auto loopBoundary = render("loop-boundary", loop, 8, { noteOn(0) });
    const std::array<double, 8> loopExpected { 0.0, 0.2, 0.4, 0.6, 0.2, 0.4, 0.6, 0.2 };
    for (std::size_t frame = 0; frame < loopExpected.size(); ++frame)
        requireFrame(loopBoundary, 0, frame, loopExpected[frame], "Forward loop boundary");
    artifacts.push_back(std::move(loopBoundary));

    auto multipleWraps = render("multiple-loop-wraps", loop, 8, { noteOn(0, 72) });
    const std::array<double, 8> wrapExpected { 0.0, 0.4, 0.2, 0.6, 0.4, 0.2, 0.6, 0.4 };
    for (std::size_t frame = 0; frame < wrapExpected.size(); ++frame)
        requireFrame(multipleWraps, 0, frame, wrapExpected[frame], "Multiple loop wraps");
    artifacts.push_back(std::move(multipleWraps));

    auto polyphony = render("polyphony", loopConstant, 8,
                            { noteOn(0, 60), noteOn(0, 61), noteOn(0, 62) });
    requireFrame(polyphony, 0, 0, 3.0, "Three-voice polyphony");
    require(polyphony.summary.activeVoiceCount == 3,
            "Polyphony scenario should retain three active voices.");
    artifacts.push_back(std::move(polyphony));

    std::vector<drs::tests::OfflineTimelineEvent> stealEvents;
    for (int index = 0; index < 25; ++index)
        stealEvents.push_back(noteOn(0, 48 + index));
    auto stealing = render("voice-stealing", loopConstant, 4, std::move(stealEvents));
    requireFrame(stealing, 0, 0, 24.0, "Fixed-pool post-steal mix");
    require(stealing.summary.counters.startedVoiceCount == 25
                && stealing.summary.counters.stolenVoiceCount == 1
                && stealing.summary.activeVoiceCount == 24,
            "The 25th voice should deterministically steal one of 24 slots.");
    artifacts.push_back(std::move(stealing));

    auto repeated = render("repeated-note-release", loopConstant, 12,
                           { noteOn(0), noteOn(0), noteOff(4) });
    require(repeated.summary.counters.startedVoiceCount == 2
                && repeated.summary.counters.releasedVoiceCount == 2
                && repeated.summary.releasingVoiceCount == 2,
            "One note-off should release every repeated-note owner.");
    artifacts.push_back(std::move(repeated));

    auto allNotesOff = render("all-notes-off", loopConstant, 12,
                              { noteOn(0, 60), noteOn(0, 61), noteOn(0, 62),
                                command(4, drs::engine::SamplerRenderEventType::allNotesOff) });
    require(allNotesOff.summary.counters.releasedVoiceCount == 3
                && allNotesOff.summary.releasingVoiceCount == 3,
            "All-notes-off should release every active voice.");
    artifacts.push_back(std::move(allNotesOff));

    auto reset = render("emergency-reset", loopConstant, 8,
                        { noteOn(0), noteOn(0, 61),
                          command(3, drs::engine::SamplerRenderEventType::reset) });
    requireFrame(reset, 0, 2, 2.0, "Reset precondition mix");
    requireFrame(reset, 0, 3, 0.0, "Emergency reset boundary");
    require(reset.summary.counters.resetVoiceCount == 2
                && reset.summary.activeVoiceCount == 0
                && reset.summary.releasingVoiceCount == 0,
            "Emergency reset should synchronously clear both voices.");
    artifacts.push_back(std::move(reset));

    return artifacts;
}

drs::tests::OfflineRenderArtifact runPartitionInvarianceMatrix()
{
    ModelSpec spec { { { 0.0f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f, 0.75f,
                           0.6f, 0.45f, 0.3f, 0.15f, 0.0f, -0.15f, -0.3f, -0.45f,
                           -0.6f, -0.45f, -0.3f, -0.15f } } };
    spec.loopEnabled = true;
    spec.loopStartFrame = 2;
    spec.loopEndFrame = 18;
    const std::vector<drs::tests::OfflineTimelineEvent> events {
        noteOn(5, 60), noteOn(37, 64), noteOn(91, 60), noteOff(333, 60),
        noteOn(511, 67, 96), command(1025, drs::engine::SamplerRenderEventType::allNotesOff),
        noteOn(2047, 72), command(3073, drs::engine::SamplerRenderEventType::reset),
        noteOn(4091, 55), noteOff(4503, 55)
    };
    const auto model = makeModel("partition-invariance", spec);
    const auto makeRequest = [&](std::uint32_t partition)
    {
        return drs::tests::OfflineRenderRequest {
            "partition-invariance", model, 48000.0, 5000, partition, events
        };
    };

    const auto reference = drs::tests::renderOffline(makeRequest(32));
    const std::array<std::uint32_t, 5> otherPartitions { 64, 127, 256, 512, 1024 };
    for (const auto partition : otherPartitions)
    {
        const auto candidate = drs::tests::renderOffline(makeRequest(partition));
        const auto comparison = drs::tests::compareOfflineArtifacts(reference, candidate);
        if (!comparison.equivalent)
        {
            const auto failureDirectory = fs::current_path() / "sprint4-offline-render-failures";
            drs::tests::writeOfflineMismatchArtifacts(failureDirectory,
                                                      reference,
                                                      candidate,
                                                      comparison);
            throw std::runtime_error("Partition " + std::to_string(partition)
                                     + " diverged from partition 32: " + comparison.message
                                     + " Failure artifacts: " + failureDirectory.generic_string());
        }
    }
    return reference;
}

struct BaselineSummary
{
    std::uint64_t frames = 0;
    std::string checksum;
    double peak = 0.0;
    double rms = 0.0;
    std::int64_t first = -1;
    std::int64_t last = -1;
    std::array<std::uint64_t, 13> lifecycle {};
};

std::array<std::uint64_t, 13> lifecycleValues(const drs::tests::OfflineRenderSummary& summary)
{
    const auto& value = summary.counters;
    return { value.renderedBlockCount, value.startedVoiceCount, value.releasedVoiceCount,
             value.completedVoiceCount, value.stolenVoiceCount, value.droppedEventCount,
             value.resetVoiceCount, value.appliedActivationCount, value.enqueuedRetirementCount,
             value.reclaimedActivationCount, summary.activeVoiceCount,
             summary.releasingVoiceCount, summary.finishedVoiceCount };
}

std::vector<std::string> split(const std::string& text, char delimiter)
{
    std::vector<std::string> values;
    std::istringstream input(text);
    std::string value;
    while (std::getline(input, value, delimiter))
        values.push_back(value);
    return values;
}

std::map<std::string, BaselineSummary> loadBaselines()
{
    std::ifstream input(DRS_SPRINT4_OFFLINE_BASELINE_PATH, std::ios::binary);
    require(input.good(), "Could not open the reviewed Sprint 4.7 baseline manifest.");
    std::map<std::string, BaselineSummary> baselines;
    auto schemaAccepted = false;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.rfind("# drs.sprint4.offline-render-baseline", 0) == 0)
        {
            schemaAccepted = line.find("version=1") != std::string::npos
                && line.find("sampleTolerance=1e-06") != std::string::npos
                && line.find("checksumQuantum=1e-07") != std::string::npos;
            continue;
        }
        if (line.empty() || line.front() == '#')
            continue;
        const auto columns = split(line, '|');
        require(columns.size() == 8, "Malformed Sprint 4.7 baseline row: " + line);
        BaselineSummary value;
        value.frames = std::stoull(columns[1]);
        value.checksum = columns[2];
        value.peak = std::stod(columns[3]);
        value.rms = std::stod(columns[4]);
        value.first = std::stoll(columns[5]);
        value.last = std::stoll(columns[6]);
        const auto lifecycle = split(columns[7], ',');
        require(lifecycle.size() == value.lifecycle.size(),
                "Malformed lifecycle counters in baseline row: " + line);
        for (std::size_t index = 0; index < lifecycle.size(); ++index)
            value.lifecycle[index] = std::stoull(lifecycle[index]);
        require(baselines.emplace(columns[0], std::move(value)).second,
                "Duplicate Sprint 4.7 baseline scenario: " + columns[0]);
    }
    require(schemaAccepted,
            "Sprint 4.7 baseline schema, version, or numeric contract is unsupported.");
    return baselines;
}

void emitBaselines(const std::vector<drs::tests::OfflineRenderArtifact>& artifacts)
{
    std::cout << "# drs.sprint4.offline-render-baseline|version=1|sampleTolerance="
              << drs::tests::offlineSampleTolerance
              << "|checksumQuantum=" << drs::tests::offlineChecksumQuantum << '\n';
    std::cout << "# scenario|frames|checksum|peak|rms|firstNonZero|lastNonZero|"
                 "rendered,started,released,completed,stolen,dropped,reset,applied,enqueued,reclaimed,active,releasing,finished\n";
    std::cout << std::setprecision(17);
    for (const auto& artifact : artifacts)
    {
        const auto& summary = artifact.summary;
        std::cout << artifact.scenarioId << '|' << summary.frameCount << '|'
                  << summary.quantizedChecksum << '|' << summary.peak << '|' << summary.rms << '|'
                  << summary.firstNonZeroFrame << '|' << summary.lastNonZeroFrame << '|';
        const auto lifecycle = lifecycleValues(summary);
        for (std::size_t index = 0; index < lifecycle.size(); ++index)
            std::cout << (index == 0 ? "" : ",") << lifecycle[index];
        std::cout << '\n';
    }
}

void verifyBaselines(const std::vector<drs::tests::OfflineRenderArtifact>& artifacts)
{
    const auto baselines = loadBaselines();
    require(baselines.size() == artifacts.size(),
            "Reviewed baseline scenario count does not match the offline conformance matrix.");
    for (const auto& artifact : artifacts)
    {
        const auto iterator = baselines.find(artifact.scenarioId);
        require(iterator != baselines.end(), "Missing reviewed baseline: " + artifact.scenarioId);
        const auto& expected = iterator->second;
        const auto& actual = artifact.summary;
        const auto mismatch = expected.frames != actual.frameCount
            || expected.checksum != actual.quantizedChecksum
            || std::abs(expected.peak - actual.peak) > drs::tests::offlineSummaryTolerance
            || std::abs(expected.rms - actual.rms) > drs::tests::offlineSummaryTolerance
            || expected.first != actual.firstNonZeroFrame
            || expected.last != actual.lastNonZeroFrame
            || expected.lifecycle != lifecycleValues(actual);
        if (mismatch)
        {
            const auto directory = fs::current_path() / "sprint4-offline-render-failures";
            fs::create_directories(directory);
            std::ofstream output(directory / (artifact.scenarioId + "-baseline-mismatch.txt"),
                                 std::ios::binary);
            output << "Reviewed baseline mismatch for " << artifact.scenarioId << '\n'
                   << "expected checksum=" << expected.checksum << '\n'
                   << "actual checksum=" << actual.quantizedChecksum << '\n'
                   << std::setprecision(17)
                   << "expected peak=" << expected.peak << " rms=" << expected.rms << '\n'
                   << "actual peak=" << actual.peak << " rms=" << actual.rms << '\n';
            throw std::runtime_error("Reviewed baseline mismatch for " + artifact.scenarioId
                                     + ". Failure artifact: " + directory.generic_string());
        }
    }
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        auto artifacts = runGoldenBehaviorMatrix();
        artifacts.push_back(runPartitionInvarianceMatrix());
        if (argc == 2 && std::string(argv[1]) == "--emit-baselines")
        {
            emitBaselines(artifacts);
            return 0;
        }
        require(argc == 1, "Only --emit-baselines is supported.");
        verifyBaselines(artifacts);
        std::cout << "Sprint 4.7 deterministic offline render conformance passed: "
                  << artifacts.size() << " reviewed scenarios and partitions 32/64/127/256/512/1024."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 4.7 offline conformance failed: " << exception.what() << std::endl;
        return 1;
    }
}
