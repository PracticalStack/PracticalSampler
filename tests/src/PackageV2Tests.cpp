#include "drs/engine/PackageV2.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "drs/engine/PackageV2StreamingExport.h"
#include "drs/engine/SampleDataSource.h"
#include "drs/engine/SamplerVoice.h"
#include "drs/engine/DeferredPackageSession.h"
#include "../support/LargeInstrumentFixtures.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;
void require(bool condition, const std::string& message);

void runLargeScaleFixtureMatrix()
{
    const auto fixture = drs::test::makeLargeInstrumentScaleFixture();
    require(fixture.sources.size() == 641
                && fixture.routeSourceIndices.size() == 1704
                && fixture.cachePressurePages.size() == 2048,
            "Large fixture must preserve the Salamander source/route scale and cache-pressure pattern.");
    for (const auto& descriptor : fixture.sources)
        require(drs::engine::validateSampleDataSourceDescriptor(descriptor).valid,
                "Every large-scale source descriptor must pass the production 64-bit contract.");
    require(fixture.sourceAudioBytes > 1800ull * 1024ull * 1024ull
                && fixture.decodedFloatBytes > 2400ull * 1024ull * 1024ull
                && fixture.sparsePackageOffset > 0xffffffffull
                && fixture.sources.back().dataOffsetBytes > 0xffffffffull
                && fixture.sources.front().frameCount >= 75ull * 48000ull,
            "Large fixture must model multi-gig audio, >4 GiB offsets, and long samples without PCM allocation.");
    for (const auto sourceIndex : fixture.routeSourceIndices)
        require(sourceIndex < fixture.sources.size(),
                "Every scale route must resolve to one of the 641 stable source identities.");
    std::cout << "Large fixture trace: sources=" << fixture.sources.size()
              << " routes=" << fixture.routeSourceIndices.size()
              << " sourceBytes=" << fixture.sourceAudioBytes
              << " decodedFloatBytes=" << fixture.decodedFloatBytes
              << " sparseOffset=" << fixture.sparsePackageOffset
              << " pressureTouches=" << fixture.cachePressurePages.size() << std::endl;

    const auto waveformFixture = drs::test::makeWaveformRegionLargeSourceFixture();
    require(waveformFixture.multiSourceInstrument.size() == 64
                && waveformFixture.multiSourceAudioBytes >= 499ull * 1024ull * 1024ull
                && waveformFixture.multiSourceAudioBytes <= 500ull * 1024ull * 1024ull,
            "Waveform qualification must model a multi-source instrument totaling approximately 500 MiB.");
    require(drs::engine::validateSampleDataSourceDescriptor(
                waveformFixture.singleLongSource).valid,
            "The single approximately-500-MiB long source must pass the production descriptor contract.");
    for (const auto& descriptor : waveformFixture.multiSourceInstrument)
        require(drs::engine::validateSampleDataSourceDescriptor(descriptor).valid,
                "Every approximately-500-MiB instrument source must pass the production descriptor contract.");
    require(waveformFixture.playbackStartFrame < waveformFixture.loopStartFrame
                && waveformFixture.loopStartFrame < waveformFixture.loopEndFrame
                && waveformFixture.loopEndFrame < waveformFixture.playbackEndFrame
                && waveformFixture.playbackEndFrame
                    <= waveformFixture.singleLongSource.frameCount,
            "Large-source playback and loop boundaries must remain ordered deep into the file.");
}

#pragma pack(push, 1)
struct SparseV2Header
{
    char magic[8] { 'D', 'R', 'S', 'P', 'K', 'G', '2', '\0' };
    std::uint32_t version = 2;
    std::uint32_t headerBytes = 120;
    std::uint32_t recordCount = 1;
    std::uint32_t flags = 1;
    std::uint64_t tocOffset = 120;
    std::uint64_t tocSize = 128;
    std::uint64_t recordRegionOffset = 248;
    std::uint64_t recordRegionSize = 0;
    char packageId[64] { 's', 'p', 'a', 'r', 's', 'e' };
};

struct SparseV2Record
{
    char sourceId[64] { 's', 'p', 'a', 'r', 's', 'e' };
    std::uint32_t kind = 5;
    std::uint64_t pageIndex = 7000000000ull;
    std::uint64_t sealedOffset = 0;
    std::uint64_t sealedSize = 16;
    std::uint64_t plaintextSize = 0;
    char checksum[17] { 'c', 'b', 'f', '2', '9', 'c', 'e', '4',
                        '8', '4', '2', '2', '2', '3', '2', '5' };
    std::uint64_t sourceGeneration = 1;
    char reserved[3] {};
};
#pragma pack(pop)

