#include "drs/engine/AuthoringSession.h"
#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PerformancePublishPreparation.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

enum class FixtureFormat { wav, flac };
enum class WorkLane { preview, publish };

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string toString(FixtureFormat format) { return format == FixtureFormat::wav ? "wav" : "flac"; }
std::string toString(WorkLane lane) { return lane == WorkLane::preview ? "preview" : "publish"; }

FixtureFormat parseFormat(const std::string& value)
{
    if (value == "wav") return FixtureFormat::wav;
    if (value == "flac") return FixtureFormat::flac;
    throw std::runtime_error("Expected fixture format 'wav' or 'flac'.");
}

WorkLane parseLane(const std::string& value)
{
    if (value == "preview") return WorkLane::preview;
    if (value == "publish") return WorkLane::publish;
    throw std::runtime_error("Expected work lane 'preview' or 'publish'.");
}

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-sprint4-entry-authored-input-tests";
    fs::create_directories(path);
    return path;
}

juce::AudioBuffer<float> buildFixtureBuffer(int variant)
{
    constexpr int frameCount = 960;
    juce::AudioBuffer<float> buffer(2, frameCount);
    for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
    {
        const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
        buffer.setSample(0, sampleIndex,
                         std::sin(phase * juce::MathConstants<float>::twoPi * (3.0f + variant)) * 0.3f);
        buffer.setSample(1, sampleIndex,
                         std::cos(phase * juce::MathConstants<float>::twoPi * (5.0f + variant)) * 0.2f);
    }
    return buffer;
}

