#include "drs/engine/AuthoringSession.h"
#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/PackageV2StreamingExport.h"
#include "drs/engine/DeferredPackageSession.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "shared/PerformancePackageExportService.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <Psapi.h>
#endif

namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::uint64_t elapsedMicros(const Clock::time_point start)
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count());
}

std::uint64_t peakWorkingSetBytes()
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters {};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) != FALSE)
        return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
#endif
    return 0;
}

std::string joinIssues(const std::vector<std::string>& issues)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            stream << " | ";
        stream << issues[index];
    }
    return stream.str();
}

std::string joinFindings(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < findings.size(); ++index)
    {
        if (index != 0)
            stream << " | ";
        stream << findings[index].code << ": " << findings[index].message;
    }
    return stream.str();
}

drs::engine::RuntimeProjectModel makeBlankProject(const fs::path& sfzPath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "qualification.accurate-salamander";
    project.displayName = "Accurate Salamander Qualification";
    project.contentRootPath = sfzPath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (sfzPath.parent_path() / "qualification.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    return project;
}

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(
    drs::engine::PlaybackSnapshotBuilder& builder,
    const drs::engine::RuntimeProjectModel& project,
    const std::size_t revision)
{
    const auto request = builder.requestBuild(revision, false);
    require(request.accepted, "Snapshot request was rejected.");
    const auto result = builder.buildSnapshot(request, project);
    require(result.built && result.activationEligible,
            "Snapshot build failed: " + result.state);
    return result;
}

struct PreparedRun
{
    drs::engine::PreparedPlaybackWorkerStepResult step;
    std::uint64_t elapsed = 0;
};

PreparedRun prepareSynchronously(const drs::engine::PlaybackSnapshotBuildResult& snapshot)
{
    drs::engine::PreparedPlaybackSchedulerBudgets budgets;
    // Force the qualification through the streaming seam even for a one-zone scope.
    budgets.maximumRetainedPreparedBytes = 1;
    drs::engine::PreparedPlaybackService service("large-instrument-qualification", 2, false, budgets);
    const drs::engine::RuntimeStreamLoadResult noCompiledStream;
    const auto start = Clock::now();
    require(service.enqueuePreviewBuild(snapshot).accepted,
            "Preparation request was rejected before worker execution.");
    auto step = service.processNextQueuedBuild(noCompiledStream);
    const auto elapsed = elapsedMicros(start);
    require(step.processed && step.result.built && step.result.activationEligible,
            "Preparation failed: " + step.result.state + " :: "
                + joinFindings(step.result.findings));
    require(step.result.metrics.decodedBytes == 0,
            "Streaming preparation decoded full sample PCM.");
    return { std::move(step), elapsed };
}

struct SourceMetrics
{
    std::uint64_t headBytes = 0;
    std::uint64_t pageBytes = 0;
    std::uint64_t rangeReads = 0;
    std::uint64_t rangeBytes = 0;
    std::uint64_t maximumReadMicros = 0;
    std::uint64_t pageMisses = 0;
    std::uint64_t pageCacheBudgetBytes = 0;
    std::uint64_t maximumAllocatedPageBytes = 0;
    std::uint64_t leasedPageBytes = 0;
    std::uint64_t retiredPageBytes = 0;
    std::uint64_t intentPublished = 0;
    std::uint64_t intentConsumed = 0;
    std::uint64_t intentDropped = 0;
    std::size_t maximumIntentDepth = 0;
};

SourceMetrics collectSourceMetrics(const drs::engine::ImmutablePreparedPlayback& prepared)
{
    SourceMetrics total;
    for (const auto& sample : prepared.samples)
    {
        const auto source = std::dynamic_pointer_cast<const drs::engine::WavPagedSampleDataSource>(
            sample.dataSource);
        if (source == nullptr)
            continue;
        const auto metrics = source->metrics();
        total.headBytes += metrics.residentHeadBytes;
        total.pageBytes += metrics.residentPageBytes;
        total.rangeReads += metrics.rangeReadCount;
        total.rangeBytes += metrics.bytesRead;
        total.maximumReadMicros = std::max(total.maximumReadMicros,
                                           metrics.maximumReadLatencyMicros);
        total.pageMisses += metrics.pageMissCount;
        total.pageCacheBudgetBytes += metrics.pageCacheBudgetBytes;
        total.maximumAllocatedPageBytes += metrics.maximumAllocatedPageBytes;
        total.leasedPageBytes += metrics.leasedPageBytes;
        total.retiredPageBytes += metrics.retiredPageBytes;
        const auto intents = source->intentMetrics();
        total.intentPublished += intents.publishedCount;
        total.intentConsumed += intents.consumedCount;
        total.intentDropped += intents.droppedCount;
        total.maximumIntentDepth = std::max(total.maximumIntentDepth, intents.maximumDepth);
    }
    return total;
}

struct SustainedPlaybackResult
{
    float peak = 0.0f;
    std::uint64_t pageMisses = 0;
    std::uint64_t underrunFrames = 0;
    std::uint64_t recoveries = 0;
    std::uint64_t elapsed = 0;
    std::uint64_t maximumRenderMicros = 0;
    drs::engine::PreparedPlaybackWorkerStatus worker;
    SourceMetrics sources;
};

struct PackageQualificationResult
{
    std::uint64_t packageBytes = 0;
    std::uint64_t recordCount = 0;
    std::uint64_t totalMicros = 0;
    std::uint64_t peakPlaintextBytes = 0;
    std::uint64_t peakSealedBytes = 0;
    std::uint64_t verificationBytes = 0;
    double throughput = 0.0;
    std::uint64_t headBytes = 0;
    std::uint64_t metadataOpenMicros = 0;
    std::uint64_t warmMetadataOpenMicros = 0;
    std::uint64_t playableMicros = 0;
    float firstNotePeak = 0.0f;
    std::uint64_t cancellationMicros = 0;
    std::uint64_t cancellationPolls = 0;
    std::uint64_t cancellationBytesProcessed = 0;
};

PackageQualificationResult exportAndActivatePackage(
    const fs::path& packagePath,
    const drs::engine::RuntimeProjectModel& project,
    const drs::engine::SfzImportProjectionResult& projection,
    const drs::engine::PlaybackSnapshotBuildResult& fullSnapshot,
    const drs::engine::PreparedPlaybackBuildResult& fullPrepared,
    const int note,
    const int velocity)
{
    drs::app::PerformancePackageExportRequest request;
    request.project = project;
    request.projectId = project.projectId;
    request.baseRevision = 1;
    request.packagePath = packagePath.generic_string();
    request.sessionState.loadProfileId = "balanced";
    const auto operation = drs::app::executePerformancePackageExport(
        request,
        { [](const drs::app::PerformancePackageExportProgress& progress)
    {
        if (progress.stage == drs::app::PerformancePackageExportStage::sealingPackage
            && progress.bytesProcessed != 0
            && progress.bytesProcessed % (256ull * 1024ull * 1024ull) < 65536)
            std::cout << "Package export progress bytes: " << progress.bytesProcessed << "/"
                      << progress.totalBytes << std::endl;
    }, {} });
    require(operation.exported,
            "Production actual-corpus v2 export failed: " + operation.state + " :: "
                + joinIssues(operation.issues));

    PackageQualificationResult result;
    result.packageBytes = operation.packageBytes;
    result.recordCount = operation.payloadCount;
    result.totalMicros = operation.totalDurationMicros;
    result.peakPlaintextBytes = operation.peakPlaintextBufferBytes;
    result.peakSealedBytes = operation.peakSealedBufferBytes;
    result.verificationBytes = operation.verificationBytesRead;
    result.throughput = operation.plaintextThroughputBytesPerSecond;

    const auto metadataStart = Clock::now();
    const auto packageMetadata = drs::engine::loadPerformancePackageV2Metadata(
        packagePath.generic_string());
    require(packageMetadata.loaded && packageMetadata.package != nullptr,
            "Production package v2 metadata loader failed: "
                + joinIssues(packageMetadata.issues));
    auto opened = packageMetadata.package;
    result.metadataOpenMicros = elapsedMicros(metadataStart);
    require(opened->opened && opened->records.size() == result.recordCount,
            "Exported actual-corpus package failed structural reopen.");
    const auto warmMetadataStart = Clock::now();
    const auto warmOpened = drs::engine::openPackageV2(packagePath.generic_string());
    result.warmMetadataOpenMicros = elapsedMicros(warmMetadataStart);
    require(warmOpened.opened, "Warm package metadata reopen failed.");
    const auto& packageDescriptors = packageMetadata.sampleDescriptors;

    auto packagePrepared = fullPrepared;
    std::vector<std::shared_ptr<drs::engine::PackagePagedSampleDataSource>> sources;
    sources.reserve(packagePrepared.prepared.samples.size());
    for (auto& preparedSample : packagePrepared.prepared.samples)
    {
        const auto descriptor = std::find_if(packageDescriptors.begin(), packageDescriptors.end(),
            [&](const auto& candidate) { return candidate.sourceId == preparedSample.sampleSourceId; });
        require(descriptor != packageDescriptors.end(),
                "Exported package descriptor is missing a prepared sample source.");
        auto source = std::make_shared<drs::engine::PackagePagedSampleDataSource>(*descriptor, opened);
        preparedSample.dataSource = source;
        preparedSample.decodedSampleData.reset();
        sources.push_back(std::move(source));
    }
    const auto payload = drs::engine::buildPlaybackActivationPayload(
        drs::engine::PlaybackActivationLane::performance,
        fullSnapshot.requestedDraftRevision,
        &fullSnapshot,
        &packagePrepared);
    const auto model = drs::engine::buildSamplerRenderModel(payload);
    require(model.built && model.model != nullptr,
            "Exported actual-corpus package did not build the common render model.");

    drs::engine::DeferredPackageSession deferred;
    drs::engine::DeferredPackageSessionPlan plan;
    plan.packagePath = packagePath.generic_string();
    plan.package = opened;
    plan.sources = sources;
    plan.buildRenderModel = [retained = model.model] { return retained; };
    const auto playableStart = Clock::now();
    require(deferred.begin(std::move(plan)), "Deferred actual-corpus package begin failed.");
    while (deferred.snapshot().stage != drs::engine::DeferredPackageSessionStage::playable)
        require(deferred.serviceNextWorkerStep(),
                "Deferred actual-corpus package stopped before playable readiness.");
    result.playableMicros = elapsedMicros(playableStart);
    for (const auto& source : sources)
        result.headBytes += source->metrics().publishedHeadBytes;

    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::performance);
    require(context.prepare(48000.0) && deferred.stagePlayableActivation(context),
            "Deferred actual-corpus package activation staging failed.");
    constexpr std::uint32_t framesPerBlock = 256;
    std::array<std::vector<float>, 2> outputStorage {
        std::vector<float>(framesPerBlock), std::vector<float>(framesPerBlock)
    };
    std::array<float*, 2> channels { outputStorage[0].data(), outputStorage[1].data() };
    for (std::size_t block = 0; block < 8; ++block)
    {
        std::fill(outputStorage[0].begin(), outputStorage[0].end(), 0.0f);
        std::fill(outputStorage[1].begin(), outputStorage[1].end(), 0.0f);
        std::array<drs::engine::SamplerRenderEvent, 1> events {};
        std::size_t eventCount = 0;
        if (block == 0)
        {
            events[0].type = drs::engine::SamplerRenderEventType::noteOn;
            events[0].midiNote = static_cast<std::uint8_t>(note);
            events[0].velocity = static_cast<float>(velocity) / 127.0f;
            eventCount = 1;
        }
        const auto rendered = context.renderBlock(
            { channels.data(), 2, framesPerBlock }, { events.data(), eventCount });
        require(rendered.accepted, "Package first-note render block failed.");
        if (block == 0)
            require(deferred.observeAudioCutover(context),
                    "Package callback cutover was not observed.");
        for (const auto& channel : outputStorage)
            for (const auto value : channel)
                result.firstNotePeak = std::max(result.firstNotePeak, std::abs(value));
    }
    require(result.firstNotePeak > 1.0e-5f,
            "Exported actual-corpus package first note was inaudible.");

    const auto canceledPath = packagePath.parent_path()
        / (packagePath.stem().generic_string() + "-cancelled.drpkg");
    std::error_code cleanupError;
    fs::remove(canceledPath, cleanupError);
    fs::remove(fs::path(canceledPath.generic_string() + ".stage"), cleanupError);
    auto cancellationRequest = request;
    cancellationRequest.packagePath = canceledPath.generic_string();
    const auto cancellationStart = Clock::now();
    const auto canceled = drs::app::executePerformancePackageExport(
        cancellationRequest,
        { [&](const drs::app::PerformancePackageExportProgress& progress)
        {
            if (progress.stage == drs::app::PerformancePackageExportStage::sealingPackage)
            {
                result.cancellationBytesProcessed = std::max(
                    result.cancellationBytesProcessed, progress.bytesProcessed);
            }
        }, [&]
        {
            ++result.cancellationPolls;
            return result.cancellationBytesProcessed >= 64ull * 1024ull * 1024ull;
        } });
    result.cancellationMicros = elapsedMicros(cancellationStart);
    require(canceled.canceled && !canceled.exported,
            "Actual-corpus production export cancellation was not honored.");
    require(!fs::exists(canceledPath)
                && !fs::exists(fs::path(canceledPath.generic_string() + ".stage")),
            "Actual-corpus canceled export left a publishable or staging package.");
    return result;
}