static_assert(sizeof(SparseV2Header) == 120 && sizeof(SparseV2Record) == 128);

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<std::uint8_t> bytes(const std::size_t count, const std::uint8_t seed)
{
    std::vector<std::uint8_t> value(count);
    for (std::size_t index = 0; index < count; ++index)
        value[index] = static_cast<std::uint8_t>(seed + index * 17u);
    return value;
}

std::vector<std::uint8_t> floatBytes(const std::vector<float>& samples,
                                     const std::size_t begin,
                                     const std::size_t count)
{
    std::vector<std::uint8_t> value(count * sizeof(float));
    std::memcpy(value.data(), samples.data() + begin, value.size());
    return value;
}

drs::engine::PackageV2WritePlan plan(const fs::path& path)
{
    using Kind = drs::engine::PackageV2RecordKind;
    drs::engine::PackageV2WritePlan value;
    value.packageId = "package-v2-fixture";
    value.outputPath = path.generic_string();
    value.records = {
        { { "package", Kind::manifest, 0 }, bytes(73, 1) },
        { { "instrument", Kind::runtimeInstrument, 0 }, bytes(211, 2) },
        { { "stream-index", Kind::streamIndex, 0 }, bytes(317, 3) },
        { { "piano-a4", Kind::sampleHead, 0 }, bytes(16 * 1024, 4) },
        { { "piano-a4", Kind::samplePage, 0 }, bytes(64 * 1024, 5) },
        { { "piano-a4", Kind::samplePage, 1 }, bytes(31 * 1024, 6) }
    };
    return value;
}

class ChecksumSabotageCrypto final : public drs::engine::PackageCryptoProvider
{
public:
    explicit ChecksumSabotageCrypto(const drs::engine::PackageCryptoProvider& source)
        : backing(source)
    {
    }
    const char* algorithmId() const noexcept override { return backing.algorithmId(); }
    std::size_t nonceSizeBytes() const noexcept override { return backing.nonceSizeBytes(); }
    std::size_t tagSizeBytes() const noexcept override { return backing.tagSizeBytes(); }
    bool seal(const drs::engine::PackageSealRequest& request,
              drs::engine::PackageSealedBlob& output,
              std::string& issue) const override
    {
        return backing.seal(request, output, issue);
    }
    bool open(const drs::engine::PackageOpenRequest& request,
              std::vector<std::uint8_t>& plaintext,
              std::string& issue) const override
    {
        if (!backing.open(request, plaintext, issue))
            return false;
        if (!plaintext.empty())
            plaintext.front() ^= 0x1u;
        return true;
    }
private:
    const drs::engine::PackageCryptoProvider& backing;
};

void runRecordAddressabilityAndCorruptionMatrix()
{
    using Kind = drs::engine::PackageV2RecordKind;
    const auto path = fs::temp_directory_path() / "drs-package-v2-fixture.drpkg";
    auto writePlan = plan(path);
    const auto written = drs::engine::writePackageV2(writePlan);
    require(written.written && written.failure == drs::engine::PackageV2Failure::none,
            "Package v2 writer should emit independently sealed bounded records.");

    const auto opened = drs::engine::openPackageV2(path.generic_string());
    require(opened.opened && opened.records.size() == writePlan.records.size()
                && opened.tocBytes < opened.packageBytes / 10
                && std::all_of(opened.records.begin(), opened.records.end(), [](const auto& record)
                   {
                       return record.plaintextSizeBytes
                           <= drs::engine::performancePackageV2MaximumRecordBytes;
                   }),
            "Package v2 open must read bounded metadata/TOC without materializing record payloads.");

    const drs::engine::PackageV2RecordIdentity page0 { "piano-a4", Kind::samplePage, 0 };
    const drs::engine::PackageV2RecordIdentity page1 { "piano-a4", Kind::samplePage, 1 };
    const auto firstPage = drs::engine::openPackageV2Record(opened, page0);
    require(firstPage.opened && firstPage.plaintextBytes == writePlan.records[4].plaintextBytes
                && firstPage.metrics.recordsOpened == 1
                && firstPage.metrics.bytesRead == firstPage.descriptor.sealedSizeBytes
                && firstPage.metrics.largestPlaintextRecordBytes == 64 * 1024,
            "A page open must read/authenticate/checksum only the requested record range.");

    const auto cancelled = drs::engine::openPackageV2Record(
        opened, page1, drs::engine::getDeterministicPackageCryptoProvider(), [] { return true; });
    require(!cancelled.opened && cancelled.failure == drs::engine::PackageV2Failure::cancelled
                && cancelled.metrics.bytesRead == 0 && cancelled.metrics.cancellationCount == 1,
            "Cancellation before record I/O must be precise and allocation-bounded.");

    ChecksumSabotageCrypto sabotage(drs::engine::getDeterministicPackageCryptoProvider());
    const auto checksumFailure = drs::engine::openPackageV2Record(opened, page1, sabotage);
    require(!checksumFailure.opened
                && checksumFailure.failure == drs::engine::PackageV2Failure::checksum
                && checksumFailure.metrics.checksumFailures == 1,
            "Authenticated plaintext must still pass the independent record checksum.");

    const auto page1Descriptor = *std::find_if(opened.records.begin(), opened.records.end(),
                                               [&](const auto& record)
                                               {
                                                   return record.identity.sourceId == page1.sourceId
                                                       && record.identity.kind == page1.kind
                                                       && record.identity.pageIndex == page1.pageIndex;
                                               });
    {
        std::fstream mutation(path, std::ios::binary | std::ios::in | std::ios::out);
        mutation.seekg(static_cast<std::streamoff>(page1Descriptor.sealedOffsetBytes + 20));
        char value = 0;
        mutation.read(&value, 1);
        value ^= 0x40;
        mutation.seekp(static_cast<std::streamoff>(page1Descriptor.sealedOffsetBytes + 20));
        mutation.write(&value, 1);
    }
    const auto unaffected = drs::engine::openPackageV2Record(opened, page0);
    const auto corrupted = drs::engine::openPackageV2Record(opened, page1);
    require(unaffected.opened && !corrupted.opened
                && corrupted.failure == drs::engine::PackageV2Failure::authentication
                && corrupted.metrics.authenticationFailures == 1,
            "Corrupting one record must fail that record without touching an unrelated page.");

    std::error_code cleanupError;
    fs::remove(path, cleanupError);
    require(!cleanupError, "Package v2 fixture should clean up.");
}

