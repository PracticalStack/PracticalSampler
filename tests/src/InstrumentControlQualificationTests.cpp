#include "drs/engine/SfzImport.h"
#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/AuthoringSession.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoicePool.h"
#include "drs/engine/SampleDataSource.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>

namespace
{
void require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

float renderImportedSampleThroughGainControl(
    const drs::engine::ImportedSampleData& imported,
    const drs::engine::RuntimeProjectInstrumentControlDefinition& sourceControl,
    const drs::engine::RuntimeProjectMidiControlBindingDefinition& sourceBinding,
    const drs::engine::RuntimeProjectInstrumentControlTargetDefinition& sourceTarget,
    const std::uint8_t controllerValue)
{
    using namespace drs::engine;
    ImmutablePlaybackSnapshot snapshot;
    snapshot.schemaName = "drs.playback.snapshot";
    snapshot.schemaVersion = 1;
    snapshot.contentDigest = "naked-drums-control-audio";
    snapshot.draftRevision = 1;

    auto control = sourceControl;
    snapshot.instrumentControls.push_back(control);
    ImmutablePlaybackSnapshot::InstrumentControlValue value;
    value.id = control.id;
    value.normalizedValue = 1.0;
    snapshot.instrumentControlValues.push_back(value);

    auto binding = sourceBinding;
    snapshot.midiControlBindings.push_back(binding);
    auto target = sourceTarget;
    // The one-sample audio fixture intentionally uses a single route. Preserve the
    // imported target law while removing the source-specific owner constraint.
    target.ownerKind.clear();
    target.ownerId.clear();
    snapshot.instrumentControlTargets.push_back(target);

    PlaybackSnapshotZone zone;
    zone.id = "naked-drums.audio-zone";
    zone.sampleSourceId = "naked-drums.audio-sample";
    zone.displayName = "Naked Drums audio control fixture";
    zone.groupId = "naked-drums.audio-group";
    zone.rootKey = zone.keyLow = zone.keyHigh = 60;
    zone.velocityLow = 1;
    zone.velocityHigh = 127;
    snapshot.zones.push_back(zone);
    PlaybackSnapshotGroupRoute group;
    group.groupId = zone.groupId;
    group.zoneIds = { zone.id };
    group.displayName = "Naked Drums audio control fixture";
    snapshot.groupRoutes.push_back(group);

    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = imported.normalizedChannels;
    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 1;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.preparedContentDigest = "naked-drums-control-audio-prepared";
    prepared.draftRevision = snapshot.draftRevision;
    prepared.instrumentControls = snapshot.instrumentControls;
    prepared.instrumentControlValues = snapshot.instrumentControlValues;
    prepared.instrumentControlTargets = snapshot.instrumentControlTargets;
    prepared.midiControlBindings = snapshot.midiControlBindings;
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = zone.sampleSourceId;
    sample.streamSampleId = "naked-drums.audio-stream";
    sample.sampleRate = imported.metadata.sampleRate;
    sample.frameCount = imported.metadata.frameCount;
    sample.channelCount = imported.metadata.channelCount;
    sample.decodedSampleData = decoded;
    prepared.samples.push_back(sample);
    PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = zone.id;
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
    if (!built.built || built.model == nullptr)
        throw std::runtime_error("The actual Naked Drums sample control fixture did not compile.");

    SamplerVoicePool pool;
    if (!pool.prepare(*built.model, imported.metadata.sampleRate))
        throw std::runtime_error("The actual Naked Drums sample control fixture did not prepare.");
    std::array<SamplerRenderEvent, 2> events {};
    events[0].type = SamplerRenderEventType::controllerChange;
    events[0].controllerNumber = static_cast<std::uint8_t>(binding.controllerNumber);
    events[0].controllerValue = controllerValue;
    events[1].type = SamplerRenderEventType::noteOn;
    events[1].midiNote = 60;
    events[1].velocity = 1.0f;
    constexpr std::size_t frameCount = 256;
    std::vector<float> audio(frameCount, 0.0f);
    std::array<float*, 1> channels { audio.data() };
    const auto result = pool.renderBlock({ channels.data(), 1, frameCount },
                                          { events.data(), events.size() });
    if (!result.accepted || result.render.startedVoiceCount != 1)
        throw std::runtime_error("The actual Naked Drums sample control fixture did not start a voice.");
    return *std::max_element(audio.begin(), audio.end(),
        [](const float left, const float right) { return std::abs(left) < std::abs(right); });
}
}