SustainedPlaybackResult runSustainedPlayback(
    const drs::engine::PlaybackSnapshotBuildResult& snapshot,
    const int note,
    const int velocity,
    const std::uint64_t pageServicePollMilliseconds = 5)
{
    drs::engine::PreparedPlaybackSchedulerBudgets budgets;
    budgets.maximumRetainedPreparedBytes = 1;
    budgets.pageServicePollMilliseconds = pageServicePollMilliseconds;
    drs::engine::PreparedPlaybackService service("large-instrument-qualification-live", 2, true, budgets);
    const drs::engine::RuntimeStreamLoadResult noCompiledStream;
    service.setBackgroundWorkerStream(noCompiledStream);
    require(service.enqueuePreviewBuild(snapshot).accepted,
            "Live preparation request was rejected.");
    require(service.waitForWorkerIdle(30000),
            "Live preparation worker did not reach idle within 30 seconds.");
    auto completed = service.drainCompletedBuilds();
    require(completed.size() == 1 && completed.front().result.built,
            "Live preparation did not publish one completed result.");
    auto& prepared = completed.front().result;

    const auto payload = drs::engine::buildPlaybackActivationPayload(
        drs::engine::PlaybackActivationLane::preview,
        snapshot.requestedDraftRevision,
        &snapshot,
        &prepared);
    const auto renderBuild = drs::engine::buildSamplerRenderModel(payload);
    require(renderBuild.built && renderBuild.model != nullptr,
            "Streaming preparation did not build a render model.");

    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::preview);
    require(context.prepare(48000.0), "Playback context preparation failed.");
    require(context.stageActivation(renderBuild.model), "Playback activation staging failed.");

    SustainedPlaybackResult result;
    const auto start = Clock::now();
    constexpr std::uint32_t framesPerBlock = 256;
    constexpr std::size_t blockCount = 375; // Two seconds at 48 kHz.
    std::array<std::vector<float>, 2> outputStorage {
        std::vector<float>(framesPerBlock), std::vector<float>(framesPerBlock)
    };
    std::array<float*, 2> channels {
        outputStorage[0].data(), outputStorage[1].data()
    };

    for (std::size_t block = 0; block < blockCount; ++block)
    {
        std::fill(outputStorage[0].begin(), outputStorage[0].end(), 0.0f);
        std::fill(outputStorage[1].begin(), outputStorage[1].end(), 0.0f);
        drs::engine::SamplerAudioBufferView output { channels.data(), 2, framesPerBlock };
        std::array<drs::engine::SamplerRenderEvent, 8> events {};
        std::size_t eventCount = 0;
        if (block == 0)
        {
            for (std::size_t voice = 0; voice < 8; ++voice)
            {
                events[voice].type = drs::engine::SamplerRenderEventType::noteOn;
                events[voice].midiNote = static_cast<std::uint8_t>(note);
                events[voice].velocity = static_cast<float>(velocity) / 127.0f;
                events[voice].inputSequence = static_cast<std::uint32_t>(voice + 1);
            }
            eventCount = events.size();
        }
        const auto renderStarted = Clock::now();
        const auto render = context.renderBlock(output, { events.data(), eventCount });
        result.maximumRenderMicros = std::max(
            result.maximumRenderMicros, elapsedMicros(renderStarted));
        require(render.accepted, "Sustained streaming render block was rejected.");
        result.pageMisses += render.voicePool.render.pageMissCount;
        result.underrunFrames += render.voicePool.render.underrunFrameCount;
        result.recoveries += render.voicePool.render.pageRecoveryCount;
        for (const auto& channel : outputStorage)
        {
            for (const auto value : channel)
                result.peak = std::max(result.peak, std::abs(value));
        }
        // Pace the synthetic callback loop no faster than a 48 kHz device so the
        // asynchronous page worker is measured under a realistic service window.
        std::this_thread::sleep_for(std::chrono::milliseconds(6));
    }
    result.elapsed = elapsedMicros(start);
    result.worker = service.getWorkerStatus();
    result.sources = collectSourceMetrics(prepared.prepared);
    require(result.peak > 1.0e-5f, "Sustained streamed playback was inaudible.");
    require(result.worker.pagePrepareCount > 0,
            "Sustained playback published no worker-prepared pages beyond the resident head.");
    require(result.worker.pagePrepareFailureCount == 0,
            "Sustained playback encountered page preparation failures.");
    require(result.maximumRenderMicros < 5333,
            "Sustained playback exceeded one 256-frame callback period at 48 kHz.");
    return result;
}