void runBoundsAndPolicyMatrix()
{
    using Kind = drs::engine::PackageV2RecordKind;
    const auto path = fs::temp_directory_path() / "drs-package-v2-policy.drpkg";
    auto oversized = plan(path);
    oversized.records.push_back({ { "too-large", Kind::samplePage, 0 },
                                  bytes(64 * 1024 + 1, 9) });
    require(drs::engine::writePackageV2(oversized).failure
                == drs::engine::PackageV2Failure::recordTooLarge,
            "Package v2 must reject records above the bounded 64 KiB plaintext policy.");

    auto duplicate = plan(path);
    duplicate.records.push_back(duplicate.records.back());
    require(drs::engine::writePackageV2(duplicate).failure
                == drs::engine::PackageV2Failure::duplicateRecord,
            "Package v2 must reject duplicate source/kind/page identities.");

    auto valid = plan(path);
    require(drs::engine::writePackageV2(valid).written,
            "Truncation fixture should write before corruption.");
    const auto size = fs::file_size(path);
    fs::resize_file(path, size - 1);
    require(drs::engine::openPackageV2(path.generic_string()).failure
                == drs::engine::PackageV2Failure::bounds,
            "Truncated record regions must fail checked 64-bit bounds validation.");
    std::error_code cleanupError;
    fs::remove(path, cleanupError);

    const auto sparsePath = fs::temp_directory_path() / "drs-package-v2-sparse-5g.drpkg";
    constexpr std::uint64_t sparseRecordOffset = 5ull * 1024ull * 1024ull * 1024ull;
    SparseV2Header sparseHeader;
    SparseV2Record sparseRecord;
    sparseRecord.sealedOffset = sparseRecordOffset;
    sparseHeader.recordRegionSize = sparseRecordOffset + sparseRecord.sealedSize
        - sparseHeader.recordRegionOffset;
    {
        std::ofstream sparse(sparsePath, std::ios::binary | std::ios::trunc);
        sparse.write(reinterpret_cast<const char*>(&sparseHeader), sizeof(sparseHeader));
        sparse.write(reinterpret_cast<const char*>(&sparseRecord), sizeof(sparseRecord));
        sparse.seekp(static_cast<std::streamoff>(sparseRecordOffset + sparseRecord.sealedSize - 1));
        sparse.put('\0');
    }
    const auto sparseOpen = drs::engine::openPackageV2(sparsePath.generic_string());
    require(sparseOpen.opened && sparseOpen.packageBytes > 0xffffffffull
                && sparseOpen.records.size() == 1
                && sparseOpen.records.front().sealedOffsetBytes == sparseRecordOffset
                && sparseOpen.records.front().identity.pageIndex == 7000000000ull,
            "Package v2 TOC must preserve sparse >4 GiB offsets and 64-bit page identities.");
    fs::remove(sparsePath, cleanupError);
}