int main()
{
    using namespace drs::engine;
    try
    {
        const auto fixture = std::string(DRS_SOURCE_ROOT)
            + "/tests/fixtures/instrument-controls/controller-surface-baseline.sfz";
        const auto parsed = parseSfzDocument(fixture);
        require(parsed.parsed, "The synthetic controller surface fixture must parse.");
        require(parsed.document.sections.size() >= 2,
                "Qualification fixture must retain global and region sections.");
        const auto nakedDrums = std::string(DRS_SOURCE_ROOT)
            + "/../DemoSFVInstruments/WilkinsonAudio.NakedDrums-master/WilkinsonAudio.NakedDrums-master/Wilkinson Audio/Naked Drums/User/Naked Drums GM.sfz";
        const auto parseStarted = std::chrono::steady_clock::now();
        const auto nakedParsed = parseSfzDocument(nakedDrums);
        const auto parseElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - parseStarted);
        require(nakedParsed.parsed && nakedParsed.complete
                    && nakedParsed.document.sections.size() > 1900,
                "The Naked Drums GM corpus must parse its complete include graph deterministically.");

        RuntimeProjectModel nakedProject;
        nakedProject.schemaName = "drs.project";
        nakedProject.schemaVersion = instrumentControlProjectSchemaVersion;
        nakedProject.projectId = "naked-drums-gm-controls";
        nakedProject.displayName = "Naked Drums GM";
        nakedProject.contentRootPath = std::string(DRS_SOURCE_ROOT) + "/../DemoSFVInstruments";
        nakedProject.defaultInstrumentManifestPath = "naked-drums-gm.drinst";
        nakedProject.authoring.schemaName = "drs.authoring";
        nakedProject.authoring.schemaVersion = instrumentControlAuthoringSchemaVersion;
        const auto analysisStarted = std::chrono::steady_clock::now();
        const auto nakedAnalysis = analyzeSfzImportDocument(nakedDrums);
        const auto analysisElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - analysisStarted);
        require(nakedAnalysis.analyzed && nakedAnalysis.report.available
                    && !nakedAnalysis.report.blocking
                    && nakedAnalysis.report.summary.blockingOpcodeCount == 0
                    && nakedAnalysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired,
                "Naked Drums GM must be review-ready with only explicit compatibility findings.");
        const auto hasReportSection = [&](const std::string& name)
        {
            return std::any_of(nakedAnalysis.report.sections.begin(),
                               nakedAnalysis.report.sections.end(),
                [&](const auto& section) { return section.name == name; });
        };
        require(hasReportSection("Controls") && hasReportSection("Bindings")
                    && hasReportSection("Target Coverage")
                    && hasReportSection("Hidden/Conditional Controllers")
                    && hasReportSection("Conflicts")
                    && hasReportSection("Unsupported Controller Targets"),
                "Naked Drums GM import review must expose all controller report sections.");
        const auto projectionStarted = std::chrono::steady_clock::now();
        const auto nakedProjection = projectSfzImportDocument(nakedProject, nakedDrums);
        const auto projectionElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - projectionStarted);
        require(parseElapsed < std::chrono::seconds(30)
                    && analysisElapsed < std::chrono::seconds(30)
                    && projectionElapsed < std::chrono::seconds(30),
                "Naked Drums import, report analysis, and projection must stay within bounded qualification time.");
        require(nakedProjection.projected && nakedProjection.playable,
                (std::string("Naked Drums GM must project into a playable native project: ")
                 + (nakedProjection.issues.empty() ? std::string {} : nakedProjection.issues.front())).c_str());
        require(nakedProjection.instrumentControls.size() == 26
                    && nakedProjection.midiControlBindings.size() == 26
                    && nakedProjection.instrumentControlTargets.size() == 25
                    && nakedProjection.sampleSources.size() == 1100
                    && nakedProjection.zones.size() == 1100,
                "Naked Drums GM must project a deterministic control, target, sample, and zone catalog.");
        require(!nakedProjection.sampleSources.empty(),
                "Naked Drums GM must expose at least one real sample source.");
        const auto& firstSample = nakedProjection.sampleSources.front();
        const auto firstSamplePath = std::filesystem::path(firstSample.path);
        require(std::filesystem::is_regular_file(firstSamplePath)
                    && std::filesystem::file_size(firstSamplePath) > 0,
                "Naked Drums GM first projected sample must resolve to a non-empty source file.");
        const auto firstSampleBytes = std::filesystem::file_size(firstSamplePath);
        const auto importedFirstSample = importSampleFile(firstSample.path);
        require(importedFirstSample.imported
                    && importedFirstSample.sample.metadata.formatName == "FLAC file"
                    && !importedFirstSample.sample.normalizedChannels.empty()
                    && importedFirstSample.sample.metadata.frameCount > 0,
                "Naked Drums GM first projected FLAC sample must decode through the product importer.");
        auto pagedFlacDescriptor = buildPagedSampleDataSourceDescriptor(
            firstSample.id, firstSample.path, 0, 64ull * 1024ull, 64ull * 1024ull);
        require(pagedFlacDescriptor.built,
                "Naked Drums GM FLAC sample must produce a bounded paged source descriptor.");
        WavPagedSampleDataSource pagedFlac(std::move(pagedFlacDescriptor));
        require(pagedFlac.prepareHead(),
                "Naked Drums GM FLAC sample must prepare its bounded streaming head.");
        const auto pagedHead = pagedFlac.acquireFrameView(0, 32);
        require(pagedHead.status == SampleFrameViewStatus::ready
                    && pagedHead.frameCount == 32
                    && pagedHead.channelCount == importedFirstSample.sample.metadata.channelCount,
                "Naked Drums GM FLAC paged source must expose decoded head frames.");
        const auto controlForActualGain = [&](const int controller)
        {
            const auto found = std::find_if(nakedProjection.instrumentControls.begin(),
                                            nakedProjection.instrumentControls.end(),
                [&](const auto& control)
                {
                    return control.importedSourceController == controller;
                });
            require(found != nakedProjection.instrumentControls.end(),
                    "Naked Drums GM actual gain control descriptor is missing.");
            return found;
        };
        const auto bindingForActualGain = [&](const std::string& controlId)
        {
            const auto found = std::find_if(nakedProjection.midiControlBindings.begin(),
                                            nakedProjection.midiControlBindings.end(),
                [&](const auto& binding) { return binding.controlId == controlId && binding.enabled; });
            require(found != nakedProjection.midiControlBindings.end(),
                    "Naked Drums GM actual gain binding is missing.");
            return found;
        };
        const auto targetForActualGain = [&](const std::string& controlId)
        {
            const auto found = std::find_if(nakedProjection.instrumentControlTargets.begin(),
                                            nakedProjection.instrumentControlTargets.end(),
                [&](const auto& target)
                {
                    return target.controlId == controlId
                        && target.targetKind == RuntimeInstrumentControlTargetKind::gain;
                });
            require(found != nakedProjection.instrumentControlTargets.end(),
                    "Naked Drums GM actual gain target is missing.");
            return found;
        };
        const auto qualifyActualGainControl = [&](const int controller)
        {
            const auto control = controlForActualGain(controller);
            const auto binding = bindingForActualGain(control->id);
            const auto target = targetForActualGain(control->id);
            const auto minimum = renderImportedSampleThroughGainControl(
                importedFirstSample.sample, *control, *binding, *target, 0);
            const auto maximum = renderImportedSampleThroughGainControl(
                importedFirstSample.sample, *control, *binding, *target, 127);
            require(std::abs(minimum) < 1.0e-6f && std::abs(maximum) > 0.0001f,
                    "Naked Drums GM actual sample rendering gain control did not respond.");
            return std::pair { minimum, maximum };
        };
        const auto actualGainCc20 = qualifyActualGainControl(20);
        const auto actualGainCc41 = qualifyActualGainControl(41);
        const auto actualGainCc42 = qualifyActualGainControl(42);
        const auto actualGainCc7 = qualifyActualGainControl(7);
        require(std::abs(controlForActualGain(7)->normalizedDefault - (100.0 / 127.0)) < 1.0e-9,
                "Naked Drums explicit set_cc7=100 must remain authoritative.");
        require(controlForActualGain(20)->normalizedDefault == 1.0
                    && controlForActualGain(41)->normalizedDefault == 1.0
                    && controlForActualGain(42)->normalizedDefault == 1.0,
                "Naked Drums gain controls without set_cc values must start open without an external UI script.");
        std::set<std::pair<int, int>> velocityWindows;
        for (const auto& zone : nakedProjection.zones)
            velocityWindows.insert({ zone.velocityLow, zone.velocityHigh });
        require(velocityWindows.size() >= 5
                    && velocityWindows.begin()->first <= 1
                    && velocityWindows.rbegin()->second >= 127,
                "Naked Drums GM must preserve low-to-high velocity coverage across its projected zones.");
        const auto hasControllerLabel = [&](const int controller, const std::string& label)
        {
            return std::any_of(nakedProjection.instrumentControls.begin(),
                               nakedProjection.instrumentControls.end(),
                [&](const auto& control)
                {
                    return control.importedSourceController == controller
                        && control.displayName == label;
                });
        };
        require(hasControllerLabel(7, "Vol") && hasControllerLabel(10, "Pan")
                    && hasControllerLabel(20, "KickIn")
                    && hasControllerLabel(41, "DirectMics")
                    && hasControllerLabel(42, "Overheads"),
                "Naked Drums GM must preserve its master, kick, and mixer labels.");
        const auto controlIdForController = [&](const int controller)
        {
            const auto found = std::find_if(nakedProjection.instrumentControls.begin(),
                                            nakedProjection.instrumentControls.end(),
                [&](const auto& control)
                {
                    return control.importedSourceController == controller;
                });
            return found == nakedProjection.instrumentControls.end() ? std::string {} : found->id;
        };
        const auto hasGainTarget = [&](const int controller)
        {
            const auto controlId = controlIdForController(controller);
            return !controlId.empty() && std::any_of(nakedProjection.instrumentControlTargets.begin(),
                                                     nakedProjection.instrumentControlTargets.end(),
                [&](const auto& target)
                {
                    return target.controlId == controlId
                        && target.targetKind == RuntimeInstrumentControlTargetKind::gain;
                });
        };
        require(hasGainTarget(7) && hasGainTarget(20) && hasGainTarget(41)
                    && hasGainTarget(42),
                "Naked Drums GM mixer controls must retain master, kick, DI, and overhead gain targets.");

        // The application creates new projects at the layer-contract schema (10/9).
        // Applying an SFZ with controls must promote that destination to schema 11,
        // otherwise appendImportedContent would intentionally preserve the legacy
        // schema and silently drop the imported control catalog.
        auto legacyImportProject = nakedProject;
        legacyImportProject.schemaVersion = layerContractProjectSchemaVersion;
        legacyImportProject.authoring.schemaVersion = layerContractAuthoringSchemaVersion;
        const auto legacyProjection = projectSfzImportDocument(legacyImportProject, nakedDrums);
        require(legacyProjection.projected && !legacyProjection.instrumentControls.empty(),
                "Naked Drums legacy-schema projection must retain imported controls before apply.");
        AuthoringSession legacySession(legacyImportProject);
        const auto appliedLegacyProjection = applySfzImportProjection(
            legacySession, legacyProjection, "Apply Naked Drums GM qualification import");
        require(appliedLegacyProjection.applied,
                "Naked Drums legacy-schema projection must apply successfully.");
        require(legacySession.getProject().schemaVersion == instrumentControlProjectSchemaVersion
                    && legacySession.getProject().authoring.schemaVersion == instrumentControlAuthoringSchemaVersion
                    && legacySession.getProject().authoring.instrumentControls.size() == 26
                    && legacySession.getProject().authoring.midiControlBindings.size() == 26
                    && legacySession.getProject().authoring.instrumentControlTargets.size() == 25,
                "Applying Naked Drums GM to a new project must persist its control catalog and bindings.");
        auto legacyZeroDefaultProject = legacySession.getProject();
        for (auto& control : legacyZeroDefaultProject.authoring.instrumentControls)
            if (control.importedSourceController == 20
                || control.importedSourceController == 41
                || control.importedSourceController == 42)
                control.normalizedDefault = 0.0;
        PlaybackSnapshotBuilder legacyDefaultRepairBuilder;
        const auto repairedSnapshot = legacyDefaultRepairBuilder.buildSnapshot(
            legacyDefaultRepairBuilder.requestBuild(1, true), legacyZeroDefaultProject);
        const auto repairedDefault = [&](const int controller)
        {
            const auto control = std::find_if(repairedSnapshot.snapshot.instrumentControls.begin(),
                                              repairedSnapshot.snapshot.instrumentControls.end(),
                [&](const auto& value) { return value.importedSourceController == controller; });
            return control == repairedSnapshot.snapshot.instrumentControls.end()
                ? -1.0 : control->normalizedDefault;
        };
        require(repairedSnapshot.built && repairedDefault(20) == 1.0
                    && repairedDefault(41) == 1.0 && repairedDefault(42) == 1.0,
                "Previously imported Naked Drums projects with zero fallback gain defaults must publish audibly.");
        const auto hasTargetKind = [&](const RuntimeInstrumentControlTargetKind kind)
        {
            return std::any_of(nakedProjection.instrumentControlTargets.begin(),
                               nakedProjection.instrumentControlTargets.end(),
                [&](const auto& target) { return target.targetKind == kind; });
        };
        require(!hasTargetKind(RuntimeInstrumentControlTargetKind::tune)
                    && !hasTargetKind(RuntimeInstrumentControlTargetKind::envelopeDecay),
                "Naked Drums GM must explicitly classify the absence of tune/decay on-CC targets.");
        std::size_t corpusDocuments = 0;
        const auto corpusRoot = std::filesystem::path(DRS_SOURCE_ROOT) / "tests" / "fixtures";
        for (const auto& entry : std::filesystem::recursive_directory_iterator(corpusRoot))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".sfz")
                continue;
            const auto corpusAnalysis = analyzeSfzImportDocument(entry.path().generic_string());
            require(corpusAnalysis.analyzed && corpusAnalysis.report.available
                        && corpusAnalysis.report.sections.size() == 6,
                    "Every SFZ corpus document must produce the structured controller review sections.");
            ++corpusDocuments;
        }
        require(corpusDocuments >= 3,
                "The wider SFZ qualification corpus must contain multiple controller documents.");
        std::cout << "Instrument control qualification tests passed: parseMs=" << parseElapsed.count()
                  << " analysisMs=" << analysisElapsed.count()
                  << " projectionMs=" << projectionElapsed.count()
                  << " zones=" << nakedProjection.zones.size()
                  << " controls=" << nakedProjection.instrumentControls.size()
                  << " firstSampleBytes=" << firstSampleBytes
                  << " actualGainCc20=" << actualGainCc20.first << "/" << actualGainCc20.second
                  << " actualGainCc41=" << actualGainCc41.first << "/" << actualGainCc41.second
                  << " actualGainCc42=" << actualGainCc42.first << "/" << actualGainCc42.second
                  << " actualGainCc7=" << actualGainCc7.first << "/" << actualGainCc7.second
                  << " corpusDocuments=" << corpusDocuments << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Instrument control qualification tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
