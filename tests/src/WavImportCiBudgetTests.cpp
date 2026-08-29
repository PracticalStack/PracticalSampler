#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/WaveformPreviewService.h"
#include "shared/WavImportService.h"
#include "../support/WavImportTestSupport.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct StagePause
{
    std::mutex mutex;
    std::condition_variable condition;
    bool reached = false;
    bool released = false;

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        reached = true;
        condition.notify_all();
        condition.wait(lock, [&] { return released; });
    }

    bool waitUntilReached(const std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return condition.wait_for(lock, timeout, [&] { return reached; });
    }

    void release()
    {
        std::lock_guard<std::mutex> lock(mutex);
        released = true;
        condition.notify_all();
    }
};

struct StagePauseReleaseGuard
{
    StagePause& pause;

    ~StagePauseReleaseGuard()
    {
        pause.release();
    }
};

struct RequestFixture
{
    fs::path root;
    drs::tests::GeneratedWavImportBatchCorpus corpus;
    drs::app::WavImportRequest request;
};

RequestFixture makeLargeBatchFixture(const std::string& projectId,
                                     const std::size_t itemCount)
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    RequestFixture fixture;
    fixture.root = fs::temp_directory_path() / ("drs-wav-import-ci-budgets-" + projectId + "-" + unique);
    const auto sourceDirectory = fixture.root / "source";
    const auto contentRoot = fixture.root / "project";
    fs::create_directories(contentRoot / "Samples");
    fixture.corpus = drs::tests::createGeneratedWavImportBatchCorpus(sourceDirectory);

    fixture.request.projectId = projectId;
    fixture.request.baseRevision = 9;
    fixture.request.contentRootPath = contentRoot.generic_string();
    fixture.request.selectedGroupId = "default-group";
    fixture.request.sourcePaths.assign(itemCount, fixture.corpus.cleanPath.generic_string());
    return fixture;
}

std::uint64_t regularFileCount(const fs::path& root)
{
    std::uint64_t count = 0;
    if (!fs::exists(root))
        return 0;

    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file())
            ++count;
    }

    return count;
}

std::uint64_t totalImportIoOperations(const drs::engine::SampleImportIoCounters& counters)
{
    return counters.fingerprintOpenCount
        + counters.readerOpenCount
        + counters.bytesReadCount
        + counters.fullFrameReadCount
        + counters.copyCount
        + counters.peakChunkReadCount;
}

std::string describeImportIoCounters(const drs::engine::SampleImportIoCounters& counters)
{
    std::ostringstream description;
    description << "fingerprintOpenCount=" << counters.fingerprintOpenCount
                << ", readerOpenCount=" << counters.readerOpenCount
                << ", bytesReadCount=" << counters.bytesReadCount
                << ", fullFrameReadCount=" << counters.fullFrameReadCount
                << ", copyCount=" << counters.copyCount
                << ", peakChunkReadCount=" << counters.peakChunkReadCount;
    return description.str();
}

std::uint64_t residentStringBytes(const std::string& value)
{
    return static_cast<std::uint64_t>(value.capacity());
}

