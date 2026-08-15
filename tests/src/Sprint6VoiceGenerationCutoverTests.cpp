#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "drs/engine/SamplerRenderModel.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct ModelFixture
{
    drs::engine::SamplerRenderModelPtr model;
    std::weak_ptr<const drs::engine::PreparedPlaybackDecodedSampleData> decodedLifetime;
};

ModelFixture buildModel(drs::engine::PlaybackActivationLane lane,
                        std::size_t revision,
                        int rootKey,
                        int keyLow,
                        int keyHigh,
                        double gainDb,
                        bool loopEnabled,
                        float sampleValue)
{
    using namespace drs::engine;
    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = revision;
    snapshot.contentDigest = "s6.6-snapshot-" + std::to_string(revision);
    PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "zone-" + std::to_string(revision);
    snapshotZone.sampleSourceId = "sample-" + std::to_string(revision);
    snapshotZone.displayName = "Generation Zone";
    snapshotZone.groupId = "main";
    snapshotZone.articulationId = "sustain";
    snapshotZone.rootKey = rootKey;
    snapshotZone.keyLow = keyLow;
    snapshotZone.keyHigh = keyHigh;
    snapshotZone.gainDb = gainDb;
    snapshotZone.loopEnabled = loopEnabled;
    snapshotZone.loopEndFrame = 8192;
    snapshot.zones.push_back(std::move(snapshotZone));
    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "main";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.zoneIds = { "zone-" + std::to_string(revision) };
    snapshotGroup.displayName = "Main";
    snapshot.groupRoutes.push_back(std::move(snapshotGroup));

    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::vector<float>(8192, sampleValue) };
    const std::weak_ptr<const PreparedPlaybackDecodedSampleData> decodedLifetime = decoded;

    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 6600 + revision * 2;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = revision;
    prepared.preparedContentDigest = "s6.6-prepared-" + std::to_string(revision);
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "sample-" + std::to_string(revision);
    sample.streamSampleId = "stream-" + std::to_string(revision);
    sample.sampleRate = 48000.0;
    sample.frameCount = 8192;
    sample.channelCount = 1;
    sample.decodedSampleData = decoded;
    prepared.samples.push_back(std::move(sample));
    PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = "zone-" + std::to_string(revision);
    preparedZone.sampleSourceId = "sample-" + std::to_string(revision);
    preparedZone.streamSampleId = "stream-" + std::to_string(revision);
    preparedZone.rootKey = rootKey;
    preparedZone.keyLow = keyLow;
    preparedZone.keyHigh = keyHigh;
    preparedZone.gainDb = gainDb;
    preparedZone.loopEnabled = loopEnabled;
    preparedZone.loopEndFrame = 8192;
    prepared.zones.push_back(std::move(preparedZone));
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "main";
    preparedGroup.articulationIds = { "sustain" };
    preparedGroup.zoneIds = { "zone-" + std::to_string(revision) };
    preparedGroup.displayName = "Main";
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = lane;
    payload->revision = revision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = prepared.snapshotBuildId + 1;
    payload->lifecycleState = lane == PlaybackActivationLane::performance
        ? PlaybackSnapshotLifecycleState::active
        : PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->routeDigest = "s6.6-route-" + std::to_string(revision);
    payload->sourceProvenanceDigest = "s6.6-source-" + std::to_string(revision);
    payload->macroSchemaDigest = "s6.6-macro";
    payload->retainedPreparedBytes = 8192 * sizeof(float);
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto built = buildSamplerRenderModel(payload);
    require(built.built && built.model != nullptr, "Generation fixture model must validate.");
    return { built.model, decodedLifetime };
}

struct StereoOutput
{
    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> channels;

    explicit StereoOutput(std::size_t frames)
        : left(frames, 0.0f), right(frames, 0.0f), channels { left.data(), right.data() }
    {
    }

    drs::engine::SamplerAudioBufferView view()
    {
        return { channels.data(), 2, static_cast<std::uint32_t>(left.size()) };
    }
};