std::uint64_t fileTreeBytes(const fs::path& root, std::size_t& wavCount)
{
    std::uint64_t bytes = 0;
    wavCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() == ".wav" || entry.path().extension() == ".WAV")
        {
            ++wavCount;
            bytes += entry.file_size();
        }
    }
    return bytes;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        require(argc >= 4,
                "Usage: drs_large_instrument_qualification <instrument.sfz> <report.md> <package.drpkg>");
        const auto sfzPath = fs::absolute(fs::path(argv[1]));
        const auto reportPath = fs::absolute(fs::path(argv[2]));
        const auto packagePath = fs::absolute(fs::path(argv[3]));
        require(fs::is_regular_file(sfzPath), "SFZ qualification input does not exist.");

        std::size_t wavCount = 0;
        const auto corpusBytes = fileTreeBytes(sfzPath.parent_path().parent_path(), wavCount);
        const auto blankProject = makeBlankProject(sfzPath);

        const auto importStart = Clock::now();
        const auto projection = drs::engine::projectSfzImportDocument(
            blankProject, sfzPath.generic_string());
        const auto importMicros = elapsedMicros(importStart);
        require(projection.projected && projection.playable && !projection.blocking,
                "Salamander import projection failed: " + projection.state + " :: "
                    + joinIssues(projection.issues));
        require(projection.sampleSources.size() >= 641,
                "Salamander projection did not retain at least 641 sample sources.");
        require(projection.zones.size() >= 1704,
                "Salamander projection did not retain at least 1,704 zones/routes.");

        drs::engine::AuthoringSession session(blankProject);
        const auto applyStart = Clock::now();
        const auto apply = drs::engine::applySfzImportProjection(
            session, projection, "Import Accurate Salamander qualification corpus");
        const auto applyMicros = elapsedMicros(applyStart);
        require(apply.applied, "Salamander import projection did not apply.");

        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        const auto snapshotStart = Clock::now();
        const auto fullSnapshot = buildSnapshot(snapshotBuilder, session.getProject(), 1);
        const auto snapshotMicros = elapsedMicros(snapshotStart);
        require(fullSnapshot.snapshot.sampleIdentities.size() >= 641
                    && fullSnapshot.snapshot.zones.size() >= 1704,
                "Full-draft snapshot lost Salamander dependencies.");

        const auto& anchorZone = fullSnapshot.snapshot.zones.front();
        const auto scopedZone = drs::engine::scopePlaybackSnapshotForPreparation(
            fullSnapshot,
            { drs::engine::PlaybackPreparationScope::selectedZone, anchorZone.id, {} });
        const auto scopedGroup = drs::engine::scopePlaybackSnapshotForPreparation(
            fullSnapshot,
            { drs::engine::PlaybackPreparationScope::selectedGroup, {}, anchorZone.groupId });
        require(scopedZone.built && scopedZone.retainedZoneCount < fullSnapshot.snapshot.zones.size(),
                "Selected-zone scope did not reduce the full draft.");
        require(scopedGroup.built && scopedGroup.retainedZoneCount < fullSnapshot.snapshot.zones.size(),
                "Selected-group scope did not reduce the full draft.");

        const auto zonePrepared = prepareSynchronously(scopedZone);
        const auto groupPrepared = prepareSynchronously(scopedGroup);
        const auto fullPrepared = prepareSynchronously(fullSnapshot);
        require(fullPrepared.step.result.admission.estimatedDecodedBytes
                    > fullPrepared.step.result.admission.residentBudgetBytes,
                "Actual Salamander metadata did not exceed resident admission.");
        require(fullPrepared.step.result.admission.readiness
                    == drs::engine::PreparedPlaybackReadinessState::playable,
                "Actual Salamander full draft did not reach streaming playable readiness.");
        const auto fullSources = collectSourceMetrics(fullPrepared.step.result.prepared);
        require(fullSources.headBytes <= 16ull * 1024ull * projection.sampleSources.size(),
                "Prepared Salamander heads exceeded the configured 16 KiB-per-source ceiling.");

        const auto zonePlayback = runSustainedPlayback(
            scopedZone,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127));
        const auto groupPlayback = runSustainedPlayback(
            scopedGroup,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127));
        const auto constrainedPlayback = runSustainedPlayback(
            scopedZone,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127),
            75);
        require(zonePlayback.pageMisses == 0 && zonePlayback.underrunFrames == 0
                    && groupPlayback.pageMisses == 0 && groupPlayback.underrunFrames == 0,
                "Normal-storage Salamander playback reported a page miss or underrun.");
        const auto package = exportAndActivatePackage(
            packagePath, session.getProject(), projection, fullSnapshot,
            fullPrepared.step.result,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127));

        fs::create_directories(reportPath.parent_path());
        std::ofstream report(reportPath, std::ios::binary | std::ios::trunc);
        require(report.good(), "Could not create the qualification report.");
        report << "# Accurate Salamander large-instrument qualification\n\n"
               << "Result: PASS\n\n"
               << "Signed by: DRS automated large-instrument qualification (Release)\n\n"
               << "Corpus: `" << sfzPath.generic_string() << "`\n\n"
               << "- WAV files: " << wavCount << "\n"
               << "- Corpus WAV bytes: " << corpusBytes << "\n"
               << "- Projected sources: " << projection.sampleSources.size() << "\n"
               << "- Projected zones/routes: " << projection.zones.size() << "\n"
               << "- Import analysis/projection: " << importMicros << " us\n"
               << "- Atomic authoring apply: " << applyMicros << " us\n"
               << "- Full snapshot: " << snapshotMicros << " us\n"
               << "- Selected-zone retained zones/sources: " << scopedZone.retainedZoneCount
               << "/" << scopedZone.retainedSampleCount << "\n"
               << "- Selected-group retained zones/sources: " << scopedGroup.retainedZoneCount
               << "/" << scopedGroup.retainedSampleCount << "\n"
               << "- Selected-zone preparation: " << zonePrepared.elapsed << " us\n"
               << "- Selected-group preparation: " << groupPrepared.elapsed << " us\n"
               << "- Full-draft preparation: " << fullPrepared.elapsed << " us\n"
               << "- Estimated resident decoded bytes: "
               << fullPrepared.step.result.admission.estimatedDecodedBytes << "\n"
               << "- Full-draft decoded bytes: " << fullPrepared.step.result.metrics.decodedBytes << "\n"
               << "- Full-draft resident-head bytes: " << fullSources.headBytes << "\n"
               << "- Selected-zone sustained peak/elapsed: " << std::setprecision(9)
               << zonePlayback.peak << "/" << zonePlayback.elapsed << " us\n"
               << "- Selected-group sustained peak/elapsed: " << groupPlayback.peak
               << "/" << groupPlayback.elapsed << " us\n"
               << "- Zone worker intents/prepared/failures: " << zonePlayback.worker.pageIntentCount
               << "/" << zonePlayback.worker.pagePrepareCount << "/"
               << zonePlayback.worker.pagePrepareFailureCount << "\n"
               << "- Zone page cache bytes: " << zonePlayback.sources.pageBytes << "\n"
               << "- Zone maximum page read latency: "
               << zonePlayback.sources.maximumReadMicros << " us\n"
               << "- Zone maximum callback duration/budget: "
               << zonePlayback.maximumRenderMicros << "/5333 us\n"
               << "- Zone intent published/consumed/dropped/max-depth: "
               << zonePlayback.sources.intentPublished << "/"
               << zonePlayback.sources.intentConsumed << "/"
               << zonePlayback.sources.intentDropped << "/"
               << zonePlayback.sources.maximumIntentDepth << "\n"
               << "- Zone cache budget/peak/leased/retired bytes: "
               << zonePlayback.sources.pageCacheBudgetBytes << "/"
               << zonePlayback.sources.maximumAllocatedPageBytes << "/"
               << zonePlayback.sources.leasedPageBytes << "/"
               << zonePlayback.sources.retiredPageBytes << "\n"
               << "- Zone page misses/underrun frames/recoveries: " << zonePlayback.pageMisses
               << "/" << zonePlayback.underrunFrames << "/" << zonePlayback.recoveries << "\n"
               << "- Group intents/prepared/failures: "
               << groupPlayback.worker.pageIntentCount << "/"
               << groupPlayback.worker.pagePrepareCount << "/"
               << groupPlayback.worker.pagePrepareFailureCount << "\n"
               << "- Group page misses/underrun frames/recoveries: "
               << groupPlayback.pageMisses << "/" << groupPlayback.underrunFrames
               << "/" << groupPlayback.recoveries << "\n"
               << "- Constrained profile: 75 ms page-service poll\n"
               << "- Constrained peak/elapsed: " << constrainedPlayback.peak
               << "/" << constrainedPlayback.elapsed << " us\n"
               << "- Constrained intents/prepared/failures: "
               << constrainedPlayback.worker.pageIntentCount << "/"
               << constrainedPlayback.worker.pagePrepareCount << "/"
               << constrainedPlayback.worker.pagePrepareFailureCount << "\n"
               << "- Constrained page misses/underrun frames/recoveries: "
               << constrainedPlayback.pageMisses << "/" << constrainedPlayback.underrunFrames
               << "/" << constrainedPlayback.recoveries << "\n"
               << "- Peak process working set: " << peakWorkingSetBytes() << " bytes\n\n"
               << "## Package v2\n\n"
               << "- Package bytes: " << package.packageBytes << "\n"
               << "- Records: " << package.recordCount << "\n"
               << "- Export elapsed: " << package.totalMicros << " us\n"
               << "- Export throughput: " << package.throughput << " plaintext bytes/s\n"
               << "- Peak plaintext/sealed buffers: " << package.peakPlaintextBytes
               << "/" << package.peakSealedBytes << " bytes\n"
               << "- Structural verification bytes: " << package.verificationBytes << "\n"
               << "- Metadata open: " << package.metadataOpenMicros << " us\n"
               << "- Warm metadata reopen: " << package.warmMetadataOpenMicros << " us\n"
               << "- Head-ready/playable: " << package.playableMicros << " us\n"
               << "- Package resident-head bytes: " << package.headBytes << "\n"
               << "- Package first-note peak: " << package.firstNotePeak << "\n"
               << "- Actual-corpus cancellation latency/polls: "
               << package.cancellationMicros << " us/" << package.cancellationPolls << "\n"
               << "- Actual-corpus cancellation bytes processed: "
               << package.cancellationBytesProcessed << "\n\n"
               << "The run used real corpus WAV descriptors, bounded 16 KiB heads, the production "
                  "page-intent worker, the immutable render model, and callback-side activation.\n";
        require(report.good(), "Qualification report write failed.");

        std::cout << "Accurate Salamander qualification passed: sources="
                  << projection.sampleSources.size() << " zones=" << projection.zones.size()
                  << " fullPrepareMicros=" << fullPrepared.elapsed
                  << " headBytes=" << fullSources.headBytes
                  << " pagePrepared=" << zonePlayback.worker.pagePrepareCount
                  << " packageBytes=" << package.packageBytes
                  << " peakWorkingSet=" << peakWorkingSetBytes() << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Accurate Salamander qualification failed: " << error.what() << std::endl;
        return 1;
    }
}
