#include "drs/engine/AuthoringPreviewPreparation.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void writeAudioFixture(const fs::path& path, bool flac, int variant)
{
    juce::File file(path.generic_string());
    if (file.existsAsFile())
        require(file.deleteFile(), "Could not replace Preview preparation fixture.");

    auto fileStream = std::make_unique<juce::FileOutputStream>(file);
    require(fileStream->openedOk(), "Could not create Preview preparation fixture.");
    std::unique_ptr<juce::OutputStream> stream = std::move(fileStream);
    std::unique_ptr<juce::AudioFormat> format = flac
        ? std::unique_ptr<juce::AudioFormat>(std::make_unique<juce::FlacAudioFormat>())
        : std::unique_ptr<juce::AudioFormat>(std::make_unique<juce::WavAudioFormat>());
    auto writer = format->createWriterFor(
        stream,
        juce::AudioFormatWriterOptions {}
            .withSampleRate(48000.0)
            .withNumChannels(2)
            .withBitsPerSample(24));
    require(writer != nullptr, "Could not open Preview preparation audio writer.");

    constexpr int frameCount = 24032;
    juce::AudioBuffer<float> buffer(2, frameCount);
    for (int frame = 0; frame < frameCount; ++frame)
    {
        const auto phase = static_cast<float>(frame) / static_cast<float>(frameCount);
        buffer.setSample(0, frame, std::sin(phase * juce::MathConstants<float>::twoPi
                                           * static_cast<float>(3 + variant)) * 0.25f);
        buffer.setSample(1, frame, std::cos(phase * juce::MathConstants<float>::twoPi
                                           * static_cast<float>(5 + variant)) * 0.2f);
    }
    require(writer->writeFromAudioSampleBuffer(buffer, 0, frameCount),
            "Could not write Preview preparation fixture.");
}

drs::engine::PlaybackActivationPayloadPtr preparePreview(
    drs::engine::EngineFacade& facade,
    const drs::engine::RuntimeProjectModel& project,
    std::size_t revision)
{
    require(facade.replaceDraftPlaybackAuthoringProject(project),
            "Facade rejected an authored Preview project.");
    require(facade.stageDraftRevision(revision), "Facade rejected an authored Preview revision.");
    require(facade.refreshPreviewToCurrentDraft(), "Facade rejected Preview preparation.");
    require(facade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(3000)),
            "Preview worker did not settle.");
    facade.serviceBackgroundWork();
    const auto payload = facade.getPreviewActivationPayload();
    require(payload != nullptr && payload->revision == revision,
            "Preview worker did not publish the requested immutable payload.");
    return payload;
}

drs::engine::PlaybackActivationPayloadPtr prepareScopedPreview(
    drs::engine::EngineFacade& facade,
    const drs::engine::RuntimeProjectModel& project,
    std::size_t revision,
    drs::engine::PlaybackPreparationScopeRequest scope)
{
    require(facade.replaceDraftPlaybackAuthoringProject(project),
            "Facade rejected a scoped authored Preview project.");
    require(facade.stageDraftRevision(revision), "Facade rejected a scoped Preview revision.");
    require(facade.refreshPreviewForPreparationScope(scope),
            "Facade rejected scoped Preview preparation.");
    require(facade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(3000)),
            "Scoped Preview worker did not settle.");
    facade.serviceBackgroundWork();
    const auto payload = facade.getPreviewActivationPayload();
    require(payload != nullptr && payload->revision == revision,
            "Scoped Preview worker did not publish the requested immutable payload.");
    return payload;
}