void runPackageDataSourceMatrix()
{
    using Kind = drs::engine::PackageV2RecordKind;
    const auto path = fs::temp_directory_path() / "drs-package-v2-source.drpkg";
    const std::vector<float> samples { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f,
                                       0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
    drs::engine::PackageV2WritePlan writePlan;
    writePlan.packageId = "package-v2-source";
    writePlan.outputPath = path.generic_string();
    writePlan.records = {
        { { "package", Kind::manifest, 0 }, bytes(32, 1) },
        { { "instrument", Kind::runtimeInstrument, 0 }, bytes(32, 2) },
        { { "index", Kind::streamIndex, 0 }, bytes(32, 3) },
        { { "piano", Kind::sampleHead, 0 }, floatBytes(samples, 0, 4) },
        { { "piano", Kind::samplePage, 0 }, floatBytes(samples, 4, 4) },
        { { "piano", Kind::samplePage, 1 }, floatBytes(samples, 8, 2) }
    };
    require(drs::engine::writePackageV2(writePlan).written,
            "Package-backed source fixture should write.");
    auto package = std::make_shared<const drs::engine::PackageV2OpenResult>(
        drs::engine::openPackageV2(path.generic_string()));
    drs::engine::SampleDataSourceDescriptor descriptor;
    descriptor.kind = drs::engine::SampleDataSourceKind::packageRecord;
    descriptor.sourceId = "piano";
    descriptor.canonicalSourceIdentity = path.generic_string() + "#piano";
    descriptor.provenanceIdentity = "package-v2-source:piano:g1";
    descriptor.formatName = "package-float32";
    descriptor.channelLayout = "mono";
    descriptor.generation = 1;
    descriptor.sampleRate = 48000.0;
    descriptor.frameCount = samples.size();
    descriptor.channelCount = 1;
    descriptor.bytesPerFrame = sizeof(float);
    descriptor.dataSizeBytes = samples.size() * sizeof(float);
    descriptor.headSizeBytes = 4 * sizeof(float);
    descriptor.pageSizeBytes = 4 * sizeof(float);
    auto source = std::make_shared<drs::engine::PackagePagedSampleDataSource>(descriptor, package);
    require(source->acquireFrameView(0, 1).status
                == drs::engine::SampleFrameViewStatus::pageMissing
                && source->prepareHead() && source->preparePage(0),
            "Package sample records must open only on explicit worker preparation.");
    const auto head = source->acquireFrameView(3, 1);
    const auto page = source->acquireFrameView(4, 2);
    require(head.status == drs::engine::SampleFrameViewStatus::ready
                && page.status == drs::engine::SampleFrameViewStatus::ready
                && head.channels[0][0] == samples[3]
                && page.channels[0][0] == samples[4]
                && page.channels[0][1] == samples[5]
                && source->metrics().recordOpenCount == 2
                && source->metrics().publishedHeadBytes == 4 * sizeof(float)
                && source->metrics().publishedPageBytes == 4 * sizeof(float),
            "Package data source must publish float views matching independently opened records.");
    require(!source->preparePage(1, [] { return true; })
                && source->metrics().cancellationCount == 1
                && source->acquireFrameView(8, 1).status
                    == drs::engine::SampleFrameViewStatus::pageMissing,
            "Cancelled package page work must not publish partial storage.");

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 1;
    snapshot.contentDigest = "package-v2-snapshot";
    drs::engine::PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "zone";
    snapshotZone.sampleSourceId = "piano";
    snapshotZone.displayName = "Package Piano";
    snapshotZone.groupId = "group";
    snapshotZone.articulationId = "sustain";
    snapshot.zones.push_back(snapshotZone);
    drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "group";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.zoneIds = { "zone" };
    snapshotGroup.displayName = "Group";
    snapshot.groupRoutes.push_back(snapshotGroup);

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 1;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = 1;
    prepared.preparedContentDigest = "package-v2-prepared";
    drs::engine::PreparedPlaybackSampleHandle preparedSample;
    preparedSample.sampleSourceId = "piano";
    preparedSample.streamSampleId = "piano-stream";
    preparedSample.sampleRate = 48000.0;
    preparedSample.frameCount = samples.size();
    preparedSample.channelCount = 1;
    preparedSample.dataSource = source;
    prepared.samples.push_back(preparedSample);
    drs::engine::PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = "zone";
    preparedZone.sampleSourceId = "piano";
    preparedZone.streamSampleId = "piano-stream";
    prepared.zones.push_back(preparedZone);
    drs::engine::PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "group";
    preparedGroup.articulationIds = { "sustain" };
    preparedGroup.zoneIds = { "zone" };
    preparedGroup.displayName = "Group";
    prepared.groupRoutes.push_back(preparedGroup);

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::performance;
    payload->revision = 1;
    payload->snapshotBuildId = 1;
    payload->preparedBuildId = 2;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(snapshot);
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(prepared);
    const auto renderModel = drs::engine::buildSamplerRenderModel(payload);
    require(renderModel.built && renderModel.model != nullptr,
            "Package data source must enter the common immutable render model.");
    drs::engine::SamplerVoice voice;
    drs::engine::SamplerVoiceStartRequest start;
    start.voiceId = 1;
    start.routeIndex = 0;
    start.sourceMidiNote = 60;
    start.effectiveMidiNote = 60;
    start.effectiveVelocity = 127;
    std::vector<float> left(7, 0.0f), right(7, 0.0f);
    std::array<float*, 2> channels { left.data(), right.data() };
    require(voice.start(*renderModel.model, start)
                && voice.render({ channels.data(), 2, 7 }, 0, 7).pageMissCount == 0
                && left[3] == samples[3] && left[4] == samples[4]
                && left[6] == samples[6],
            "The production voice must render continuously from package head into package page 0.");

    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::performance);
    require(context.prepare(48000.0), "Deferred package playback context should prepare.");
    drs::engine::DeferredPackageSession session;
    drs::engine::DeferredPackageSessionPlan deferredPlan;
    deferredPlan.packagePath = path.generic_string();
    deferredPlan.package = package;
    deferredPlan.sources = { source };
    deferredPlan.buildRenderModel = [model = renderModel.model] { return model; };
    const auto metadataStarted = std::chrono::steady_clock::now();
    require(session.begin(deferredPlan)
                && session.snapshot().stage
                    == drs::engine::DeferredPackageSessionStage::metadataReady
                && session.snapshot().metadataAccepted && !session.snapshot().playable,
            "Deferred package begin must expose metadata without claiming playable readiness.");
    require(drs::engine::projectDeferredPackageReadiness(session.snapshot())
                == drs::engine::PackageSessionReadiness::metadataLoaded
                && drs::engine::packageWorkspaceStatusText(
                    drs::engine::projectDeferredPackageReadiness(session.snapshot()))
                    == "Package metadata loaded | Playback deferred",
            "Metadata readiness must map to the shared plug-in/standalone status contract.");
    const auto metadataMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - metadataStarted).count();
    while (session.snapshot().stage != drs::engine::DeferredPackageSessionStage::playable)
        require(session.serviceNextWorkerStep(),
                "Deferred package worker stages must advance automatically to playable.");
    const auto pluginProjection = drs::engine::projectDeferredPackageStatus(session.snapshot());
    const auto standaloneProjection = drs::engine::projectDeferredPackageStatus(session.snapshot());
    require(pluginProjection.headline == standaloneProjection.headline
                && pluginProjection.detail == standaloneProjection.detail
                && drs::engine::projectDeferredPackageReadiness(session.snapshot())
                    == drs::engine::PackageSessionReadiness::playable
                && session.stagePlayableActivation(context),
            "Plug-in and standalone must share one lifecycle/status projection and staging contract.");
    require(drs::engine::projectDeferredPackageReadiness(session.snapshot())
                == drs::engine::PackageSessionReadiness::pendingActivation,
            "Staged playback must remain pending until the callback consumes the activation.");
    std::vector<float> activationLeft(1, 0.0f), activationRight(1, 0.0f);
    std::array<float*, 2> activationChannels { activationLeft.data(), activationRight.data() };
    drs::engine::SamplerEventBlock noEvents;
    const auto cutover = context.renderBlock(
        { activationChannels.data(), 2, 1 }, noEvents.view());
    require(cutover.activationApplied && session.observeAudioCutover(context)
                && session.snapshot().stage == drs::engine::DeferredPackageSessionStage::active
                && session.snapshot().active && metadataMicros < 10000,
            "Playable package activation must become active only after callback-side cutover.");
    require(drs::engine::projectDeferredPackageReadiness(session.snapshot())
                == drs::engine::PackageSessionReadiness::active,
            "Callback cutover must project the shared active readiness state.");

    drs::engine::SamplerEventBlock note;
    note.push({ drs::engine::SamplerRenderEventType::noteOn, 0, 60, 1.0f });
    context.renderBlock({ activationChannels.data(), 2, 1 }, note.view());
    require(session.begin(deferredPlan), "Replacement package request should supersede prior work.");
    while (session.snapshot().stage != drs::engine::DeferredPackageSessionStage::playable)
        session.serviceNextWorkerStep();
    require(session.stagePlayableActivation(context),
            "Replacement package should stage without stopping the active generation.");
    context.renderBlock({ activationChannels.data(), 2, 1 }, noEvents.view());
    require(session.observeAudioCutover(context)
                && context.getSnapshot().retiredGenerationVoiceCount > 0,
            "Old-generation voices must survive a package replacement cutover.");
    std::vector<float> retirementLeft(16, 0.0f), retirementRight(16, 0.0f);
    std::array<float*, 2> retirementChannels { retirementLeft.data(), retirementRight.data() };
    context.renderBlock({ retirementChannels.data(), 2, 16 }, noEvents.view());
    context.serviceRetirements();

    drs::engine::DeferredPackageSessionPlan invalidPlan;
    invalidPlan.packagePath = path.generic_string() + ".missing";
    invalidPlan.package = package;
    invalidPlan.buildRenderModel = deferredPlan.buildRenderModel;
    require(!session.begin(std::move(invalidPlan))
                && session.snapshot().stage == drs::engine::DeferredPackageSessionStage::degraded
                && session.snapshot().active
                && drs::engine::projectDeferredPackageReadiness(session.snapshot())
                    == drs::engine::PackageSessionReadiness::degraded,
            "A failed replacement must degrade diagnostics while retaining prior active audio.");
    const auto churnStarted = std::chrono::steady_clock::now();
    for (int index = 0; index < 100; ++index)
    {
        require(session.begin(deferredPlan), "Rapid replacement request should remain admissible.");
        session.cancel(&context);
    }
    const auto churnMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - churnStarted).count();
    require(churnMicros < 100000 && session.snapshot().active,
            "Rapid replacement/cancel churn must remain nonblocking and preserve active audio.");
    std::size_t locatorOpenCount = 0;
    const auto locatorStarted = std::chrono::steady_clock::now();
    require(session.requestLocatorRestore(path.generic_string(),
                [&](const std::string& requestedPath)
                {
                    ++locatorOpenCount;
                    require(requestedPath == path.generic_string(),
                            "Deferred locator resolver must receive the persisted package path.");
                    return deferredPlan;
                })
                && locatorOpenCount == 0
                && session.snapshot().stage
                    == drs::engine::DeferredPackageSessionStage::locatorPending,
            "Host locator restore must queue without opening package files synchronously.");
    const auto locatorRequestMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - locatorStarted).count();
    require(session.serviceNextWorkerStep() && locatorOpenCount == 1
                && session.snapshot().stage
                    == drs::engine::DeferredPackageSessionStage::metadataReady
                && locatorRequestMicros < 10000,
            "Background service must continue a queued host locator into metadata-ready state.");
    std::cout << "Deferred lifecycle trace: metadataMicros=" << metadataMicros
              << " replacementCancel100Micros=" << churnMicros
              << " locatorRequestMicros=" << locatorRequestMicros
              << " activeGeneration=" << session.snapshot().activeGeneration << std::endl;

    const auto page1 = *std::find_if(package->records.begin(), package->records.end(), [](const auto& record)
    {
        return record.identity.sourceId == "piano"
            && record.identity.kind == Kind::samplePage && record.identity.pageIndex == 1;
    });
    {
        std::fstream mutation(path, std::ios::binary | std::ios::in | std::ios::out);
        mutation.seekg(static_cast<std::streamoff>(page1.sealedOffsetBytes + 18));
        char value = 0;
        mutation.read(&value, 1);
        value ^= 0x20;
        mutation.seekp(static_cast<std::streamoff>(page1.sealedOffsetBytes + 18));
        mutation.write(&value, 1);
    }
    require(!source->preparePage(1)
                && source->metrics().authenticationFailureCount == 1
                && source->acquireFrameView(4, 1).status
                    == drs::engine::SampleFrameViewStatus::ready,
            "Corrupt package pages must fail without invalidating already published pages.");

    std::error_code cleanupError;
    fs::remove(path, cleanupError);
}