std::uint64_t estimateResidentBytes(const drs::app::WavImportCompletionItem& item)
{
    std::uint64_t bytes = sizeof(item);
    bytes += residentStringBytes(item.itemId);
    bytes += residentStringBytes(item.sourcePath);
    bytes += residentStringBytes(item.stagedPath);
    bytes += residentStringBytes(item.finalPath);
    bytes += residentStringBytes(item.fingerprint.sourcePath);
    bytes += residentStringBytes(item.fingerprint.fingerprintHex);
    bytes += residentStringBytes(item.fingerprint.state);
    bytes += item.fingerprint.issues.capacity() * sizeof(std::string);
    for (const auto& issue : item.fingerprint.issues)
        bytes += residentStringBytes(issue);
    bytes += residentStringBytes(item.inspection.sourcePath);
    bytes += residentStringBytes(item.inspection.state);
    bytes += residentStringBytes(item.inspection.metadata.sourcePath);
    bytes += residentStringBytes(item.inspection.metadata.formatName);
    bytes += residentStringBytes(item.inspection.metadata.sourceChecksumHex);
    bytes += residentStringBytes(item.inspection.metadata.channelLayout);
    bytes += item.inspection.warnings.capacity() * sizeof(std::string);
    for (const auto& warning : item.inspection.warnings)
        bytes += residentStringBytes(warning);
    bytes += item.inspection.issues.capacity() * sizeof(std::string);
    for (const auto& issue : item.inspection.issues)
        bytes += residentStringBytes(issue);
    bytes += item.filenameTokens.capacity() * sizeof(drs::engine::SampleFilenameToken);
    for (const auto& token : item.filenameTokens)
    {
        bytes += residentStringBytes(token.text);
        bytes += residentStringBytes(token.normalizedText);
        bytes += residentStringBytes(token.canonicalValue);
    }
    bytes += item.findings.capacity() * sizeof(drs::engine::AuthoringImportFinding);
    for (const auto& finding : item.findings)
    {
        bytes += residentStringBytes(finding.code);
        bytes += residentStringBytes(finding.summary);
        bytes += residentStringBytes(finding.detail);
        bytes += finding.relatedTokens.capacity() * sizeof(std::string);
        for (const auto& token : finding.relatedTokens)
            bytes += residentStringBytes(token);
    }
    bytes += residentStringBytes(item.suggestedZone.sourceSampleId);
    bytes += residentStringBytes(item.suggestedZone.rootKeySource);
    bytes += residentStringBytes(item.suggestedZone.velocitySource);
    bytes += residentStringBytes(item.suggestedZone.zone.id);
    bytes += residentStringBytes(item.suggestedZone.zone.sampleSourceId);
    bytes += residentStringBytes(item.suggestedZone.zone.displayName);
    bytes += residentStringBytes(item.suggestedZone.zone.groupId);
    bytes += residentStringBytes(item.suggestedZone.zone.articulationId);
    return bytes;
}

std::uint64_t estimateResidentBytes(const drs::app::WavImportBatchSnapshot& snapshot)
{
    std::uint64_t bytes = sizeof(snapshot);
    bytes += residentStringBytes(snapshot.identity.projectId);
    bytes += residentStringBytes(snapshot.identity.contentRootPath);
    bytes += residentStringBytes(snapshot.identity.selectedGroupId);
    bytes += residentStringBytes(snapshot.status);
    bytes += snapshot.items.capacity() * sizeof(drs::app::WavImportItemProgress);
    for (const auto& item : snapshot.items)
    {
        bytes += residentStringBytes(item.itemId);
        bytes += residentStringBytes(item.sourcePath);
        bytes += residentStringBytes(item.stagedPath);
        bytes += residentStringBytes(item.status);
    }

    if (snapshot.completion != nullptr)
    {
        bytes += sizeof(*snapshot.completion);
        bytes += residentStringBytes(snapshot.completion->identity.projectId);
        bytes += residentStringBytes(snapshot.completion->identity.contentRootPath);
        bytes += residentStringBytes(snapshot.completion->identity.selectedGroupId);
        bytes += residentStringBytes(snapshot.completion->status);
        bytes += snapshot.completion->items.capacity() * sizeof(drs::app::WavImportCompletionItem);
        for (const auto& item : snapshot.completion->items)
            bytes += estimateResidentBytes(item);
    }

    return bytes;
}

std::uint64_t estimateWaveformPeakWorkingBytes(
    const drs::app::WaveformPreviewRequest& request,
    const drs::engine::WaveformPeakBuildResult& result)
{
    const auto totalFrames = result.metadata.frameCount;
    const auto displayPointCount = std::max<std::size_t>(
        1,
        std::min<std::size_t>(request.displayPointCount, static_cast<std::size_t>(totalFrames)));
    const auto chunkFrameCount = std::max<std::uint64_t>(1, request.chunkFrameCount);
    const auto bufferFrameCapacity = std::min<std::uint64_t>(chunkFrameCount, totalFrames);
    const auto chunkBufferBytes = bufferFrameCapacity * result.metadata.channelCount * sizeof(float);
    const auto resultPointBytes = displayPointCount * sizeof(drs::engine::WaveformPeakPoint);
    const auto pointInitializedBytes = displayPointCount;
    return chunkBufferBytes + resultPointBytes + pointInitializedBytes;
}