drs::engine::AuthoringPreviewRequest makeRequest(
    drs::engine::AuthoringPreviewScope scope,
    std::size_t revision,
    std::string selectedZoneId = {},
    std::string selectedGroupId = {})
{
    drs::engine::AuthoringPreviewRequest request;
    request.identity.requestId = scope == drs::engine::AuthoringPreviewScope::selectedZone
        ? 1
        : (scope == drs::engine::AuthoringPreviewScope::selectedGroup ? 2 : 3);
    request.identity.draftRevision = revision;
    request.identity.scope = scope;
    request.identity.selectedZoneId = std::move(selectedZoneId);
    request.identity.selectedGroupId = std::move(selectedGroupId);
    request.reason = scope == drs::engine::AuthoringPreviewScope::selectedZone
        ? drs::engine::AuthoringPreviewRequestReason::explicitSelectedZoneAudition
        : (scope == drs::engine::AuthoringPreviewScope::selectedGroup
               ? drs::engine::AuthoringPreviewRequestReason::explicitSelectedGroupAudition
               : drs::engine::AuthoringPreviewRequestReason::explicitCurrentDraftAudition);
    return request;
}

bool hasFinding(const drs::engine::AuthoringPreviewPreparationResult& result,
                const std::string& code)
{
    return std::any_of(result.findings.begin(), result.findings.end(),
                       [&](const auto& finding) { return finding.code == code; });
}

std::string summarizeFindings(const drs::engine::AuthoringPreviewPreparationResult& result)
{
    if (result.findings.empty())
        return "no findings";

    std::string summary;
    for (std::size_t index = 0; index < result.findings.size() && index < 3; ++index)
    {
        if (!summary.empty())
            summary += " | ";

        const auto& finding = result.findings[index];
        summary += "[" + finding.code + "] " + finding.message + " (" + finding.path + ")";
    }
    return summary;
}

const drs::engine::PreparedPlaybackSampleHandle& findSample(
    const drs::engine::PlaybackActivationPayloadPtr& payload,
    const std::string& sourceId)
{
    const auto iterator = std::find_if(payload->prepared->samples.begin(), payload->prepared->samples.end(),
                                       [&](const auto& sample) { return sample.sampleSourceId == sourceId; });
    require(iterator != payload->prepared->samples.end(), "Prepared source was not retained: " + sourceId);
    return *iterator;
}
} // namespace