void writeFixture(const fs::path& path, FixtureFormat fixtureFormat, int variant = 0)
{
    juce::File fixtureFile(path.generic_string());
    if (fixtureFile.existsAsFile())
        require(fixtureFile.deleteFile(), "Could not replace authored-input fixture: " + path.generic_string());

    auto fileOutput = std::make_unique<juce::FileOutputStream>(fixtureFile);
    require(fileOutput->openedOk(), "Could not create authored-input fixture: " + path.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);
    std::unique_ptr<juce::AudioFormat> format = fixtureFormat == FixtureFormat::wav
        ? std::unique_ptr<juce::AudioFormat>(std::make_unique<juce::WavAudioFormat>())
        : std::unique_ptr<juce::AudioFormat>(std::make_unique<juce::FlacAudioFormat>());

    auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(48000.0)
        .withNumChannels(2)
        .withBitsPerSample(24);
    auto writer = format->createWriterFor(output, options);
    require(writer != nullptr, "Could not create authored-input writer: " + path.generic_string());
    const auto buffer = buildFixtureBuffer(variant);
    require(writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write authored-input fixture: " + path.generic_string());
}

void writeUnsupportedFixture(const fs::path& path)
{
    juce::File fixtureFile(path.generic_string());
    if (fixtureFile.existsAsFile())
        require(fixtureFile.deleteFile(), "Could not replace unsupported fixture.");
    std::ofstream output(path, std::ios::binary);
    output << "not an audio file";
    require(output.good(), "Could not write unsupported fixture.");
}

bool containsFinding(const drs::engine::PreparedPlaybackBuildResult& result, const std::string& code)
{
    for (const auto& finding : result.findings)
        if (finding.severity == drs::engine::PlaybackSnapshotFindingSeverity::error && finding.code == code)
            return true;
    return false;
}

bool containsFinding(const drs::engine::DraftPlaybackPreparedRevision& revision, const std::string& code)
{
    for (const auto& finding : revision.findings)
        if (finding.severity == drs::engine::PlaybackSnapshotFindingSeverity::error && finding.code == code)
            return true;
    return false;
}

const drs::engine::PreparedPlaybackSampleHandle& findPreparedSample(
    const drs::engine::PreparedPlaybackBuildResult& result,
    const std::string& sampleSourceId)
{
    for (const auto& sample : result.prepared.samples)
        if (sample.sampleSourceId == sampleSourceId)
            return sample;
    throw std::runtime_error("Prepared sample was not found: " + sampleSourceId);
}

const drs::engine::PreparedPlaybackStreamHandle& findPreparedStream(
    const drs::engine::PreparedPlaybackBuildResult& result,
    const std::string& sampleSourceId)
{
    for (const auto& stream : result.prepared.streams)
        if (stream.sampleSourceId == sampleSourceId)
            return stream;
    throw std::runtime_error("Prepared stream was not found: " + sampleSourceId);
}

drs::engine::RuntimeProjectModel buildAuthoredProject(const fs::path& fixturePath,
                                                      const std::string& sampleSourceId)
{
    const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
    require(phase2Project.loaded, "EG1 requires the Phase 2 reference project as an authoring baseline.");

    drs::engine::RuntimeProjectSampleSource source;
    source.id = sampleSourceId;
    source.path = fixturePath.generic_string();
    source.role = "entry-gate-authored-source";

    drs::engine::RuntimeProjectZoneDefinition zone;
    zone.id = "zone-" + sampleSourceId;
    zone.sampleSourceId = sampleSourceId;
    zone.displayName = "Entry Gate Authored Source";
    zone.groupId = "main";
    zone.articulationId = "sustain";
    zone.rootKey = 60;
    zone.keyLow = 60;
    zone.keyHigh = 60;
    zone.velocityLow = 1;
    zone.velocityHigh = 127;

    drs::engine::AuthoringSession session(phase2Project.project);
    const auto appended = session.appendImportedContent({ source }, { zone }, "Add Sprint 4 entry-gate source");
    require(appended.applied, "Could not append the external authored source.");
    return session.getProject();
}

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(const drs::engine::RuntimeProjectModel& project,
                                                       std::size_t revision,
                                                       WorkLane lane)
{
    drs::engine::PlaybackSnapshotBuilder builder;
    const auto request = builder.requestBuild(revision, lane == WorkLane::publish);
    require(request.accepted, "Authored-input snapshot request should be accepted.");
    const auto snapshot = builder.buildSnapshot(request, project);
    require(snapshot.built && snapshot.activationEligible,
            "Authored input should produce a valid immutable snapshot before preparation.");
    return snapshot;
}

drs::engine::PreparedPlaybackBuildResult prepare(drs::engine::PreparedPlaybackService& service,
                                                 const drs::engine::PlaybackSnapshotBuildResult& snapshot,
                                                 WorkLane lane,
                                                 const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto queued = lane == WorkLane::preview
        ? service.enqueuePreviewBuild(snapshot)
        : service.enqueuePublishBuild(snapshot);
    require(queued.accepted, "Authored preparation should enter the requested worker lane.");
    const auto processed = service.processNextQueuedBuild(referenceStream);
    require(processed.processed, "Authored preparation should execute on the worker.");
    require(processed.lane == (lane == WorkLane::preview
                                   ? drs::engine::PreparedPlaybackWorkLane::preview
                                   : drs::engine::PreparedPlaybackWorkLane::performance),
            "Authored preparation completed on the wrong worker lane.");
    return processed.result;
}

drs::engine::RuntimeStreamLoadResult loadReferenceStream()
{
    const auto manifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
    require(manifest.loaded, "EG1 requires the Phase 1 reference manifest.");
    const auto stream = drs::engine::loadRuntimeStreamContainerForInstrument(manifest);
    require(stream.loaded, "EG1 requires the reference stream to verify optional topology behavior.");
    return stream;
}

void runNewSourceScenario(FixtureFormat format, WorkLane lane,
                          const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto suffix = toString(format) + "-" + toString(lane);
    const auto sourceId = "eg1-external-" + suffix;
    const auto path = getScratchDirectory() / ("external-" + suffix + "." + toString(format));
    writeFixture(path, format);
    const auto project = buildAuthoredProject(path, sourceId);
    const auto snapshot = buildSnapshot(project, 1, lane);

    drs::engine::PreparedPlaybackService service;
    const auto cold = prepare(service, snapshot, lane, referenceStream);
    require(cold.built && cold.activationEligible, "New authored source should prepare successfully.");
    require(!cold.prepared.routeDigest.empty()
                && !cold.prepared.sourceProvenanceDigest.empty()
                && !cold.prepared.macroSchemaDigest.empty(),
            "Worker preparation must carry deterministic Publish topology, provenance, and macro digests.");
    require(findPreparedSample(cold, sourceId).decodedSampleData != nullptr,
            "New authored source must retain decoded PCM.");
    const auto& stream = findPreparedStream(cold, sourceId);
    require(!stream.compiledStreamTopologyAvailable && stream.topologyKind == "decoded-memory",
            "New authored source should use the optional decoded-memory topology.");
    require(cold.metrics.cacheMissCount == project.sampleSources.size(),
            "First preparation should cold-miss each project sample.");

    const auto warm = prepare(service, snapshot, lane, referenceStream);
    require(warm.built && warm.metrics.cacheHitCount == project.sampleSources.size()
                && warm.metrics.cacheMissCount == 0,
            "Unchanged authored input should deterministically reuse the warm cache.");
    require(warm.prepared.routeDigest == cold.prepared.routeDigest
                && warm.prepared.sourceProvenanceDigest == cold.prepared.sourceProvenanceDigest
                && warm.prepared.macroSchemaDigest == cold.prepared.macroSchemaDigest,
            "Warm and cold authored WAV/FLAC preparation must produce identical conformance digests.");

    if (lane == WorkLane::publish)
    {
        drs::engine::PerformancePublishRequestIdentity identity;
        identity.requestId = 1;
        identity.cancellationGeneration = 1;
        identity.projectGeneration = 1;
        identity.draftRevision = snapshot.snapshot.draftRevision;
        identity.authoredContentDigest = snapshot.snapshot.contentDigest;
        identity.macroSchemaDigest = drs::engine::computePlaybackSnapshotMacroSchemaDigest(snapshot.snapshot);
        const auto conformance = drs::engine::validatePerformancePublishPreparation(identity, snapshot, cold);
        require(conformance.completeProject && conformance.activationEligible,
                "General authored WAV/FLAC Publish input must pass whole-project immutable conformance.");
    }

    drs::engine::DraftPlaybackContract contract(1);
    const auto shellRequest = lane == WorkLane::preview
        ? contract.requestPreviewBuild()
        : contract.requestPerformanceBuild();
    const auto completed = lane == WorkLane::preview
        ? contract.completePreviewBuild(shellRequest.requestId, snapshot, cold)
        : contract.completePerformanceBuild(shellRequest.requestId, snapshot, cold);
    require(completed, "Frozen shell lane should accept successful authored preparation.");
    const auto& shellRevision = lane == WorkLane::preview
        ? contract.getStatus().preview
        : contract.getStatus().performance;
    require(shellRevision.available && shellRevision.activationEligible
                && shellRevision.revision == 1
                && shellRevision.contentDigest == snapshot.snapshot.contentDigest
                && shellRevision.preparedContentDigest == cold.prepared.preparedContentDigest
                && shellRevision.preparedSampleCount == cold.metrics.preparedSampleCount
                && shellRevision.preparationCacheMissCount == cold.metrics.cacheMissCount,
            "Frozen shell lane must preserve authored revision, digests, and prepared metrics.");
}

void runReplacementAndRelinkScenarios(const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto replacementPath = getScratchDirectory() / "same-path-replacement.wav";
    const std::string replacementId = "eg1-same-path-replacement";
    writeFixture(replacementPath, FixtureFormat::wav, 0);
    auto replacementProject = buildAuthoredProject(replacementPath, replacementId);
    drs::engine::PreparedPlaybackService replacementService;
    const auto first = prepare(replacementService, buildSnapshot(replacementProject, 1, WorkLane::preview),
                               WorkLane::preview, referenceStream);
    const auto firstSample = findPreparedSample(first, replacementId);
    writeFixture(replacementPath, FixtureFormat::wav, 3);
    const auto replaced = prepare(replacementService, buildSnapshot(replacementProject, 2, WorkLane::preview),
                                  WorkLane::preview, referenceStream);
    const auto replacedSample = findPreparedSample(replaced, replacementId);
    require(replaced.built && replaced.metrics.cacheMissCount == 1,
            "Same-path replacement should cold-miss only the changed authored source.");
    require(firstSample.sourceFingerprintHex != replacedSample.sourceFingerprintHex
                && firstSample.cacheKey != replacedSample.cacheKey,
            "Same-path replacement must derive a new cache key from actual bytes.");

    const auto relinkA = getScratchDirectory() / "relink-a.wav";
    const auto relinkB = getScratchDirectory() / "relink-b.wav";
    const std::string relinkId = "eg1-relinked-source";
    writeFixture(relinkA, FixtureFormat::wav, 4);
    writeFixture(relinkB, FixtureFormat::wav, 4);
    auto relinkProject = buildAuthoredProject(relinkA, relinkId);
    drs::engine::PreparedPlaybackService relinkService;
    const auto beforeRelink = prepare(relinkService, buildSnapshot(relinkProject, 1, WorkLane::publish),
                                      WorkLane::publish, referenceStream);
    for (auto& source : relinkProject.sampleSources)
        if (source.id == relinkId)
            source.path = relinkB.generic_string();
    const auto afterRelink = prepare(relinkService, buildSnapshot(relinkProject, 2, WorkLane::publish),
                                     WorkLane::publish, referenceStream);
    require(afterRelink.built && afterRelink.metrics.cacheMissCount == 1,
            "Relinking should cold-miss only the authored source whose canonical identity changed.");
    require(findPreparedSample(beforeRelink, relinkId).cacheKey
                != findPreparedSample(afterRelink, relinkId).cacheKey,
            "Relinking must change cache identity even when file bytes are identical.");
}

void runWithoutCompiledContainerScenario()
{
    const auto path = getScratchDirectory() / "without-compiled-container.flac";
    const std::string sourceId = "eg1-without-compiled-container";
    writeFixture(path, FixtureFormat::flac, 2);
    const auto project = buildAuthoredProject(path, sourceId);
    const auto snapshot = buildSnapshot(project, 1, WorkLane::preview);
    drs::engine::PreparedPlaybackService service;
    const drs::engine::RuntimeStreamLoadResult noCompiledContainer;
    const auto prepared = prepare(service, snapshot, WorkLane::preview, noCompiledContainer);
    require(prepared.built && prepared.activationEligible
                && prepared.metrics.preparedSampleCount == project.sampleSources.size(),
            "Authored preparation must not require any loaded compiled stream container.");
    require(findPreparedStream(prepared, sourceId).topologyKind == "decoded-memory",
            "Container-free authored preparation should publish decoded-memory topology.");
}

void runFailureAndCancellationScenarios(const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto missingPath = getScratchDirectory() / "missing-after-snapshot.wav";
    const std::string missingId = "eg1-missing-source";
    writeFixture(missingPath, FixtureFormat::wav);
    const auto missingSnapshot = buildSnapshot(buildAuthoredProject(missingPath, missingId), 1, WorkLane::preview);
    require(juce::File(missingPath.generic_string()).deleteFile(), "Could not remove missing-source fixture.");
    drs::engine::PreparedPlaybackService missingService;
    const auto missing = prepare(missingService, missingSnapshot, WorkLane::preview, referenceStream);
    require(!missing.built && containsFinding(missing, "prepared-sample-source-missing"),
            "Missing source must produce the structured worker finding.");

    drs::engine::DraftPlaybackContract previewContract(1);
    const auto previewRequest = previewContract.requestPreviewBuild();
    require(previewContract.completePreviewBuild(previewRequest.requestId, missingSnapshot, missing)
                && containsFinding(previewContract.getStatus().preview, "prepared-sample-source-missing"),
            "Preview shell failure state must preserve the actionable authored finding.");

    const auto unsupportedPath = getScratchDirectory() / "unsupported-after-snapshot.wav";
    const std::string unsupportedId = "eg1-unsupported-source";
    writeFixture(unsupportedPath, FixtureFormat::wav);
    const auto unsupportedSnapshot = buildSnapshot(buildAuthoredProject(unsupportedPath, unsupportedId),
                                                   1, WorkLane::publish);
    writeUnsupportedFixture(unsupportedPath);
    drs::engine::PreparedPlaybackService unsupportedService;
    const auto unsupported = prepare(unsupportedService, unsupportedSnapshot, WorkLane::publish, referenceStream);
    require(!unsupported.built && containsFinding(unsupported, "prepared-sample-format-unsupported"),
            "Unsupported source must produce the structured worker finding.");

    drs::engine::DraftPlaybackContract publishContract(1);
    const auto publishRequest = publishContract.requestPerformanceBuild();
    require(publishContract.completePerformanceBuild(publishRequest.requestId, unsupportedSnapshot, unsupported)
                && containsFinding(publishContract.getStatus().performance, "prepared-sample-format-unsupported"),
            "Publish shell failure state must preserve the actionable authored finding.");

    const auto cancelPath = getScratchDirectory() / "queued-cancellation.wav";
    writeFixture(cancelPath, FixtureFormat::wav);
    const auto cancelSnapshot = buildSnapshot(buildAuthoredProject(cancelPath, "eg1-canceled-source"),
                                              1, WorkLane::preview);
    drs::engine::PreparedPlaybackService cancelService;
    require(cancelService.enqueuePreviewBuild(cancelSnapshot).accepted,
            "Cancellation scenario should enqueue preview work.");
    const auto canceled = cancelService.cancelQueuedPreviewBuilds("EG1 queued authored preparation canceled");
    require(canceled.size() == 1
                && canceled.front().lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::canceled
                && canceled.front().metrics.cancellationCount == 1
                && !cancelService.hasPendingQueuedBuilds()
                && !cancelService.processNextQueuedBuild(referenceStream).processed,
            "Queued cancellation must leave no stale authored completion or orphaned work.");
}

void runFullSuite()
{
    const auto referenceStream = loadReferenceStream();
    runNewSourceScenario(FixtureFormat::wav, WorkLane::preview, referenceStream);
    runNewSourceScenario(FixtureFormat::wav, WorkLane::publish, referenceStream);
    runNewSourceScenario(FixtureFormat::flac, WorkLane::preview, referenceStream);
    runNewSourceScenario(FixtureFormat::flac, WorkLane::publish, referenceStream);
    runWithoutCompiledContainerScenario();
    runReplacementAndRelinkScenarios(referenceStream);
    runFailureAndCancellationScenarios(referenceStream);
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        if (argc == 1)
        {
            runFullSuite();
            std::cout << "Sprint 4 Entry Gate EG1 authored-input matrix passed." << std::endl;
        }
        else
        {
            require(argc == 3,
                    "Usage: drs_sprint4_entry_authored_input_tests [<wav|flac> <preview|publish>]");
            runNewSourceScenario(parseFormat(argv[1]), parseLane(argv[2]), loadReferenceStream());
            std::cout << "EG1 authored-input scenario passed: " << argv[1] << " / " << argv[2] << std::endl;
        }
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