std::string describeTimingBudget(const char* label,
                                 const std::uint64_t measuredMicros,
                                 const std::uint64_t thresholdMicros)
{
    std::ostringstream description;
    description << label << "=" << measuredMicros
                << "us (threshold=" << thresholdMicros
                << "us, within=" << (measuredMicros <= thresholdMicros ? "true" : "false") << ")";
    return description.str();
}

struct CiBudgetThresholds
{
    static constexpr std::size_t largeBatchItemCount = 256;
    static constexpr std::uint64_t maximumLargeBatchResidentBytes = 192ull * 1024ull;
    static constexpr std::uint64_t maximumWaveformPeakWorkingBytes = 40ull * 1024ull;
#if defined(_DEBUG)
    static constexpr std::uint64_t maximumWavImportCancellationMicros = 250000ull;
    static constexpr std::uint64_t maximumWaveformPreviewCancellationMicros = 150000ull;
#else
    static constexpr std::uint64_t maximumWavImportCancellationMicros = 100000ull;
    static constexpr std::uint64_t maximumWaveformPreviewCancellationMicros = 50000ull;
#endif
};
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::engine::resetSampleImportIoCounters();
        drs::plugin::Processor constructorProcessor;
        constructorProcessor.prepareToPlay(44100.0, 64);
        juce::MemoryBlock constructorState;
        constructorProcessor.getStateInformation(constructorState);
        const auto constructorCounters = drs::engine::getSampleImportIoCounters();
        require(totalImportIoOperations(constructorCounters) == 0,
                "WAV CI constructor budget must stay at zero import I/O operations, observed "
                    + describeImportIoCounters(constructorCounters));

        StagePause importPause;
        drs::app::WavImportServiceOptions importOptions;
        importOptions.stageObserver = [&](const drs::app::WavImportBatchStage stage)
        {
            if (stage == drs::app::WavImportBatchStage::staging)
                importPause.wait();
        };

        drs::app::WavImportService importService(std::move(importOptions));
        StagePauseReleaseGuard importPauseRelease { importPause };
        auto importClient = importService.openClient();
        const auto importFixture = makeLargeBatchFixture("wav-ci-budget-import", CiBudgetThresholds::largeBatchItemCount);
        drs::engine::resetSampleImportIoCounters();
        const auto importAccepted = importClient.submit(importFixture.request);
        require(importAccepted.wasAccepted(),
                "WAV CI large-batch import request must be accepted.");
        require(importPause.waitUntilReached(5s),
                "WAV CI large-batch import request did not reach the staging checkpoint.");
        const auto importSubmitCounters = drs::engine::getSampleImportIoCounters();
        require(totalImportIoOperations(importSubmitCounters) == 0,
                "WAV CI import submit budget must stay at zero inline import I/O operations, observed "
                    + describeImportIoCounters(importSubmitCounters));
        const auto importSnapshot = importClient.getSnapshot();
        require(importSnapshot != nullptr
                    && importSnapshot->stage == drs::app::WavImportBatchStage::staging
                    && importSnapshot->items.size() == CiBudgetThresholds::largeBatchItemCount,
                "WAV CI large-batch import snapshot must remain staged with the full tracked batch.");
        const auto residentBatchBytes = estimateResidentBytes(*importSnapshot);
        require(residentBatchBytes <= CiBudgetThresholds::maximumLargeBatchResidentBytes,
                "WAV CI large-batch resident snapshot bytes exceeded the reviewed budget.");
        require(importClient.cancel("WAV CI import cancellation"),
                "WAV CI large-batch import request must accept cancellation.");
        const auto importCancelStart = std::chrono::steady_clock::now();
        importPause.release();
        require(importClient.waitForTerminal(5s),
                "WAV CI large-batch import request did not reach a terminal state after cancellation.");
        const auto wavImportCancellationMicros
            = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                             std::chrono::steady_clock::now() - importCancelStart)
                                             .count());
        require(regularFileCount(fs::path(importFixture.request.contentRootPath) / "Samples") == 0,
                "WAV CI large-batch import cancellation leaked staged files.");

        StagePause previewPause;
        drs::tests::DeterministicSampleImportHooks previewHooks;
        const std::string previewSamplePath = "wav-ci-budget-preview.wav";
        previewHooks.addReaderFixture({
            previewSamplePath,
            "WAV file",
            48000.0,
            32768,
            2,
            32,
            true,
            {},
            false
        });

        drs::app::WaveformPreviewServiceOptions previewOptions;
        previewOptions.sampleImportHooks = &previewHooks;
        previewOptions.stageObserver = [&](const drs::app::WaveformPreviewServiceStage stage)
        {
            if (stage == drs::app::WaveformPreviewServiceStage::building)
                previewPause.wait();
        };

        drs::app::WaveformPreviewService previewService(std::move(previewOptions));
        StagePauseReleaseGuard previewPauseRelease { previewPause };
        drs::app::WaveformPreviewRequest previewRequest;
        previewRequest.projectId = "wav-ci-budget-preview";
        previewRequest.baseRevision = 11;
        previewRequest.contentRootPath = "synthetic";
        previewRequest.sampleSourceId = "sample-source";
        previewRequest.sourcePath = previewSamplePath;
        previewRequest.displayPointCount = 512;
        previewRequest.chunkFrameCount = 4096;

        drs::engine::resetSampleImportIoCounters();
        const auto previewAccepted = previewService.submit(previewRequest);
        require(previewAccepted.accepted,
                "WAV CI waveform preview request must be accepted.");
        require(previewPause.waitUntilReached(5s),
                "WAV CI waveform preview request did not reach the building checkpoint.");
        const auto previewSubmitCounters = drs::engine::getSampleImportIoCounters();
        require(totalImportIoOperations(previewSubmitCounters) == 0,
                "WAV CI waveform preview submit budget must stay at zero inline sample I/O operations, observed "
                    + describeImportIoCounters(previewSubmitCounters));
        const auto buildingSnapshot = previewService.getSnapshot();
        require(buildingSnapshot != nullptr
                    && buildingSnapshot->stage == drs::app::WaveformPreviewServiceStage::building,
                "WAV CI waveform preview request must remain in building while paused.");
        require(previewService.cancel("WAV CI waveform preview cancellation"),
                "WAV CI waveform preview request must accept cancellation.");
        const auto previewCancelStart = std::chrono::steady_clock::now();
        previewPause.release();
        require(previewService.waitForTerminal(5s),
                "WAV CI waveform preview request did not reach a terminal state after cancellation.");
        const auto waveformPreviewCancellationMicros
            = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                             std::chrono::steady_clock::now() - previewCancelStart)
                                             .count());
        const auto canceledPreview = previewService.getSnapshot();
        require(canceledPreview != nullptr
                    && (canceledPreview->stage == drs::app::WaveformPreviewServiceStage::canceled
                        || canceledPreview->stage == drs::app::WaveformPreviewServiceStage::superseded)
                    && canceledPreview->result != nullptr,
                "WAV CI waveform preview request must publish a terminal canceled result.");
        const auto waveformPeakWorkingBytes
            = estimateWaveformPeakWorkingBytes(previewRequest, *canceledPreview->result);
        require(waveformPeakWorkingBytes <= CiBudgetThresholds::maximumWaveformPeakWorkingBytes,
                "WAV CI waveform peak working-set estimate exceeded the reviewed budget.");

        std::cout << "WAV CI budget diagnostics: constructorIoOps="
                  << totalImportIoOperations(constructorCounters)
                  << ", importSubmitIoOps=" << totalImportIoOperations(importSubmitCounters)
                  << ", previewSubmitIoOps=" << totalImportIoOperations(previewSubmitCounters)
                  << ", residentBatchBytes=" << residentBatchBytes
                  << ", waveformPeakWorkingBytes=" << waveformPeakWorkingBytes
                  << ", " << describeTimingBudget("wavImportCancel",
                                                  wavImportCancellationMicros,
                                                  CiBudgetThresholds::maximumWavImportCancellationMicros)
                  << ", " << describeTimingBudget("waveformPreviewCancel",
                                                  waveformPreviewCancellationMicros,
                                                  CiBudgetThresholds::maximumWaveformPreviewCancellationMicros)
                  << std::endl;
        std::cout << "WAV import CI budget tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import CI budget tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
