#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PerformancePublishPreparation.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct Fixture
{
    PlaybackSnapshotBuildResult snapshot;
    PreparedPlaybackBuildResult prepared;
    PerformancePublishRequestIdentity identity;
};

PreparedPlaybackSampleHandle makeSample(std::string id, std::string path,
                                        std::string fingerprint, std::string format)
{
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = id;
    sample.streamSampleId = id;
    sample.sourcePath = path;
    sample.canonicalSourcePath = path;
    sample.canonicalSourceIdentity = id + "|" + path;
    sample.sourceFingerprintHex = fingerprint;
    sample.formatName = format;
    sample.role = "authored-performance-source";
    sample.channelLayout = "stereo";
    sample.sampleRate = 48000.0;
    sample.frameCount = 128;
    sample.channelCount = 2;
    sample.ownershipToken = "owner:" + id;
    sample.cacheKey = "cache:" + id + ":" + fingerprint;
    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::vector<float>(128, 0.1f), std::vector<float>(128, -0.1f) };
    sample.decodedSampleData = std::move(decoded);
    return sample;
}

Fixture makeFixture()
{
    Fixture value;
    auto& snapshot = value.snapshot;
    snapshot.built = true;
    snapshot.activationEligible = true;
    snapshot.buildId = 41;
    snapshot.cancellationId = 41;
    snapshot.requestedDraftRevision = 9;
    snapshot.activationRequested = true;
    snapshot.lifecycleState = PlaybackSnapshotLifecycleState::ready;
    snapshot.snapshot.schemaName = "drs.playbackSnapshot";
    snapshot.snapshot.schemaVersion = 1;
    snapshot.snapshot.projectId = "full-project";
    snapshot.snapshot.displayName = "Full Project";
    snapshot.snapshot.draftRevision = 9;
    snapshot.snapshot.sampleIdentities = {
        { "wav-source", "C:/authored/kick.wav", "kick" },
        { "flac-source", "C:/authored/strings.flac", "strings" }
    };
    PlaybackSnapshotMacroDefault dynamics;
    dynamics.id = "dynamics";
    dynamics.name = "Dynamics";
    dynamics.defaultValue = 0.5;
    dynamics.minValue = 0.0;
    dynamics.maxValue = 1.0;
    dynamics.targets.push_back({ "gain", "groups/main/gain", "amplitude" });
    snapshot.snapshot.macroDefaults.push_back(dynamics);
    snapshot.snapshot.fxSlots.push_back({ "room", "Room", "reverb", false });
    snapshot.snapshot.routingBuses.push_back({ "main", "Main", "groups/main", { "room" } });
    snapshot.snapshot.zones = {
        { "kick-zone", "wav-source", "Kick", "drums", "sustain", 36, 36, 36, 1, 127,
          -1.0, -0.2, 0, false, 0, 0 },
        { "strings-zone", "flac-source", "Strings", "main", "sustain", 60, 48, 84, 1, 127,
          -3.0, 0.25, 4, true, 16, 96 }
    };
    snapshot.snapshot.articulationRoutes.push_back({ "sustain", { "kick-zone", "strings-zone" } });
    snapshot.snapshot.groupRoutes = {
        { "drums", { "sustain" }, { "kick-zone" } },
        { "main", { "sustain" }, { "strings-zone" } }
    };
    snapshot.snapshot.contentDigest = computePlaybackSnapshotContentDigest(snapshot.snapshot);

    auto& result = value.prepared;
    result.built = true;
    result.activationEligible = true;
    result.buildId = 77;
    result.cancellationId = 77;
    result.snapshotBuildId = snapshot.buildId;
    result.requestedDraftRevision = 9;
    result.activationRequested = true;
    result.lifecycleState = PlaybackSnapshotLifecycleState::ready;
    result.prepared.snapshotBuildId = snapshot.buildId;
    result.prepared.snapshotContentDigest = snapshot.snapshot.contentDigest;
    result.prepared.compilerVersion = "sprint6.3-test";
    result.prepared.draftRevision = 9;
    result.prepared.samples = {
        makeSample("wav-source", "C:/authored/kick.wav", "aaaa", "WAV"),
        makeSample("flac-source", "C:/authored/strings.flac", "bbbb", "FLAC")
    };
    for (std::size_t index = 0; index < result.prepared.samples.size(); ++index)
    {
        const auto& sample = result.prepared.samples[index];
        result.prepared.ownershipRecords.push_back({ sample.ownershipToken, {}, sample.cacheKey,
                                                     sample.sampleSourceId, sample.streamSampleId,
                                                     "active-cache-entry", 2048, result.buildId, 0 });
        PreparedPlaybackStreamHandle stream;
        stream.sampleSourceId = sample.sampleSourceId;
        stream.streamSampleId = sample.streamSampleId;
        stream.payloadEncoding = "decoded-float32";
        stream.topologyKind = "decoded-memory";
        stream.ownershipToken = sample.ownershipToken;
        stream.cacheKey = sample.cacheKey;
        stream.ownershipRecordIndex = index;
        result.prepared.streams.push_back(std::move(stream));
        result.prepared.samples[index].ownershipRecordIndex = index;
    }
    result.prepared.zones = {
        { "kick-zone", "wav-source", "wav-source", 0, 0, 36, 36, 36, 1, 127,
          -1.0, -0.2, 0, false, 0, 0 },
        { "strings-zone", "flac-source", "flac-source", 1, 1, 60, 48, 84, 1, 127,
          -3.0, 0.25, 4, true, 16, 96 }
    };
    result.metrics.preparedSampleCount = 2;
    result.metrics.preparedStreamCount = 2;
    result.metrics.preparedZoneCount = 2;
    result.metrics.preparedOwnershipRecordCount = 2;
    result.prepared.routeDigest = computePreparedPlaybackRouteDigest(snapshot.snapshot, result.prepared);
    result.prepared.sourceProvenanceDigest = computePreparedPlaybackSourceProvenanceDigest(result.prepared);
    result.prepared.macroSchemaDigest = computePlaybackSnapshotMacroSchemaDigest(snapshot.snapshot);
    result.prepared.preparedContentDigest = computePreparedPlaybackContentDigest(result.prepared);

    value.identity.requestId = 12;
    value.identity.cancellationGeneration = 3;
    value.identity.projectGeneration = 5;
    value.identity.draftRevision = 9;
    value.identity.authoredContentDigest = snapshot.snapshot.contentDigest;
    value.identity.macroSchemaDigest = result.prepared.macroSchemaDigest;
    return value;
}