int main()
{
    using namespace drs::engine;
    const auto scratch = fs::temp_directory_path() / "drs-sprint5-preview-preparation";
    try
    {
        fs::create_directories(scratch);
        const auto wavPath = scratch / "selected-zone.wav";
        const auto flacPath = scratch / "current-draft.flac";
        const auto relinkPath = scratch / "selected-zone-relink.wav";
        writeAudioFixture(wavPath, false, 0);
        writeAudioFixture(flacPath, true, 1);
        writeAudioFixture(relinkPath, false, 2);

        const auto loaded = loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Mini Sprint 5.4 requires the Phase 2 authored reference project.");
        auto project = loaded.project;
        project.sampleSources[0].path = wavPath.generic_string();
        project.sampleSources[1].path = flacPath.generic_string();

        PlaybackSnapshotBuildResult closureSource;
        closureSource.built = true;
        closureSource.activationEligible = true;
        closureSource.lifecycleState = PlaybackSnapshotLifecycleState::ready;
        closureSource.snapshot.draftRevision = 200;
        closureSource.snapshot.sampleIdentities = {
            { "selected-source", "selected.wav", "sustain" },
            { "xfade-source", "xfade.wav", "sustain" },
            { "rr-source", "rr.wav", "sustain" },
            { "release-source", "release.wav", "release" },
            { "unrelated-source", "unrelated.wav", "sustain" }
        };
        PlaybackSnapshotZone selectedClosureZone;
        selectedClosureZone.id = "selected";
        selectedClosureZone.sampleSourceId = "selected-source";
        selectedClosureZone.groupId = "selected-group";
        selectedClosureZone.articulationId = "main";
        selectedClosureZone.keyLow = 48;
        selectedClosureZone.keyHigh = 72;
        selectedClosureZone.velocityCrossfadeRuntime.fadeInNeighborZoneId = "xfade";
        selectedClosureZone.roundRobin = RoundRobinDescriptor { "selected-pool", 2, 0,
                                                                RoundRobinMode::sequential };
        auto xfadeClosureZone = selectedClosureZone;
        xfadeClosureZone.id = "xfade";
        xfadeClosureZone.sampleSourceId = "xfade-source";
        xfadeClosureZone.velocityCrossfadeRuntime = {};
        xfadeClosureZone.roundRobin.reset();
        auto rrClosureZone = selectedClosureZone;
        rrClosureZone.id = "rr";
        rrClosureZone.sampleSourceId = "rr-source";
        rrClosureZone.velocityCrossfadeRuntime = {};
        rrClosureZone.roundRobin->slotIndex = 1;
        auto releaseClosureZone = selectedClosureZone;
        releaseClosureZone.id = "release";
        releaseClosureZone.sampleSourceId = "release-source";
        releaseClosureZone.groupId = "release-group";
        releaseClosureZone.velocityCrossfadeRuntime = {};
        releaseClosureZone.roundRobin.reset();
        releaseClosureZone.performance.event = PerformanceEventKind::release;
        auto unrelatedClosureZone = selectedClosureZone;
        unrelatedClosureZone.id = "unrelated";
        unrelatedClosureZone.sampleSourceId = "unrelated-source";
        unrelatedClosureZone.groupId = "unrelated-group";
        unrelatedClosureZone.articulationId = "other";
        unrelatedClosureZone.keyLow = 80;
        unrelatedClosureZone.keyHigh = 90;
        unrelatedClosureZone.velocityCrossfadeRuntime = {};
        unrelatedClosureZone.roundRobin.reset();
        closureSource.snapshot.zones = { selectedClosureZone,
                                         xfadeClosureZone,
                                         rrClosureZone,
                                         releaseClosureZone,
                                         unrelatedClosureZone };
        closureSource.snapshot.groupRoutes = {
            { "selected-group", { "main" }, { "selected", "xfade", "rr" } },
            { "release-group", { "main" }, { "release" } },
            { "unrelated-group", { "other" }, { "unrelated" } }
        };
        closureSource.snapshot.routingBuses = {
            { "selected-bus", "Selected", "groups/selected-group" },
            { "release-bus", "Release", "groups/release-group" },
            { "unrelated-bus", "Unrelated", "groups/unrelated-group" },
            { "master-bus", "Master", "instrument/master" }
        };
        closureSource.snapshot.contentDigest
            = computePlaybackSnapshotContentDigest(closureSource.snapshot);
        const auto dependencyClosure = scopePlaybackSnapshotForPreparation(
            closureSource,
            { PlaybackPreparationScope::selectedZone, "selected", {} });
        require(dependencyClosure.built
                    && dependencyClosure.unscopedZoneCount == 5
                    && dependencyClosure.retainedZoneCount == 4
                    && dependencyClosure.unscopedSampleCount == 5
                    && dependencyClosure.retainedSampleCount == 4
                    && std::none_of(dependencyClosure.snapshot.zones.begin(),
                                    dependencyClosure.snapshot.zones.end(),
                                    [](const auto& zone) { return zone.id == "unrelated"; })
                    && std::none_of(dependencyClosure.snapshot.routingBuses.begin(),
                                    dependencyClosure.snapshot.routingBuses.end(),
                                    [](const auto& bus) { return bus.id == "unrelated-bus"; })
                    && std::any_of(dependencyClosure.snapshot.routingBuses.begin(),
                                   dependencyClosure.snapshot.routingBuses.end(),
                                   [](const auto& bus) { return bus.id == "master-bus"; }),
                "Selected-zone dependency closure must retain crossfade, round-robin, release, group-routing, and inherited master DSP dependencies only.");
        const auto repeatedDependencyClosure = scopePlaybackSnapshotForPreparation(
            closureSource,
            { PlaybackPreparationScope::selectedZone, "selected", {} });
        require(repeatedDependencyClosure.snapshot.contentDigest
                    == dependencyClosure.snapshot.contentDigest,
                "Equivalent pre-realization dependency closures must remain deterministic.");

        EngineFacade facade;
        const auto performanceBefore = facade.getPerformanceActivationPayload();
        auto payload = preparePreview(facade, project, 101);
        require(payload->snapshot->zones.size() == 3 && payload->prepared->samples.size() == 2,
                "Current-draft worker payload must retain every eligible authored route and source.");
        const auto& wavSample = findSample(payload, "sine-a3");
        const auto& flacSample = findSample(payload, "triangle-a4");
        require(wavSample.formatName == "WAV file" && flacSample.formatName == "FLAC file"
                    && !wavSample.sourcePath.empty() && !flacSample.canonicalSourcePath.empty()
                    && !wavSample.sourceFingerprintHex.empty() && !flacSample.sourceFingerprintHex.empty()
                    && wavSample.decodedSampleData != nullptr && flacSample.decodedSampleData != nullptr,
                "WAV/FLAC provenance, fingerprints, and immutable decoded ownership must survive preparation.");

        const auto selectedRequest = makeRequest(AuthoringPreviewScope::selectedZone, 101,
                                                 "lead-a4-sustain");
        const auto selected = prepareAuthoringPreviewRenderModel(payload, selectedRequest);
        require(selected.prepared,
                "Selected-zone preparation should succeed. Findings: " + summarizeFindings(selected));
        require(selected.validatedZoneCount == 3,
                "Selected-zone preparation should validate the full authored topology first.");
        require(selected.retainedZoneCount == 1,
                "Selected-zone preparation should expose one audible route.");
        require(selected.retainedSampleCount == 1,
                "Selected-zone preparation should expose one retained sample for this fixture.");
        require(selected.scopedPayload->snapshot->zones.front().id == "lead-a4-sustain",
                "Selected-zone preparation should keep the requested zone first in the scoped snapshot.");
        require(selected.scopedPayload->prepared->samples.front().sampleSourceId == "triangle-a4",
                "Selected-zone preparation should keep the selected sample source.");
        require(selected.scopedPayload->snapshot->macroDefaults.size()
                    == payload->snapshot->macroDefaults.size(),
                "Selected-zone preparation should preserve macro defaults.");
        require(selected.model->getRetainedActivationPayload() == selected.scopedPayload,
                "Selected-zone render models should retain the scoped immutable payload.");
        require(selected.scopedPayload->prepared->samples.front().decodedSampleData
                    == flacSample.decodedSampleData
                    && selected.scopedPayload->prepared->samples.front().sourceFingerprintHex
                        == flacSample.sourceFingerprintHex,
                "Selected-zone filtering must preserve decoded ownership and source provenance without copying PCM.");

        const auto repeatedSelected = prepareAuthoringPreviewRenderModel(payload, selectedRequest);
        require(repeatedSelected.prepared
                    && repeatedSelected.scopedPayload->snapshotContentDigest
                        == selected.scopedPayload->snapshotContentDigest
                    && repeatedSelected.scopedPayload->preparedContentDigest
                        == selected.scopedPayload->preparedContentDigest
                    && repeatedSelected.model->getRoutes().front().zoneId
                        == selected.model->getRoutes().front().zoneId,
                "Equivalent selected-zone preparation must have deterministic immutable identity.");

        const auto selectedGroup = prepareAuthoringPreviewRenderModel(
            payload,
            makeRequest(AuthoringPreviewScope::selectedGroup, 101, "pad-a3-low", "pad-core"));
        require(selectedGroup.prepared && selectedGroup.validatedZoneCount == 3
                    && selectedGroup.retainedZoneCount == 2
                    && selectedGroup.retainedSampleCount == 1
                    && selectedGroup.scopedPayload->snapshot->selectedGroupId == "pad-core"
                    && selectedGroup.scopedPayload->snapshot->zones.size() == 2
                    && selectedGroup.model->getRoutes().size() == 2,
                "Selected-group preparation must retain the whole selected group while keeping Preview scoped.");

        const auto currentDraft = prepareAuthoringPreviewRenderModel(
            payload, makeRequest(AuthoringPreviewScope::currentDraft, 101));
        require(currentDraft.prepared && currentDraft.retainedZoneCount == 3
                    && currentDraft.retainedSampleCount == 2
                    && currentDraft.scopedPayload == payload
                    && currentDraft.model->getRoutes().size() == 3,
                "Current-draft preparation must retain all Preview-eligible authored routes.");
        require(facade.getPerformanceActivationPayload() == performanceBefore,
                "Preview preparation must not publish or replace Performance.");

        EngineFacade scopedZoneFacade;
        PlaybackPreparationScopeRequest selectedZoneScope;
        selectedZoneScope.scope = PlaybackPreparationScope::selectedZone;
        selectedZoneScope.selectedZoneId = "lead-a4-sustain";
        const auto scopedZonePayload = prepareScopedPreview(
            scopedZoneFacade, project, 201, selectedZoneScope);
        require(scopedZonePayload->preparationScope == PlaybackPreparationScope::selectedZone
                    && scopedZonePayload->preparationSelectedZoneId == "lead-a4-sustain"
                    && scopedZonePayload->unscopedZoneCount == 3
                    && scopedZonePayload->retainedZoneCount == 1
                    && scopedZonePayload->unscopedSampleCount == 2
                    && scopedZonePayload->retainedSampleCount == 1
                    && scopedZonePayload->snapshot->zones.size() == 1
                    && scopedZonePayload->prepared->zones.size() == 1
                    && scopedZonePayload->prepared->samples.size() == 1
                    && scopedZonePayload->prepared->samples.front().sampleSourceId == "triangle-a4",
                "Selected-zone scope must trim snapshot identities before production preparation decodes them.");
        const auto selectedZoneDigest = scopedZonePayload->snapshotContentDigest;
        require(scopedZoneFacade.refreshPreviewForPreparationScope(selectedZoneScope)
                    && scopedZoneFacade.getPreviewActivationPayload() == scopedZonePayload
                    && scopedZoneFacade.getPreviewActivationPayload()->snapshotContentDigest
                        == selectedZoneDigest,
                "Equivalent scoped requests must reuse deterministic prepared identity.");

        EngineFacade scopedGroupFacade;
        PlaybackPreparationScopeRequest selectedGroupScope;
        selectedGroupScope.scope = PlaybackPreparationScope::selectedGroup;
        selectedGroupScope.selectedGroupId = "pad-core";
        const auto scopedGroupPayload = prepareScopedPreview(
            scopedGroupFacade, project, 202, selectedGroupScope);
        require(scopedGroupPayload->preparationScope == PlaybackPreparationScope::selectedGroup
                    && scopedGroupPayload->preparationSelectedGroupId == "pad-core"
                    && scopedGroupPayload->retainedZoneCount == 2
                    && scopedGroupPayload->retainedSampleCount == 1
                    && scopedGroupPayload->snapshot->zones.size() == 2
                    && scopedGroupPayload->prepared->samples.size() == 1,
                "Selected-group scope must prepare only its retained dependency set.");

        EngineFacade explicitDraftFacade;
        const auto explicitDraftPayload = prepareScopedPreview(
            explicitDraftFacade, project, 203, {});
        require(explicitDraftPayload->preparationScope == PlaybackPreparationScope::currentDraft
                    && explicitDraftPayload->retainedZoneCount == 3
                    && explicitDraftPayload->retainedSampleCount == 2
                    && explicitDraftPayload->prepared->samples.size() == 2,
                "Current-draft Preview must remain an explicit full-scope request.");

        auto invalidSnapshot = *payload->snapshot;
        invalidSnapshot.zones[1].id = invalidSnapshot.zones[0].id;
        invalidSnapshot.contentDigest = computePlaybackSnapshotContentDigest(invalidSnapshot);
        auto invalidPrepared = *payload->prepared;
        invalidPrepared.snapshotContentDigest = invalidSnapshot.contentDigest;
        invalidPrepared.preparedContentDigest = computePreparedPlaybackContentDigest(invalidPrepared);
        auto invalidPayload = std::make_shared<PlaybackActivationPayload>(*payload);
        invalidPayload->snapshotContentDigest = invalidSnapshot.contentDigest;
        invalidPayload->preparedContentDigest = invalidPrepared.preparedContentDigest;
        invalidPayload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(invalidSnapshot));
        invalidPayload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(invalidPrepared));
        const auto rejected = prepareAuthoringPreviewRenderModel(invalidPayload, selectedRequest);
        require(!rejected.prepared && hasFinding(rejected, "render-model-snapshot-zone-id-duplicate"),
                "Invalid off-route topology must fail before selected-zone filtering.");

        auto mappedProject = project;
        auto& mappedZone = mappedProject.authoring.zones[2];
        mappedZone.rootKey = 71;
        mappedZone.keyLow = 55;
        mappedZone.keyHigh = 91;
        mappedZone.velocityLow = 12;
        mappedZone.velocityHigh = 119;
        mappedZone.gainDb = -2.25;
        mappedZone.pan = 0.4;
        mappedZone.sampleStartFrame = 64;
        mappedZone.loopEnabled = true;
        mappedZone.loopStartFrame = 256;
        mappedZone.loopEndFrame = 4096;
        auto mappedPayload = preparePreview(facade, mappedProject, 102);
        const auto mapped = prepareAuthoringPreviewRenderModel(
            mappedPayload, makeRequest(AuthoringPreviewScope::selectedZone, 102, mappedZone.id));
        const auto& route = mapped.model->getRoutes().front();
        const auto& authoredRoute = mapped.scopedPayload->prepared->zones.front();
        require(mapped.prepared && route.rootKey == 71 && route.keyLow == 0 && route.keyHigh == 127
                    && route.velocityLow == 1 && route.velocityHigh == 127
                    && authoredRoute.keyLow == 55 && authoredRoute.keyHigh == 91
                    && authoredRoute.velocityLow == 12 && authoredRoute.velocityHigh == 119
                    && route.gainDb == -2.25 && route.pan == 0.4
                    && route.sampleStartFrame == 64 && route.loopEnabled
                    && route.loopStartFrame == 256 && route.loopEndFrame == 4096,
                "Mapping, gain, pan, root, bounds, velocity, start, and loop edits must reach Preview normalization.");
        require(facade.getDraftPlaybackStatus().preview.preparationCacheHitCount == 2
                    && facade.getDraftPlaybackStatus().preview.preparationCacheMissCount == 0,
                "Zone-only authored edits must reuse warm decoded source ownership.");

        const auto originalFingerprint = findSample(mappedPayload, "sine-a3").sourceFingerprintHex;
        auto relinkedProject = mappedProject;
        relinkedProject.sampleSources[0].path = relinkPath.generic_string();
        auto relinkedPayload = preparePreview(facade, relinkedProject, 103);
        const auto relinkedFingerprint = findSample(relinkedPayload, "sine-a3").sourceFingerprintHex;
        require(relinkedFingerprint != originalFingerprint
                    && facade.getDraftPlaybackStatus().preview.preparationCacheMissCount >= 1,
                "Source relink must invalidate stale decoded ownership and fingerprint identity.");

        writeAudioFixture(relinkPath, false, 5);
        auto replacedPayload = preparePreview(facade, relinkedProject, 104);
        require(findSample(replacedPayload, "sine-a3").sourceFingerprintHex != relinkedFingerprint
                    && facade.getDraftPlaybackStatus().preview.preparationCacheMissCount >= 1,
                "Same-path source replacement must invalidate stale cache identity.");

        fs::remove_all(scratch);
        std::cout << "Mini Sprint 5.4 selected-zone/current-draft preparation matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        fs::remove_all(scratch);
        std::cerr << "Mini Sprint 5.4 Preview preparation matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