drs::engine::SamplerRenderEvent noteOn(std::uint32_t offset, int note)
{
    return { drs::engine::SamplerRenderEventType::noteOn,
             offset, static_cast<std::uint8_t>(note), 1.0f };
}

drs::engine::SamplerRenderEvent noteOff(std::uint32_t offset, int note)
{
    return { drs::engine::SamplerRenderEventType::noteOff,
             offset, static_cast<std::uint8_t>(note), 0.0f };
}

drs::engine::SamplerRenderEvent sustain(std::uint32_t offset, bool down)
{
    return { drs::engine::SamplerRenderEventType::sustainPedal,
             offset, 0, down ? 1.0f : 0.0f };
}

drs::engine::SamplerRenderEvent command(drs::engine::SamplerRenderEventType type,
                                        std::uint32_t offset = 0)
{
    return { type, offset, 0, 0.0f };
}

template <std::size_t Size>
drs::engine::SamplerRenderEventView view(const std::array<drs::engine::SamplerRenderEvent, Size>& events)
{
    return { events.data(), events.size() };
}

const drs::engine::SamplerVoiceSlotSnapshot* findVoice(
    const drs::engine::SamplerVoicePool& pool,
    int note,
    std::array<drs::engine::SamplerVoiceSlotSnapshot, drs::engine::SamplerVoicePool::capacity>& storage)
{
    for (std::size_t index = 0; index < storage.size(); ++index)
    {
        storage[index] = pool.getSlotSnapshot(index);
        if ((storage[index].state == drs::engine::SamplerVoiceSlotState::active
             || storage[index].state == drs::engine::SamplerVoiceSlotState::releasing)
            && storage[index].sourceMidiNote == note)
        {
            return &storage[index];
        }
    }
    return nullptr;
}

void runHeldVoiceAndSustainMatrix()
{
    using namespace drs::engine;
    SamplerPlaybackContext context(PlaybackActivationLane::performance);
    require(context.prepare(48000.0), "Performance generation context must prepare.");
    auto oldModel = buildModel(PlaybackActivationLane::performance, 1, 48, 48, 48,
                               -12.0, true, 0.25f);
    require(context.stageActivation(oldModel.model), "Old generation must stage.");
    const std::array startOld { noteOn(0, 48) };
    StereoOutput oldOutput(8);
    require(context.renderBlock(oldOutput.view(), view(startOld)).activationApplied,
            "Old generation must activate before its first note.");
    const auto oldGeneration = context.getSnapshot().activeActivationGeneration;

    auto newModel = buildModel(PlaybackActivationLane::performance, 2, 72, 60, 60,
                               0.0, false, 1.0f);
    require(context.stageActivation(newModel.model), "New generation must stage.");
    const std::array retrigger { noteOn(0, 60) };
    StereoOutput cutoverOutput(8);
    const auto cutover = context.renderBlock(cutoverOutput.view(), view(retrigger));
    const auto cutoverSnapshot = context.getSnapshot();
    require(cutover.activationApplied
                && cutoverSnapshot.activeActivationGeneration != oldGeneration
                && cutoverSnapshot.activeGenerationVoiceCount == 1
                && cutoverSnapshot.retiredGenerationVoiceCount == 1,
            "Activation-sample retrigger must use the new generation while the held voice keeps the old generation.");

    std::array<SamplerVoiceSlotSnapshot, SamplerVoicePool::capacity> voices {};
    const auto* oldVoice = findVoice(context.getVoicePool(), 48, voices);
    require(oldVoice != nullptr
                && oldVoice->activationGeneration == oldGeneration
                && oldVoice->modelRevision == 1
                && oldVoice->loopActive
                && std::abs(oldVoice->baseGain
                            - static_cast<float>(std::pow(10.0, -12.0 / 20.0))) < 1.0e-6f,
            "Removed old route must retain its original generation, loop, pitch, gain, and sample handle.");
    const auto* newVoice = findVoice(context.getVoicePool(), 60, voices);
    require(newVoice != nullptr
                && newVoice->activationGeneration == cutoverSnapshot.activeActivationGeneration
                && newVoice->modelRevision == 2
                && !newVoice->loopActive
                && newVoice->baseGain > oldVoice->baseGain
                && std::abs(newVoice->incrementFrames - oldVoice->incrementFrames) > 0.49,
            "New notes must use only the changed route parameters from the active generation.");
    require(cutoverOutput.left.front() > oldOutput.left.front(),
            "Changed new-generation gain must be audible without mutating the held voice.");

    oldModel.model.reset();
    require(!oldModel.decodedLifetime.expired(),
            "Removed route/sample ownership must remain leased by the held old-generation voice.");
    const std::array deferOld { sustain(0, true), noteOff(0, 48) };
    StereoOutput deferredOutput(8);
    const auto deferred = context.renderBlock(deferredOutput.view(), view(deferOld));
    require(deferred.voicePool.render.releasedVoiceCount == 0
                && context.getSnapshot().sustainDeferredVoiceCount == 1,
            "Sustain must defer the note-off against the voice's original generation.");

    const std::array releasePedal { sustain(0, false) };
    StereoOutput pedalUpOutput(1);
    const auto pedalUp = context.renderBlock(pedalUpOutput.view(), view(releasePedal));
    require(pedalUp.voicePool.render.releasedVoiceCount == 1
                && context.getSnapshot().releasingVoiceCount == 1,
            "Sustain release must start the old-generation release tail exactly once.");
    const std::array<SamplerRenderEvent, 0> noEvents {};
    StereoOutput releaseTail(SamplerVoice::compatibilityReleaseSampleCount);
    context.renderBlock(releaseTail.view(), view(noEvents));
    require(!oldModel.decodedLifetime.expired() && context.serviceRetirements() == 1
                && context.waitForBackgroundReclamation()
                && oldModel.decodedLifetime.expired(),
            "Old route/sample ownership must reclaim only after its release tail, off audio.");

    const std::array stopAll { command(SamplerRenderEventType::allNotesOff) };
    StereoOutput stopOutput(1);
    const auto stopped = context.renderBlock(stopOutput.view(), view(stopAll));
    require(stopped.voicePool.render.releasedVoiceCount == 1,
            "All-notes-off must release the remaining active Performance generation.");
}