void runReaderDispatchCompatibilityMatrix()
{
    const auto v1Path = fs::temp_directory_path() / "drs-package-v1-small-compat.drpkg";
    drs::engine::PerformancePackageWritePlan v1Plan;
    v1Plan.outputPackagePath = v1Path.generic_string();
    v1Plan.manifest.packageId = "small-v1";
    v1Plan.manifest.displayName = "Small v1";
    v1Plan.manifest.instrumentId = "instrument-v1";
    v1Plan.payloads.push_back({ "manifest", drs::engine::PerformancePackagePayloadKind::packageManifest,
                                "manifest.json", "application/json", bytes(128, 7) });
    require(drs::engine::writePerformancePackage(v1Plan).written,
            "Small v1 compatibility fixture should write through the legacy writer.");
    const auto sizeBefore = fs::file_size(v1Path);
    const auto timeBefore = fs::last_write_time(v1Path);
    const auto v1 = drs::engine::dispatchPerformancePackageReader(v1Path.generic_string());
    require(v1.opened && v1.format == drs::engine::PerformancePackageDiskFormat::version1
                && !v1.migrationRequired && fs::file_size(v1Path) == sizeBefore
                && fs::last_write_time(v1Path) == timeBefore,
            "Small v1 packages must load deterministically without rewrite-on-load behavior.");

    const auto largeV1Path = fs::temp_directory_path() / "drs-package-v1-oversized.drpkg";
    {
        std::ofstream output(largeV1Path, std::ios::binary | std::ios::trunc);
        const std::array<char, 8> v1Magic { 'D', 'R', 'S', 'P', 'K', 'G', '1', '\0' };
        output.write(v1Magic.data(), v1Magic.size());
        output.seekp(static_cast<std::streamoff>(
            drs::engine::maximumResidentV1PackageBytes));
        output.put('\0');
    }
    const auto largeSize = fs::file_size(largeV1Path);
    const auto oversized = drs::engine::dispatchPerformancePackageReader(
        largeV1Path.generic_string());
    const auto oversizedInspection = drs::engine::inspectPerformancePackage(
        largeV1Path.generic_string());
    require(!oversized.opened && oversized.migrationRequired
                && oversized.format == drs::engine::PerformancePackageDiskFormat::version1
                && oversized.issues.size() == 1
                && oversized.issues.front().find("re-export") != std::string::npos
                && !oversizedInspection.valid
                && oversizedInspection.state.find("re-export") != std::string::npos
                && fs::file_size(largeV1Path) == largeSize,
            "Oversized v1 packages must reject before every resident read path with explicit v2 re-export guidance.");

    std::error_code cleanupError;
    fs::remove(v1Path, cleanupError);
    cleanupError.clear();
    fs::remove(largeV1Path, cleanupError);
}

