#include "drs/engine/InstrumentControlBinding.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoicePool.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

namespace instrument_control_allocation_probe
{
thread_local bool enabled = false;
thread_local std::size_t allocations = 0;
thread_local std::size_t deallocations = 0;
}

void* operator new(const std::size_t size)
{
    if (auto* pointer = std::malloc(size == 0 ? 1 : size))
    {
        if (instrument_control_allocation_probe::enabled)
            ++instrument_control_allocation_probe::allocations;
        return pointer;
    }
    throw std::bad_alloc();
}

void* operator new[](const std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept
{
    if (pointer != nullptr && instrument_control_allocation_probe::enabled)
        ++instrument_control_allocation_probe::deallocations;
    std::free(pointer);
}
void operator delete[](void* pointer) noexcept { ::operator delete(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { ::operator delete(pointer); }

namespace
{
void require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

float renderGainCase(const std::uint8_t firstValue,
                     const std::uint8_t secondValue,
                     const drs::engine::RuntimeInstrumentControlTargetKind targetKind
                         = drs::engine::RuntimeInstrumentControlTargetKind::gain,
                     drs::engine::SamplerPanGains* panGains = nullptr,
                     double* tuningCents = nullptr,
                     double* envelopeDecaySeconds = nullptr)
{
    using namespace drs::engine;
    ImmutablePlaybackSnapshot snapshot;
    snapshot.schemaName = "drs.playback.snapshot";
    snapshot.schemaVersion = 1;
    snapshot.contentDigest = "instrument-control-audio";
    snapshot.draftRevision = 1;
    snapshot.masterGainDb = 0.0;

    for (const auto& descriptor : std::array<std::pair<const char*, int>, 2> {
             std::pair { "gain.a", 20 }, std::pair { "gain.b", 21 } })
    {
        RuntimeProjectInstrumentControlDefinition control;
        control.id = descriptor.first;
        control.displayName = descriptor.first;
        control.category = RuntimeInstrumentControlCategory::mixer;
        control.normalizedDefault = 1.0;
        snapshot.instrumentControls.push_back(control);

        ImmutablePlaybackSnapshot::InstrumentControlValue value;
        value.id = control.id;
        value.normalizedValue = 1.0;
        snapshot.instrumentControlValues.push_back(value);

        RuntimeProjectMidiControlBindingDefinition binding;
        binding.id = std::string("binding.") + descriptor.first;
        binding.controlId = control.id;
        binding.controllerNumber = descriptor.second;
        snapshot.midiControlBindings.push_back(binding);

        RuntimeProjectInstrumentControlTargetDefinition target;
        target.id = std::string("target.") + descriptor.first;
        target.controlId = control.id;
        target.targetKind = targetKind;
        target.contributionMode = RuntimeInstrumentControlContributionMode::multiply;
        target.sourceMinimum = 0.0;
        target.sourceMaximum = 1.0;
        target.destinationMinimum = 0.0;
        target.destinationMaximum = 1.0;
        if (targetKind == RuntimeInstrumentControlTargetKind::pan)
        {
            target.destinationMinimum = -1.0;
            target.destinationMaximum = 1.0;
        }
        else if (targetKind == RuntimeInstrumentControlTargetKind::tune)
        {
            target.destinationMinimum = -100.0;
            target.destinationMaximum = 100.0;
        }
        snapshot.instrumentControlTargets.push_back(target);
    }

    snapshot.instrumentControlValues.shrink_to_fit();
    snapshot.instrumentControls.shrink_to_fit();
    snapshot.instrumentControlTargets.shrink_to_fit();

    PlaybackSnapshotZone zone;
    zone.id = "audio-zone";
    zone.sampleSourceId = "audio-sample";
    zone.displayName = "Audio fixture";
    zone.groupId = "audio-group";
    zone.rootKey = zone.keyLow = zone.keyHigh = 60;
    zone.velocityLow = 1;
    zone.velocityHigh = 127;
    snapshot.zones.push_back(zone);
    PlaybackSnapshotGroupRoute group;
    group.groupId = "audio-group";
    group.zoneIds = { zone.id };
    group.displayName = "Audio group";
    snapshot.groupRoutes.push_back(group);

    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::vector<float>(8, 1.0f) };
    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 1;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.preparedContentDigest = "instrument-control-audio-prepared";
    prepared.draftRevision = snapshot.draftRevision;
    prepared.instrumentControls = snapshot.instrumentControls;
    prepared.instrumentControlValues = snapshot.instrumentControlValues;
    prepared.instrumentControlTargets = snapshot.instrumentControlTargets;
    prepared.midiControlBindings = snapshot.midiControlBindings;
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "audio-sample";
    sample.streamSampleId = "audio-stream";
    sample.sampleRate = 48000.0;
    sample.frameCount = 8;
    sample.channelCount = 1;
    sample.decodedSampleData = decoded;
    prepared.samples.push_back(sample);
    PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = "audio-zone";
    preparedZone.sampleSourceId = sample.sampleSourceId;
    preparedZone.streamSampleId = sample.streamSampleId;
    preparedZone.preparedSampleIndex = 0;
    preparedZone.rootKey = preparedZone.keyLow = preparedZone.keyHigh = 60;
    preparedZone.velocityLow = 1;
    preparedZone.velocityHigh = 127;
    prepared.zones.push_back(preparedZone);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = group.groupId;
    preparedGroup.zoneIds = group.zoneIds;
    preparedGroup.displayName = group.displayName;
    prepared.groupRoutes.push_back(preparedGroup);

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::preview;
    payload->revision = 1;
    payload->snapshotBuildId = 1;
    payload->preparedBuildId = 2;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(snapshot);
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(prepared);
    const auto built = buildSamplerRenderModel(payload);
    if (!(built.built && built.model != nullptr))
    {
        std::string details = "Instrument-control audio fixture must build.";
        for (const auto& finding : built.findings)
            details += " [" + finding.code + ": " + finding.message + "]";
        throw std::runtime_error(details);
    }
    require(built.model->getRoutes().size() == 1
                && built.model->getRoutes().front().controlContributions.size() == 2,
            "Instrument-control audio fixture must retain both gain contributions.");
    require(built.model->getInstrumentControlBindings().controlCount() == 2,
            "Instrument-control audio fixture must compile both bindings.");

    SamplerVoicePool pool;
    require(pool.prepare(*built.model, 48000.0), "Instrument-control audio pool must prepare.");
    SamplerRenderEvent events[3];
    events[0].type = SamplerRenderEventType::controllerChange;
    events[0].controllerNumber = 20;
    events[0].controllerValue = firstValue;
    events[1] = events[0];
    events[1].controllerNumber = 21;
    events[1].controllerValue = secondValue;
    events[2].type = SamplerRenderEventType::noteOn;
    events[2].midiNote = 60;
    events[2].velocity = 1.0f;
    std::vector<float> left(1, 0.0f);
    std::array<float*, 1> channels { left.data() };
    instrument_control_allocation_probe::allocations = 0;
    instrument_control_allocation_probe::deallocations = 0;
    instrument_control_allocation_probe::enabled = true;
    const auto result = pool.renderBlock({ channels.data(), 1, 1 }, { events, 3 });
    instrument_control_allocation_probe::enabled = false;
    require(result.accepted && result.render.startedVoiceCount == 1,
            "Instrument-control audio fixture must start one voice.");
    require(instrument_control_allocation_probe::allocations == 0
                && instrument_control_allocation_probe::deallocations == 0,
            "Instrument-control callback routing must not allocate or free on the render thread.");
    require(SamplerRenderThreadInstrumentation::lastLockAttemptCount() == 0,
            "Instrument-control callback routing must not attempt a lock on the render thread.");
    const auto slot = pool.getSlotSnapshot(0);
    if (panGains != nullptr)
        *panGains = pool.activeVoicePanGains();
    if (tuningCents != nullptr)
        *tuningCents = pool.activeVoiceEffectiveTuningCents();
    if (envelopeDecaySeconds != nullptr)
        *envelopeDecaySeconds = pool.activeVoiceEnvelopeDecaySeconds();
    if (left.front() == 0.0f && targetKind != RuntimeInstrumentControlTargetKind::pan)
        throw std::runtime_error("Instrument-control audio fixture rendered silence: baseGain="
                                 + std::to_string(slot.baseGain)
                                 + " active=" + std::to_string(result.activeVoiceCount));

    SamplerVoicePool saturationPool;
    require(saturationPool.prepare(*built.model, 48000.0),
            "Instrument-control saturation pool must prepare.");
    std::array<SamplerRenderEvent, SamplerVoicePool::capacity + 1> saturationEvents {};
    for (auto& event : saturationEvents)
    {
        event.type = SamplerRenderEventType::noteOn;
        event.midiNote = 60;
        event.velocity = 1.0f;
    }
    std::vector<float> saturationAudio(1, 0.0f);
    std::array<float*, 1> saturationChannels { saturationAudio.data() };
    const auto saturation = saturationPool.renderBlock(
        { saturationChannels.data(), 1, 1 },
        { saturationEvents.data(), saturationEvents.size() });
    require(saturation.accepted && saturation.render.stolenVoiceCount >= 1,
            "A dense instrument-control note burst must exercise bounded voice stealing.");
    return left.front();
}
}

int main()
{
    using namespace drs::engine;
    try
    {
        RuntimeProjectInstrumentControlDefinition control;
        control.id = "runtime.gain";
        control.normalizedDefault = 0.25;
        RuntimeProjectMidiControlBindingDefinition binding;
        binding.id = "binding.runtime.gain.cc20";
        binding.controlId = control.id;
        binding.controllerNumber = 20;

        InstrumentControlBindingTable table;
        std::vector<RuntimeInstrumentControlBindingIssue> issues;
        require(table.compile({ control }, { binding }, issues),
                "Runtime table compilation must accept a bounded control catalog.");
        InstrumentControlRuntimeState state;
        state.prepare(table);
        require(state.currentValue(0) == 0.25, "Runtime state must publish authored defaults.");
        require(state.applyMidi(1, 20, 64, table), "Mapped CC must update runtime state.");
        require(state.currentValue(0) > 0.50 && state.currentValue(0) < 0.51,
                "Mapped CC must normalize to the expected 0..1 value.");
        require(state.resetControl(0) && state.currentValue(0) == 0.25,
                "Reset must be deterministic and restore the authored default.");
        for (int value = 0; value <= 127; ++value)
            require(state.applyMidi(1, 20, static_cast<std::uint8_t>(value), table),
                    "A dense 7-bit CC sweep must remain bounded and mapped.");
        std::vector<RuntimeProjectInstrumentControlDefinition> overflowControls(
            maximumInstrumentControls + 1);
        for (std::size_t index = 0; index < overflowControls.size(); ++index)
        {
            overflowControls[index].id = "overflow." + std::to_string(index);
            overflowControls[index].normalizedDefault = 0.5;
        }
        std::vector<RuntimeInstrumentControlBindingIssue> overflowIssues;
        InstrumentControlBindingTable overflowTable;
        require(!overflowTable.compile(overflowControls, {}, overflowIssues)
                    && !overflowIssues.empty(),
                "Control-capacity overflow must be rejected instead of silently truncating.");
        const auto oneController = renderGainCase(64, 127);
        const auto twoControllers = renderGainCase(64, 64);
        require(oneController > 0.49f && oneController < 0.52f,
                (std::string("A single gain controller must produce its normalized contribution: ")
                 + std::to_string(oneController)).c_str());
        require(twoControllers > 0.24f && twoControllers < 0.27f
                    && twoControllers < oneController * 0.6f,
                (std::string("Independent gain controllers must multiply on the rendered route: ")
                 + std::to_string(twoControllers)).c_str());
        SamplerPanGains panLow;
        SamplerPanGains panHigh;
        static_cast<void>(renderGainCase(
            0, 0, RuntimeInstrumentControlTargetKind::pan, &panLow));
        static_cast<void>(renderGainCase(
            127, 127, RuntimeInstrumentControlTargetKind::pan, &panHigh));
        require(panLow.left > 0.99f && panLow.right < 0.01f
                    && panHigh.left < 0.01f && panHigh.right > 0.99f,
                "Pan control contributions must add and clamp to the expected stereo gains.");
        double tuneLow = 0.0;
        double tuneHigh = 0.0;
        SamplerPanGains unusedPan;
        static_cast<void>(renderGainCase(
            0, 0, RuntimeInstrumentControlTargetKind::tune,
            &unusedPan, &tuneLow));
        static_cast<void>(renderGainCase(
            127, 127, RuntimeInstrumentControlTargetKind::tune,
            &unusedPan, &tuneHigh));
        require(tuneLow < -199.0 && tuneHigh > 199.0,
                "Tune control contributions must update pitch cents at note-on.");
        double decayLow = 0.0;
        double decayHigh = 0.0;
        static_cast<void>(renderGainCase(
            0, 0, RuntimeInstrumentControlTargetKind::envelopeDecay,
            &unusedPan, nullptr, &decayLow));
        static_cast<void>(renderGainCase(
            127, 127, RuntimeInstrumentControlTargetKind::envelopeDecay,
            &unusedPan, nullptr, &decayHigh));
        require(decayLow < 0.001 && decayHigh > 1.9,
                "Envelope decay control contributions must be sampled at note-on.");
        std::cout << "Instrument control runtime tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Instrument control runtime tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