bool containsVoiceId(const drs::engine::SamplerVoicePool& pool, std::uint64_t voiceId)
{
    for (std::size_t index = 0; index < drs::engine::SamplerVoicePool::capacity; ++index)
        if (pool.getSlotSnapshot(index).voiceId == voiceId)
            return true;
    return false;
}

void renderOne(drs::engine::SamplerPlaybackContext& context,
               drs::engine::SamplerRenderEvent event)
{
    const std::array events { event };
    StereoOutput output(1);
    require(context.renderBlock(output.view(), view(events)).accepted,
            "Bounded generation render must be accepted.");
}

void runCapacityAndRapidCutoverMatrix()
{
    using namespace drs::engine;
    SamplerPlaybackContext context(PlaybackActivationLane::performance);
    require(context.prepare(48000.0), "Capacity context must prepare.");
    auto generationA = buildModel(PlaybackActivationLane::performance, 10, 60, 0, 127,
                                  0.0, true, 1.0f);
    require(context.stageActivation(generationA.model), "Capacity generation A must stage.");
    SamplerEventBlock fill;
    for (std::size_t index = 0; index < SamplerVoicePool::capacity; ++index)
        require(fill.push(noteOn(0, 36 + static_cast<int>(index))),
                "Fixed pool fill event must remain bounded.");
    StereoOutput fillOutput(1);
    require(context.renderBlock(fillOutput.view(), fill.view()).voicePool.activeVoiceCount
                == SamplerVoicePool::capacity,
            "One generation must fill but never exceed the fixed Performance pool.");
    renderOne(context, noteOff(0, 36));
    require(context.getSnapshot().releasingVoiceCount == 1 && containsVoiceId(context.getVoicePool(), 1),
            "Capacity fixture requires one preserved release tail.");

    auto generationB = buildModel(PlaybackActivationLane::performance, 11, 60, 0, 127,
                                  0.0, true, 0.8f);
    require(context.stageActivation(generationB.model), "Capacity generation B must stage.");
    const std::array startB { noteOn(0, 72) };
    StereoOutput outputB(1);
    const auto resultB = context.renderBlock(outputB.view(), view(startB));
    require(resultB.activationApplied
                && resultB.voicePool.render.stolenVoiceCount == 1
                && resultB.voicePool.render.generationStealCount == 1
                && resultB.voicePool.render.releasingVoiceStealCount == 0
                && containsVoiceId(context.getVoicePool(), 1)
                && !containsVoiceId(context.getVoicePool(), 2)
                && context.getSnapshot().activeVoiceCount + context.getSnapshot().releasingVoiceCount
                    == SamplerVoicePool::capacity,
            "Cross-generation pressure must steal the oldest retired active voice before a release tail.");

    auto generationC = buildModel(PlaybackActivationLane::performance, 12, 60, 0, 127,
                                  0.0, true, 0.6f);
    auto generationD = buildModel(PlaybackActivationLane::performance, 13, 60, 0, 127,
                                  0.0, true, 0.4f);
    auto generationE = buildModel(PlaybackActivationLane::performance, 14, 60, 0, 127,
                                  0.0, true, 0.2f);
    require(context.stageActivation(generationC.model), "Rapid generation C must stage.");
    renderOne(context, noteOn(0, 73));
    require(context.stageActivation(generationD.model), "Rapid generation D must stage.");
    renderOne(context, noteOn(0, 74));
    require(!context.stageActivation(generationE.model),
            "A fifth leased generation must be rejected by the fixed activation-slot bound.");

    renderOne(context, command(SamplerRenderEventType::reset));
    require(context.serviceRetirements() == 3
                && context.stageActivation(generationE.model),
            "After voice cleanup and message-owned reclamation, the next rapid generation may stage.");
    renderOne(context, noteOn(0, 75));
    require(context.getSnapshot().activeRevision == 14
                && context.getSnapshot().activeVoiceCount == 1,
            "Rapid cutover must finish on the newest accepted generation.");
}