void runStreamingExportMatrix()
{
    using Kind = drs::engine::PackageV2RecordKind;
    const auto compiledPath = fs::temp_directory_path() / "drs-v2-compiled-float.bin";
    const auto outputPath = fs::temp_directory_path() / "drs-v2-streaming-export.drpkg";
    const auto cancelledPath = fs::temp_directory_path() / "drs-v2-streaming-cancel.drpkg";
    const std::vector<float> samples { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f,
                                       0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
    {
        std::ofstream output(compiledPath, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(samples.data()),
                     static_cast<std::streamsize>(samples.size() * sizeof(float)));
    }
    const std::vector<drs::engine::PackageV2RecordSource> metadata {
        { { "package", Kind::manifest, 0 }, bytes(32, 1) },
        { { "instrument", Kind::runtimeInstrument, 0 }, bytes(48, 2) },
        { { "index", Kind::streamIndex, 0 }, bytes(64, 3) }
    };
    const std::vector<drs::engine::PackageV2CompiledSampleInput> inputs {
        { "piano", 2, compiledPath.generic_string(), 0,
          samples.size() * sizeof(float), 4 * sizeof(float), 4 * sizeof(float) }
    };
    const auto built = drs::engine::buildPackageV2StreamingExportPlan(
        "streaming-export", outputPath.generic_string(), metadata, inputs);
    require(built.built && built.plan.records.size() == 6
                && built.totalPlaintextBytes
                    == 32 + 48 + 64 + samples.size() * sizeof(float),
            "Streaming export plan must split compiled float payload into head/page records.");
    std::vector<drs::engine::PackageV2StreamingWriteStage> stages;
    drs::engine::PackageV2StreamingWriteOptions options;
    options.progressSink = [&](const auto& progress)
    {
        stages.push_back(progress.stage);
        require(progress.totalPlaintextBytes == built.totalPlaintextBytes,
                "Streaming export progress must retain deterministic total bytes.");
    };
    const auto exported = drs::engine::writePackageV2Streaming(built.plan,
        drs::engine::getDeterministicPackageCryptoProvider(), options);
    require(exported.written && exported.verified && exported.atomicallyPublished
                && exported.completedRecordCount == built.plan.records.size()
                && exported.processedPlaintextBytes == built.totalPlaintextBytes
                && exported.peakPlaintextBufferBytes <= 64 * 1024
                && exported.peakSealedBufferBytes < 65 * 1024
                && exported.totalDurationMicros > 0
                && exported.plaintextThroughputBytesPerSecond > 0.0
                && !fs::exists(exported.stagingPath)
                && !stages.empty()
                && stages.back() == drs::engine::PackageV2StreamingWriteStage::completed,
            "Streaming export must remain record-bounded, verify staging, and publish atomically.");
    const auto opened = drs::engine::openPackageV2(outputPath.generic_string());
    const auto lastPage = drs::engine::openPackageV2Record(
        opened, { "piano", Kind::samplePage, 1, 2 });
    require(lastPage.opened && lastPage.plaintextBytes == floatBytes(samples, 8, 2)
                && exported.verificationBytesRead < exported.packageBytes,
            "Streaming verification must sample records without a second full-package read.");

    auto cancelledPlan = built.plan;
    cancelledPlan.outputPath = cancelledPath.generic_string();
    std::size_t cancellationPolls = 0;
    drs::engine::PackageV2StreamingWriteOptions cancellationOptions;
    cancellationOptions.cancellationProbe = [&] { return ++cancellationPolls >= 4; };
    const auto cancelled = drs::engine::writePackageV2Streaming(
        cancelledPlan, drs::engine::getDeterministicPackageCryptoProvider(),
        cancellationOptions);
    require(!cancelled.written && cancelled.failure == drs::engine::PackageV2Failure::cancelled
                && cancelled.completedRecordCount < cancelledPlan.records.size()
                && !fs::exists(cancelledPath) && !fs::exists(cancelled.stagingPath),
            "Streaming cancellation must stop at a record boundary and remove incomplete staging.");

    const auto sparseInput = fs::temp_directory_path() / "drs-v2-sparse-1g-float.bin";
    const auto sparseOutput = fs::temp_directory_path() / "drs-v2-sparse-1g-cancel.drpkg";
    {
        std::ofstream seed(sparseInput, std::ios::binary | std::ios::trunc);
        seed.put('\0');
    }
    constexpr std::uint64_t oneGiB = 1024ull * 1024ull * 1024ull;
    fs::resize_file(sparseInput, oneGiB);
    const std::vector<drs::engine::PackageV2CompiledSampleInput> sparseInputs {
        { "stress", 3, sparseInput.generic_string(), 0, oneGiB, 16 * 1024, 64 * 1024 }
    };
    const auto sparsePlan = drs::engine::buildPackageV2StreamingExportPlan(
        "sparse-stress", sparseOutput.generic_string(), metadata, sparseInputs);
    require(sparsePlan.built && sparsePlan.totalPlaintextBytes > oneGiB,
            "Synthetic 1 GiB stress plan must use 64-bit byte accounting.");
    std::size_t sparsePolls = 0;
    drs::engine::PackageV2StreamingWriteOptions sparseOptions;
    sparseOptions.cancellationProbe = [&] { return ++sparsePolls >= 5; };
    const auto started = std::chrono::steady_clock::now();
    const auto sparseCancelled = drs::engine::writePackageV2Streaming(
        sparsePlan.plan, drs::engine::getDeterministicPackageCryptoProvider(), sparseOptions);
    const auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();
    std::cout << "Sparse cancel probe: failure=" << drs::engine::toString(sparseCancelled.failure)
              << " peakPlain=" << sparseCancelled.peakPlaintextBufferBytes
              << " peakSealed=" << sparseCancelled.peakSealedBufferBytes
              << " elapsedMs=" << elapsedMillis
              << " outputExists=" << fs::exists(sparseOutput)
              << " stageExists=" << fs::exists(sparseCancelled.stagingPath) << std::endl;
    require(!sparseCancelled.written
                && sparseCancelled.failure == drs::engine::PackageV2Failure::cancelled
                && sparseCancelled.peakPlaintextBufferBytes <= 64 * 1024
                && sparseCancelled.peakSealedBufferBytes < 65 * 1024
                && sparseCancelled.cancellationResponseMicros > 0
                && elapsedMillis < 5000
                && !fs::exists(sparseOutput) && !fs::exists(sparseCancelled.stagingPath),
            "1 GiB synthetic export cancellation must stay record-bounded and responsive.");
    std::cout << "Streaming export trace: records=" << exported.completedRecordCount
              << " plaintextBytes=" << exported.processedPlaintextBytes
              << " peakPlaintextBytes=" << exported.peakPlaintextBufferBytes
              << " peakSealedBytes=" << exported.peakSealedBufferBytes
              << " verificationBytes=" << exported.verificationBytesRead
              << " totalMicros=" << exported.totalDurationMicros
              << " throughputBps=" << exported.plaintextThroughputBytesPerSecond
              << " sparseCancelMillis=" << elapsedMillis << std::endl;

    std::error_code cleanupError;
    for (const auto& path : { compiledPath, outputPath, cancelledPath,
                              sparseInput, sparseOutput })
    {
        cleanupError.clear();
        fs::remove(path, cleanupError);
    }
}
} // namespace

int main()
{
    try
    {
        runLargeScaleFixtureMatrix();
        runRecordAddressabilityAndCorruptionMatrix();
        runBoundsAndPolicyMatrix();
        runPackageDataSourceMatrix();
        runReaderDispatchCompatibilityMatrix();
        runStreamingExportMatrix();
        std::cout << "Package v2 bounded record matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
