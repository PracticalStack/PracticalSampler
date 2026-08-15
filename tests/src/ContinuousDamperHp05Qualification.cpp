#include "Sprint4OfflineRenderHarness.h"

#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SfzImportProjection.h"
#include "shared/ProjectStorage.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using namespace drs::engine;

struct TimedMidiEvent
{
    double seconds = 0.0;
    SamplerRenderEventType type = SamplerRenderEventType::noteOn;
    int note = 60;
    int velocity = 0;
    int noteOffVelocity = 0;
    int channel = 0;
    int controllerNumber = 0;
    int controllerValue = 0;
    std::uint32_t sequence = 0;
};

struct Passage
{
    std::vector<TimedMidiEvent> events;
    double durationSeconds = 0.0;
    std::size_t noteOnCount = 0;
    std::size_t noteOffCount = 0;
    std::size_t cc64Count = 0;
};

struct MatrixResult
{
    double sampleRate = 0.0;
    std::string checksum128;
    std::uint64_t startedVoices = 0;
    std::uint64_t releasedVoices = 0;
    std::uint64_t dynamicUpdates = 0;
    std::uint64_t repedalCatches = 0;
    double passageTailRms = 0.0;
    double legacyTailRms = 0.0;
};

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

RuntimeProjectModel makeBlankProject(const fs::path& sfzPath)
{
    RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "drs.hp05.accurate-salamander";
    project.displayName = "HP-05 Accurate Salamander qualification";
    project.contentRootPath = sfzPath.parent_path().generic_string();
    project.defaultInstrumentManifestPath
        = (sfzPath.parent_path() / "hp05-accurate-salamander.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    return project;
}

const SfzImportOpcodeSupportSummary* findOpcode(const SfzImportAnalysisResult& analysis,
                                                const std::string& opcode)
{
    const auto found = std::find_if(analysis.report.opcodeSupport.begin(),
                                    analysis.report.opcodeSupport.end(),
                                    [&](const auto& value)
                                    { return value.opcodeName == opcode; });
    return found == analysis.report.opcodeSupport.end() ? nullptr : &*found;
}

const RuntimeProjectZoneDefinition& qualifyLivePreset(
    const fs::path& sfzPath,
    SfzImportProjectionResult& projection,
    RuntimeProjectModel& importedProject,
    std::size_t& curve11ZoneCount,
    std::size_t& curve12ZoneCount)
{
    const auto analysis = analyzeSfzImportDocument(sfzPath.generic_string());
    require(analysis.analyzed && analysis.report.available && !analysis.report.blocking,
            "The Salamander live recommended preset must analyze without a blocking finding");

    for (const auto* opcode : { "sustain_cc", "ampeg_dynamic", "ampeg_releasecc64",
                                "ampeg_release_curvecc64", "curve_index", "v064" })
    {
        const auto* support = findOpcode(analysis, opcode);
        require(support != nullptr
                    && support->disposition == SfzImportSupportDisposition::converted,
                std::string("The live-preset half-pedal opcode must be converted: ") + opcode);
    }
    for (const auto* opcode : { "ampeg_release_oncc72", "volume_oncc23" })
    {
        const auto* support = findOpcode(analysis, opcode);
        require(support != nullptr
                    && support->disposition == SfzImportSupportDisposition::reportedOnly,
                std::string("Deferred resonance/general modulation must remain report-only: ")
                    + opcode);
    }
    require(analysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired,
            "Deferred live-preset findings must remain visible at the review gate");

    const auto baseProject = makeBlankProject(sfzPath);
    projection = projectSfzImportAnalysis(baseProject, analysis);
    require(projection.projected && projection.playable && !projection.blocking
                && projection.semanticAnalyzedRegionCount == 1704
                && projection.zones.size() == 1700
                && projection.sampleSources.size() == 637
                && projection.omittedUnsafeRegionCount == 4,
            "The live recommended preset must retain the approved Salamander projection boundary");

    curve11ZoneCount = 0;
    curve12ZoneCount = 0;
    const RuntimeProjectZoneDefinition* representative = nullptr;
    for (const auto& zone : projection.zones)
    {
        if (!zone.damper.dynamicRelease)
            continue;
        if (zone.damper.releaseCurveIndex == 11
            && zone.performance.event == PerformanceEventKind::noteOn)
        {
            ++curve11ZoneCount;
            if (representative == nullptr)
                representative = &zone;
        }
        else if (zone.damper.releaseCurveIndex == 12)
        {
            ++curve12ZoneCount;
        }
    }
    const auto curve11Details = representative == nullptr
        ? std::string("representative=missing")
        : "representative=" + representative->id
            + " sustain=" + std::to_string(representative->damper.sustainControllerNumber)
            + " releaseCc=" + std::to_string(representative->damper.releaseControllerNumber)
            + " baseRelease=" + std::to_string(representative->releaseSeconds)
            + " amount=" + std::to_string(representative->damper.releaseAmountSeconds)
            + " v032=" + std::to_string(representative->damper.releaseCurve[32])
            + " v042=" + std::to_string(representative->damper.releaseCurve[42])
            + " v054=" + std::to_string(representative->damper.releaseCurve[54])
            + " v062=" + std::to_string(representative->damper.releaseCurve[62])
            + " v064=" + std::to_string(representative->damper.releaseCurve[64]);
    require(representative != nullptr && curve11ZoneCount == 1408
                && representative->damper.sustainControllerNumber == 90
                && representative->damper.releaseControllerNumber == 64
                && representative->damper.releaseAmountSeconds == 100.0
                && std::abs(representative->releaseSeconds - 1.0) < 1.0e-12
                && representative->damper.releaseCurve[32] == 0.0
                && representative->damper.releaseCurve[42] == 0.1
                && representative->damper.releaseCurve[54] == 0.3
                && representative->damper.releaseCurve[62] == 0.4
                && representative->damper.releaseCurve[64] == 1.0,
            "All 1,408 piano note regions must inherit the exact Salamander curve-11 damper law"
                " (curve11=" + std::to_string(curve11ZoneCount)
                + ", curve12=" + std::to_string(curve12ZoneCount)
                + ", " + curve11Details + ")");
    require(curve12ZoneCount > 0,
            "Pseudo-resonance regions must remain distinguishable from curve-11 note regions");

    AuthoringSession session(baseProject);
    const auto applied = applySfzImportProjection(
        session, projection, "Import Salamander live recommended preset for HP-05");
    require(applied.applied, "The reviewed live-preset projection must apply atomically");
    importedProject = session.getProject();

    const auto selected = std::find_if(importedProject.authoring.zones.begin(),
                                       importedProject.authoring.zones.end(),
                                       [&](const auto& zone)
                                       { return zone.id == representative->id; });
    require(selected != importedProject.authoring.zones.end(),
            "The representative curve-11 note zone must survive authoring apply");
    return *selected;
}

void qualifyMetadataPackage(const RuntimeProjectModel& project, const fs::path& scratchRoot)
{
    fs::create_directories(scratchRoot);
    const auto projectPath = scratchRoot / "accurate-salamander-live.drsproj";
    const auto instrument = drs::app::buildInstrumentManifestForProject(
        project, juce::File(projectPath.generic_string()));
    require(instrument.schemaVersion == continuousDamperInstrumentSchemaVersion
                && instrument.zones.size() == 1700,
            "The live-preset project must emit a complete instrument-schema-5 manifest");
    const auto instrumentPath = scratchRoot / "accurate-salamander-live.drinst";
    const auto serialized = serializeRuntimeInstrumentManifest(
        instrument, instrumentPath.generic_string());

    PerformancePackageWritePlan plan;
    plan.outputPackagePath = (scratchRoot / "accurate-salamander-live-metadata.drpkg")
                                 .generic_string();
    plan.manifest.packageId = "drs.hp05.accurate-salamander.metadata";
    plan.manifest.displayName = "HP-05 Accurate Salamander metadata qualification";
    plan.manifest.instrumentId = instrument.instrumentId;
    plan.payloads.push_back({
        "runtime-instrument", PerformancePackagePayloadKind::runtimeInstrument,
        "manifest/runtime-instrument.drinst", "application/json",
        std::vector<std::uint8_t>(serialized.begin(), serialized.end())
    });
    const auto written = writePerformancePackage(plan);
    require(written.written, "The live-preset runtime metadata package must write");
    const auto reopened = inspectPerformancePackage(plan.outputPackagePath);
    require(reopened.valid, "The live-preset metadata package must authenticate on reopen");
    const auto payload = std::find_if(reopened.payloads.begin(), reopened.payloads.end(),
                                      [](const auto& value)
                                      { return value.payloadKind == "runtimeInstrument"; });
    require(payload != reopened.payloads.end(),
            "The reopened package must expose its runtime instrument");
    const std::string json(payload->plaintextBytes.begin(), payload->plaintextBytes.end());
    const auto parsed = parseRuntimeInstrumentManifest(
        json, "package://manifest/runtime-instrument.drinst", false);
    require(parsed.loaded && parsed.instrument.zones.size() == 1700,
            "The package runtime instrument must reopen without losing Salamander routes");
    const auto curve11Count = std::count_if(parsed.instrument.zones.begin(),
                                            parsed.instrument.zones.end(),
                                            [](const auto& zone)
                                            {
                                                return zone.damper.dynamicRelease
                                                    && zone.damper.releaseCurveIndex == 11
                                                    && zone.performance.event
                                                        == PerformanceEventKind::noteOn;
                                            });
    require(curve11Count == 1408,
            "Package reopen must retain curve 11 on all 1,408 piano note routes");
}

Passage readPassage(const fs::path& midiPath, const bool requireFrozenHp05Identity = true)
{
    juce::FileInputStream input(juce::File(midiPath.generic_string()));
    require(input.openedOk(), "The reported Accurate Salamander MIDI passage must open");
    juce::MidiFile midi;
    require(midi.readFrom(input), "The reported MIDI passage must parse");
    require(midi.getNumTracks() == 2 && midi.getTimeFormat() == 960,
            "The reported MIDI passage identity changed");
    midi.convertTimestampTicksToSeconds();

    Passage passage;
    std::uint32_t sourceSequence = 1;
    for (int trackIndex = 0; trackIndex < midi.getNumTracks(); ++trackIndex)
    {
        const auto* track = midi.getTrack(trackIndex);
        require(track != nullptr, "Every MIDI track must remain readable");
        for (int eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
        {
            const auto* holder = track->getEventPointer(eventIndex);
            require(holder != nullptr, "Every MIDI event must remain readable");
            const auto& message = holder->message;
            TimedMidiEvent event;
            event.seconds = message.getTimeStamp();
            event.channel = std::clamp(message.getChannel() - 1, 0, 15);
            event.sequence = sourceSequence++;
            if (message.isNoteOn())
            {
                event.type = SamplerRenderEventType::noteOn;
                event.note = message.getNoteNumber();
                event.velocity = message.getVelocity();
                ++passage.noteOnCount;
            }
            else if (message.isNoteOff())
            {
                event.type = SamplerRenderEventType::noteOff;
                event.note = message.getNoteNumber();
                event.noteOffVelocity = message.getVelocity();
                ++passage.noteOffCount;
            }
            else if (message.isController() && message.getControllerNumber() == 64)
            {
                event.type = SamplerRenderEventType::controllerChange;
                event.controllerNumber = 64;
                event.controllerValue = message.getControllerValue();
                ++passage.cc64Count;
            }
            else
            {
                continue;
            }
            passage.durationSeconds = std::max(passage.durationSeconds, event.seconds);
            passage.events.push_back(event);
        }
    }
    std::stable_sort(passage.events.begin(), passage.events.end(), [](const auto& left,
                                                                     const auto& right)
    {
        if (left.seconds != right.seconds) return left.seconds < right.seconds;
        return left.sequence < right.sequence;
    });
    for (std::size_t index = 0; index < passage.events.size(); ++index)
        passage.events[index].sequence = static_cast<std::uint32_t>(index + 1);
    if (requireFrozenHp05Identity)
        require(passage.noteOnCount == 55 && passage.noteOffCount == 52
                    && passage.cc64Count == 28 && passage.durationSeconds > 3.99,
                "The reported passage must retain its frozen notes and 28-event CC64 trace"
                    " (noteOn=" + std::to_string(passage.noteOnCount)
                    + ", noteOff=" + std::to_string(passage.noteOffCount)
                    + ", cc64=" + std::to_string(passage.cc64Count)
                    + ", duration=" + std::to_string(passage.durationSeconds) + ")");
    return passage;
}

SamplerRenderModelPtr buildQualificationModel(const ContinuousDamperDefinition& damper,
                                               const std::size_t revision,
                                               const std::string& id)
{
    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = revision;
    snapshot.contentDigest = id + "-snapshot";
    PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = id + "-zone";
    snapshotZone.sampleSourceId = id + "-sample";
    snapshotZone.displayName = "HP-05 deterministic piano route";
    snapshotZone.groupId = "main";
    snapshotZone.rootKey = 60;
    snapshotZone.keyLow = 0;
    snapshotZone.keyHigh = 127;
    snapshotZone.loopEnabled = true;
    snapshotZone.loopStartFrame = 0;
    snapshotZone.loopEndFrame = 65536;
    snapshotZone.releaseSeconds = 0.02;
    snapshotZone.releaseShape = sfzDefaultReleaseShape;
    snapshotZone.damper = damper;
    snapshot.zones.push_back(snapshotZone);
    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "main";
    snapshotGroup.zoneIds = { snapshotZone.id };
    snapshot.groupRoutes.push_back(snapshotGroup);

    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::vector<float>(65536, 0.125f) };
    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 5000 + revision * 2;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = revision;
    prepared.preparedContentDigest = id + "-prepared";
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = snapshotZone.sampleSourceId;
    sample.streamSampleId = id + "-stream";
    sample.sampleRate = 48000.0;
    sample.frameCount = 65536;
    sample.channelCount = 1;
    sample.decodedSampleData = decoded;
    prepared.samples.push_back(sample);
    PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = snapshotZone.id;
    preparedZone.sampleSourceId = snapshotZone.sampleSourceId;
    preparedZone.streamSampleId = sample.streamSampleId;
    preparedZone.preparedSampleIndex = 0;
    preparedZone.rootKey = snapshotZone.rootKey;
    preparedZone.keyLow = snapshotZone.keyLow;
    preparedZone.keyHigh = snapshotZone.keyHigh;
    preparedZone.loopEnabled = true;
    preparedZone.loopStartFrame = 0;
    preparedZone.loopEndFrame = 65536;
    preparedZone.releaseSeconds = snapshotZone.releaseSeconds;
    preparedZone.releaseShape = snapshotZone.releaseShape;
    preparedZone.damper = damper;
    prepared.zones.push_back(preparedZone);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "main";
    preparedGroup.zoneIds = { preparedZone.zoneId };
    prepared.groupRoutes.push_back(preparedGroup);

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::performance;
    payload->revision = revision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = prepared.snapshotBuildId + 1;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto built = buildSamplerRenderModel(payload);
    require(built.built && built.model != nullptr,
            "The HP-05 deterministic qualification model must build");
    return built.model;
}

std::vector<drs::tests::OfflineTimelineEvent> timelineAt(const Passage& passage,
                                                         const double sampleRate)
{
    std::vector<drs::tests::OfflineTimelineEvent> timeline;
    timeline.reserve(passage.events.size());
    for (const auto& source : passage.events)
    {
        drs::tests::OfflineTimelineEvent event;
        event.frame = static_cast<std::uint64_t>(std::llround(source.seconds * sampleRate));
        event.type = source.type;
        event.midiNote = static_cast<std::uint8_t>(source.note);
        event.velocity = static_cast<float>(source.velocity) / 127.0f;
        event.midiChannel = static_cast<std::uint8_t>(source.channel);
        event.noteOffVelocity = static_cast<float>(source.noteOffVelocity) / 127.0f;
        event.inputSequence = source.sequence;
        event.controllerNumber = static_cast<std::uint8_t>(source.controllerNumber);
        event.controllerValue = static_cast<std::uint8_t>(source.controllerValue);
        timeline.push_back(event);
    }
    drs::tests::OfflineTimelineEvent termination;
    termination.frame = static_cast<std::uint64_t>(
        std::llround((passage.durationSeconds + 0.01) * sampleRate));
    termination.type = SamplerRenderEventType::allNotesOff;
    termination.inputSequence = static_cast<std::uint32_t>(timeline.size() + 1);
    timeline.push_back(termination);
    return timeline;
}

double rmsWindow(const drs::tests::OfflineRenderArtifact& artifact,
                 const double sampleRate,
                 const double startSeconds,
                 const double endSeconds)
{
    const auto start = std::min<std::size_t>(artifact.channels[0].size(),
        static_cast<std::size_t>(std::llround(startSeconds * sampleRate)));
    const auto end = std::min<std::size_t>(artifact.channels[0].size(),
        static_cast<std::size_t>(std::llround(endSeconds * sampleRate)));
    require(end > start, "The passage-tail RMS window must be non-empty");
    long double squares = 0.0;
    std::uint64_t count = 0;
    for (std::size_t frame = start; frame < end; ++frame)
    {
        for (const auto& channel : artifact.channels)
        {
            const auto value = static_cast<long double>(channel[frame]);
            squares += value * value;
            ++count;
        }
    }
    return std::sqrt(static_cast<double>(squares / static_cast<long double>(count)));
}

MatrixResult qualifyRenderMatrix(const Passage& passage,
                                 const SamplerRenderModelPtr& continuousModel,
                                 const SamplerRenderModelPtr& legacyModel,
                                 const double sampleRate)
{
    const auto frameCount = static_cast<std::uint64_t>(
        std::ceil((passage.durationSeconds + 3.0) * sampleRate));
    const auto events = timelineAt(passage, sampleRate);
    const auto makeRequest = [&](const SamplerRenderModelPtr& model,
                                 const std::uint32_t partition,
                                 const std::string& suffix)
    {
        drs::tests::OfflineRenderRequest request;
        request.scenarioId = "hp05-accurate-salamander-" + suffix + "-"
            + std::to_string(static_cast<int>(sampleRate)) + "-" + std::to_string(partition);
        request.model = model;
        request.sampleRate = sampleRate;
        request.frameCount = frameCount;
        request.partitionSize = partition;
        request.events = events;
        return request;
    };

    const auto reference = drs::tests::renderOffline(makeRequest(continuousModel, 128, "continuous"));
    for (const auto partition : { 256u, 512u })
    {
        const auto candidate = drs::tests::renderOffline(
            makeRequest(continuousModel, partition, "continuous"));
        const auto comparison = drs::tests::compareOfflineArtifacts(reference, candidate);
        require(comparison.equivalent,
                "The real-passage continuous-damper render changed with block size "
                    + std::to_string(partition) + ": " + comparison.message);
    }
    const auto legacy = drs::tests::renderOffline(makeRequest(legacyModel, 128, "legacy"));
    const auto tailStart = 3.2;
    const auto tailEnd = std::min(passage.durationSeconds + 2.5,
                                  static_cast<double>(frameCount) / sampleRate);
    const auto continuousTail = rmsWindow(reference, sampleRate, tailStart, tailEnd);
    const auto legacyTail = rmsWindow(legacy, sampleRate, tailStart, tailEnd);

    require(reference.summary.counters.dynamicReleaseUpdateCount > 0,
            "Intermediate CC64 values must update still-audible passage voices");
    require(reference.summary.counters.repedalCatchCount > 0,
            "The rising portion of the passage CC64 sweep must catch audible release tails");
    require(reference.summary.counters.droppedEventCount == 0,
            "The real MIDI passage must not overflow the bounded callback event path");
    require(reference.summary.lastNonZeroFrame
                == static_cast<std::int64_t>(reference.summary.frameCount - 1),
            "The continuous-damper passage tail must remain audible through the qualification window");
    require(continuousTail > legacyTail * 1.05,
            "Continuous damping must remove the legacy passage's premature tail cutoff");

    return { sampleRate,
             reference.summary.quantizedChecksum,
             reference.summary.counters.startedVoiceCount,
             reference.summary.counters.releasedVoiceCount,
             reference.summary.counters.dynamicReleaseUpdateCount,
             reference.summary.counters.repedalCatchCount,
             continuousTail,
             legacyTail };
}

void writeReport(const fs::path& reportPath,
                 const fs::path& sfzPath,
                 const fs::path& midiPath,
                 const Passage& passage,
                 const std::size_t curve11ZoneCount,
                 const std::size_t curve12ZoneCount,
                 const std::array<MatrixResult, 2>& matrix)
{
    if (reportPath.empty())
        return;
    fs::create_directories(reportPath.parent_path());
    std::ofstream report(reportPath, std::ios::binary | std::ios::trunc);
    require(report.good(), "The HP-05 qualification report must be writable");
    report << std::setprecision(12)
           << "{\n"
           << "  \"schema\": \"drs.continuous-damper.hp05-qualification/v1\",\n"
           << "  \"result\": \"PASS_WITH_REFERENCE_PENDING\",\n"
           << "  \"sfz\": \"" << sfzPath.generic_string() << "\",\n"
           << "  \"midi\": \"" << midiPath.generic_string() << "\",\n"
           << "  \"noteOnCount\": " << passage.noteOnCount << ",\n"
           << "  \"noteOffCount\": " << passage.noteOffCount << ",\n"
           << "  \"cc64Count\": " << passage.cc64Count << ",\n"
           << "  \"curve11NoteZoneCount\": " << curve11ZoneCount << ",\n"
           << "  \"curve12PseudoResonanceZoneCount\": " << curve12ZoneCount << ",\n"
           << "  \"plogueReferenceAvailable\": false,\n"
           << "  \"ploguePerceptualSignoff\": \"PENDING_EXTERNAL_REFERENCE\",\n"
           << "  \"resonanceScope\": \"DEFERRED\",\n"
           << "  \"matrix\": [\n";
    for (std::size_t index = 0; index < matrix.size(); ++index)
    {
        const auto& item = matrix[index];
        report << "    { \"sampleRate\": " << item.sampleRate
               << ", \"blockSizes\": [128, 256, 512]"
               << ", \"checksum128\": \"" << item.checksum128 << "\""
               << ", \"startedVoices\": " << item.startedVoices
               << ", \"releasedVoices\": " << item.releasedVoices
               << ", \"dynamicUpdates\": " << item.dynamicUpdates
               << ", \"repedalCatches\": " << item.repedalCatches
               << ", \"continuousTailRms\": " << item.passageTailRms
               << ", \"legacyTailRms\": " << item.legacyTailRms << " }"
               << (index + 1 == matrix.size() ? "\n" : ",\n");
    }
    report << "  ]\n}\n";
    require(report.good(), "The HP-05 qualification report write must complete");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const auto workspaceRoot = fs::path(DRS_WORKSPACE_ROOT);
        const auto sfzPath = workspaceRoot
            / "DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit"
              "/sfz_live/Accurate-SalamanderGrandPiano_flat.Recommended.sfz";
        const auto originalMidiPath = workspaceRoot / "DemoMidi/AccurateSalamanderTests.mid";
        const auto isolatedMidiPath = workspaceRoot
            / "DemoMidi/AccurateSalamander_RePedalIssue.mid";
        const auto hasOriginalMidi = fs::is_regular_file(originalMidiPath);
        const auto midiPath = hasOriginalMidi ? originalMidiPath : isolatedMidiPath;
        require(fs::is_regular_file(sfzPath) && fs::is_regular_file(midiPath),
                "The HP-05 real Salamander corpus and MIDI passage must be available");

        SfzImportProjectionResult projection;
        RuntimeProjectModel importedProject;
        std::size_t curve11ZoneCount = 0;
        std::size_t curve12ZoneCount = 0;
        const auto& representative = qualifyLivePreset(
            sfzPath, projection, importedProject, curve11ZoneCount, curve12ZoneCount);
        qualifyMetadataPackage(importedProject,
                               fs::temp_directory_path() / "drs-continuous-damper-hp05");

        const auto passage = readPassage(midiPath, hasOriginalMidi);
        const auto continuousModel = buildQualificationModel(
            representative.damper, 501, "hp05-continuous");
        const auto legacyModel = buildQualificationModel(
            ContinuousDamperDefinition {}, 502, "hp05-legacy");
        const std::array matrix {
            qualifyRenderMatrix(passage, continuousModel, legacyModel, 44100.0),
            qualifyRenderMatrix(passage, continuousModel, legacyModel, 48000.0)
        };
        require(matrix[0].startedVoices == matrix[1].startedVoices
                    && matrix[0].releasedVoices == matrix[1].releasedVoices
                    && matrix[0].dynamicUpdates == matrix[1].dynamicUpdates
                    && matrix[0].repedalCatches == matrix[1].repedalCatches,
                "The real-passage lifecycle and damper diagnostics must be sample-rate invariant");

        const auto reportPath = argc > 1 ? fs::path(argv[1]) : fs::path {};
        writeReport(reportPath, sfzPath, midiPath, passage,
                    curve11ZoneCount, curve12ZoneCount, matrix);
        std::cout << "Continuous damper HP-05 qualification passed: notes="
                  << passage.noteOnCount << " cc64=" << passage.cc64Count
                  << " curve11Zones=" << curve11ZoneCount
                  << " dynamicUpdates=" << matrix[1].dynamicUpdates
                  << " repedalCatches=" << matrix[1].repedalCatches
                  << " plogueReference=PENDING" << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Continuous damper HP-05 qualification failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