void runPreviewIsolationMatrix()
{
    using namespace drs::engine;
    SamplerPlaybackContext performance(PlaybackActivationLane::performance);
    SamplerPlaybackContext preview(PlaybackActivationLane::preview);
    auto performanceModel = buildModel(PlaybackActivationLane::performance, 20, 60, 0, 127,
                                       0.0, true, 1.0f);
    auto previewModel = buildModel(PlaybackActivationLane::preview, 21, 60, 0, 127,
                                   0.0, true, 1.0f);
    require(performance.prepare(48000.0) && preview.prepare(48000.0)
                && performance.stageActivation(performanceModel.model)
                && preview.stageActivation(previewModel.model),
            "Isolation contexts must prepare independently.");
    renderOne(performance, noteOn(0, 60));
    renderOne(preview, noteOn(0, 60));
    renderOne(performance, command(SamplerRenderEventType::reset));
    require(preview.getSnapshot().activeVoiceCount == 1
                && preview.getSnapshot().activeActivationGeneration != 0,
            "Performance generation stop/reset must never touch Preview voices or ownership.");
}

void runHostMidiSustainMatrix()
{
    const auto project = drs::engine::loadPhase2ReferenceProjectManifest();
    require(project.loaded, "Host MIDI generation coverage requires the reference project.");
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 64);
    processor.replaceAuthoringProject(project.project);
    processor.serviceMessageThreadWork();
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            "Host MIDI generation coverage must finish bootstrap preparation.");
    processor.serviceMessageThreadWork();
    require(processor.getEngineFacade().publishCurrentDraft(),
            "Host MIDI generation coverage requires one explicit Performance Publish.");
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            "Host MIDI generation coverage must finish Performance preparation.");
    processor.serviceMessageThreadWork();

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;
    for (int attempt = 0; attempt < 32; ++attempt)
    {
        const auto current = processor.getRealtimeSafetySnapshot();
        if (current.activeActivationPayloadBytes > 0
            && current.performanceActiveGeneration != 0)
            break;
        buffer.clear();
        processor.processBlock(buffer, midi);
        processor.serviceMessageThreadWork();
    }
    const auto bootstrap = processor.getRealtimeSafetySnapshot();
    if (bootstrap.activeActivationPayloadBytes == 0
        || bootstrap.performanceActiveGeneration == 0)
    {
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        std::cerr << "Published generation diagnostics: activeBytes="
                  << bootstrap.activeActivationPayloadBytes
                  << ", pendingBytes=" << bootstrap.pendingActivationPayloadBytes
                  << ", performanceActivations=" << bootstrap.performanceActivationCount
                  << ", controllerPreparation="
                  << static_cast<int>(controller.preparationState)
                  << ", controllerActivation="
                  << static_cast<int>(controller.activationState)
                  << ", failure=" << controller.failureFinding.code << std::endl;
    }
    require(bootstrap.activeActivationPayloadBytes > 0
                && bootstrap.performanceActiveGeneration != 0,
            "Host MIDI generation coverage requires an audible published generation.");

    midi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(110)), 0);
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 1);
    midi.addEvent(juce::MidiMessage::noteOff(1, 57), 2);
    buffer.clear();
    processor.processBlock(buffer, midi);
    auto diagnostics = processor.getRealtimeSafetySnapshot();
    if (diagnostics.performanceActiveGeneration == 0
        || diagnostics.performanceActiveGenerationVoiceCount != 1
        || diagnostics.performanceSustainDeferredVoiceCount != 1)
    {
        std::cerr << "Host sustain diagnostics: generation="
                  << diagnostics.performanceActiveGeneration
                  << ", activeGenerationVoices="
                  << diagnostics.performanceActiveGenerationVoiceCount
                  << ", activeVoices=" << diagnostics.performanceActiveVoiceCount
                  << ", deferred=" << diagnostics.performanceSustainDeferredVoiceCount
                  << ", activeRevision=" << diagnostics.activePublishedRevision
                  << ", activeBytes=" << diagnostics.activeActivationPayloadBytes
                  << std::endl;
    }
    require(diagnostics.performanceActiveGeneration != 0
                && diagnostics.performanceActiveGenerationVoiceCount == 1
                && diagnostics.performanceSustainDeferredVoiceCount == 1,
            "Host CC64 must defer note-off on the exact active Performance generation.");

    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
    buffer.clear();
    processor.processBlock(buffer, midi);
    diagnostics = processor.getRealtimeSafetySnapshot();
    if (diagnostics.performanceSustainDeferredVoiceCount != 0
        || diagnostics.performanceActiveVoiceCount != 1
        || diagnostics.performancePeakReleasingVoiceCount < 1)
    {
        std::cerr << "Host pedal-up diagnostics: active="
                  << diagnostics.performanceActiveVoiceCount
                  << ", deferred=" << diagnostics.performanceSustainDeferredVoiceCount
                  << ", peakReleasing=" << diagnostics.performancePeakReleasingVoiceCount
                  << std::endl;
    }
    require(diagnostics.performanceSustainDeferredVoiceCount == 0
                && diagnostics.performanceActiveVoiceCount == 1
                && diagnostics.performancePeakReleasingVoiceCount >= 1,
            "Host sustain release must enter the owning Performance generation's release tail.");

    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 123, 0), 1);
    buffer.clear();
    processor.processBlock(buffer, midi);
    diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.performanceSustainDeferredVoiceCount == 0
                && diagnostics.performancePeakReleasingVoiceCount >= 2,
            "Host all-notes-off must release active voices across Performance generations.");
}
} // namespace

int main()
{
    try
    {
        runHeldVoiceAndSustainMatrix();
        runCapacityAndRapidCutoverMatrix();
        runPreviewIsolationMatrix();
        runHostMidiSustainMatrix();
        std::cout << "Mini Sprint 6.6 held-note and voice-generation cutover matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.6 voice-generation cutover matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