bool containsCode(const PerformancePublishPreparationResult& result, const std::string& code)
{
    return std::any_of(result.findings.begin(), result.findings.end(), [&](const auto& finding)
    {
        return finding.code == code && !finding.path.empty();
    });
}
} // namespace

int main()
{
    try
    {
        const auto fixture = makeFixture();
        const auto valid = validatePerformancePublishPreparation(
            fixture.identity, fixture.snapshot, fixture.prepared);
        require(valid.completeProject && valid.activationEligible
                    && performancePublishResultIsEligible(fixture.identity, valid.publishResult)
                    && valid.findings.empty(),
                "The exact complete multi-zone WAV/FLAC project must be activation-eligible.");
        require(!valid.publishResult.routeDigest.empty()
                    && !valid.publishResult.sourceProvenanceDigest.empty()
                    && valid.publishResult.preparedMacroSchemaDigest == fixture.identity.macroSchemaDigest,
                "The eligible result must carry every immutable conformance digest.");

        const auto repeat = validatePerformancePublishPreparation(
            fixture.identity, fixture.snapshot, fixture.prepared);
        require(repeat.publishResult.preparedContentDigest == valid.publishResult.preparedContentDigest
                    && repeat.publishResult.routeDigest == valid.publishResult.routeDigest
                    && repeat.publishResult.sourceProvenanceDigest == valid.publishResult.sourceProvenanceDigest,
                "Repeated complete-project preparation must be deterministic.");

        auto reorderedSnapshot = fixture.snapshot.snapshot;
        auto reorderedPrepared = fixture.prepared.prepared;
        std::reverse(reorderedSnapshot.zones.begin(), reorderedSnapshot.zones.end());
        std::reverse(reorderedSnapshot.groupRoutes.begin(), reorderedSnapshot.groupRoutes.end());
        std::reverse(reorderedPrepared.zones.begin(), reorderedPrepared.zones.end());
        std::reverse(reorderedPrepared.samples.begin(), reorderedPrepared.samples.end());
        require(computePreparedPlaybackRouteDigest(reorderedSnapshot, reorderedPrepared)
                    == fixture.prepared.prepared.routeDigest
                    && computePreparedPlaybackSourceProvenanceDigest(reorderedPrepared)
                        == fixture.prepared.prepared.sourceProvenanceDigest,
                "Route and provenance digests must normalize by stable authored identity.");

        auto partial = fixture.prepared;
        partial.prepared.zones.pop_back();
        partial.prepared.routeDigest = computePreparedPlaybackRouteDigest(fixture.snapshot.snapshot, partial.prepared);
        partial.prepared.preparedContentDigest = computePreparedPlaybackContentDigest(partial.prepared);
        const auto partialResult = validatePerformancePublishPreparation(fixture.identity, fixture.snapshot, partial);
        require(!partialResult.activationEligible
                    && containsCode(partialResult, "publish-zone-coverage-incomplete"),
                "A mixed or partial project must never produce an eligible payload.");

        auto provenanceMismatch = fixture.prepared;
        provenanceMismatch.prepared.samples[0].sourceFingerprintHex = "changed";
        const auto provenanceResult = validatePerformancePublishPreparation(
            fixture.identity, fixture.snapshot, provenanceMismatch);
        require(!provenanceResult.activationEligible
                    && containsCode(provenanceResult, "publish-source-provenance-digest-mismatch"),
                "Source changes must invalidate immutable provenance.");

        auto routeMismatch = fixture.prepared;
        routeMismatch.prepared.zones[0].keyHigh = 40;
        routeMismatch.prepared.preparedContentDigest = computePreparedPlaybackContentDigest(routeMismatch.prepared);
        const auto routeResult = validatePerformancePublishPreparation(fixture.identity, fixture.snapshot, routeMismatch);
        require(!routeResult.activationEligible
                    && containsCode(routeResult, "publish-route-digest-mismatch")
                    && containsCode(routeResult, "publish-authored-zone-not-prepared"),
                "Route edits must invalidate topology and authored-zone equality.");

        auto malformedRoutes = fixture.snapshot;
        malformedRoutes.snapshot.groupRoutes[0].zoneIds = { "strings-zone" };
        malformedRoutes.snapshot.contentDigest = computePlaybackSnapshotContentDigest(malformedRoutes.snapshot);
        auto malformedIdentity = fixture.identity;
        malformedIdentity.authoredContentDigest = malformedRoutes.snapshot.contentDigest;
        auto malformedPrepared = fixture.prepared;
        malformedPrepared.prepared.snapshotContentDigest = malformedRoutes.snapshot.contentDigest;
        malformedPrepared.prepared.routeDigest = computePreparedPlaybackRouteDigest(
            malformedRoutes.snapshot, malformedPrepared.prepared);
        malformedPrepared.prepared.preparedContentDigest = computePreparedPlaybackContentDigest(
            malformedPrepared.prepared);
        const auto malformedRouteResult = validatePerformancePublishPreparation(
            malformedIdentity, malformedRoutes, malformedPrepared);
        require(!malformedRouteResult.activationEligible
                    && containsCode(malformedRouteResult, "publish-group-route-invalid"),
                "Malformed authored group routing must produce a path-scoped ineligible result.");

        auto macroMismatch = fixture.prepared;
        macroMismatch.prepared.macroSchemaDigest = "fnv1a64:wrong";
        const auto macroResult = validatePerformancePublishPreparation(fixture.identity, fixture.snapshot, macroMismatch);
        require(!macroResult.activationEligible
                    && containsCode(macroResult, "publish-macro-schema-digest-mismatch"),
                "Macro schema changes must invalidate the captured request.");

        auto wrongRevision = fixture.identity;
        ++wrongRevision.draftRevision;
        const auto revisionResult = validatePerformancePublishPreparation(
            wrongRevision, fixture.snapshot, fixture.prepared);
        require(!revisionResult.activationEligible
                    && containsCode(revisionResult, "publish-revision-mismatch"),
                "Cross-revision completion must be rejected.");

        auto canceled = fixture.prepared;
        canceled.built = false;
        canceled.activationEligible = false;
        canceled.lifecycleState = PlaybackSnapshotLifecycleState::canceled;
        const auto canceledResult = validatePerformancePublishPreparation(
            fixture.identity, fixture.snapshot, canceled);
        require(!canceledResult.activationEligible
                    && containsCode(canceledResult, "publish-preparation-canceled"),
                "Canceled preparation must remain terminal and ineligible.");

        PlaybackActivationPayloadPtr retainedPayload;
        {
            DraftPlaybackContract contract(9);
            const auto request = contract.requestPerformanceBuild();
            require(contract.completePerformanceBuild(request.requestId, fixture.snapshot, fixture.prepared),
                    "The validated project must enter the immutable activation contract.");
            retainedPayload = contract.getStatus().performance.activationPayload;
        }
        require(retainedPayload && retainedPayload->prepared && retainedPayload->snapshot
                    && retainedPayload->routeDigest == fixture.prepared.prepared.routeDigest
                    && retainedPayload->sourceProvenanceDigest
                        == fixture.prepared.prepared.sourceProvenanceDigest
                    && retainedPayload->macroSchemaDigest == fixture.prepared.prepared.macroSchemaDigest,
                "Activation payload ownership and every digest must outlive preparation locals.");

        std::cout << "Mini Sprint 6.3 full-project immutable preparation matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.3 preparation matrix failed: " << exception.what() << std::endl;
        return 1;
    }
}
