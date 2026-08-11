#include "WavImportTestSupport.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void processBlock(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
}

std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> waitForRestore(
    drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor);
        processor.serviceMessageThreadWork();
        const auto snapshot = processor.getProjectRestoreSnapshot();
        if (snapshot != nullptr
            && (snapshot->state == drs::engine::ProjectRestoreState::active
                || snapshot->state == drs::engine::ProjectRestoreState::ready
                || snapshot->state == drs::engine::ProjectRestoreState::failed
                || snapshot->state == drs::engine::ProjectRestoreState::needsLocation))
            return snapshot;
        std::this_thread::sleep_for(2ms);
    }

    throw std::runtime_error("WAV baseline restore timed out.");
}

struct TimedCounters
{
    std::uint64_t durationMicros = 0;
    drs::engine::SampleImportIoCounters counters;
};

template <typename Callback>
TimedCounters measureTimedCounters(Callback&& callback)
{
    drs::engine::resetSampleImportIoCounters();
    const auto start = std::chrono::steady_clock::now();
    callback();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    return { static_cast<std::uint64_t>(duration.count()), drs::engine::getSampleImportIoCounters() };
}

struct BatchMetrics
{
    TimedCounters importSubmit;
    TimedCounters fullBatch;
    drs::engine::AuthoringImportQueueMetrics queueMetrics;
    std::size_t skippedCount = 0;
    std::uint64_t largestDecodedSampleBytes = 0;
    std::uint64_t retainedQueueBytes = 0;
    std::uint64_t estimatedPeakWorkingBytes = 0;
    bool completedInline = false;
};

BatchMetrics measureSynchronousBatchImport(const fs::path& scratchDirectory)
{
    fs::remove_all(scratchDirectory);
    const auto corpus = drs::tests::createGeneratedWavImportBatchCorpus(scratchDirectory / "corpus");
    const auto samplesDirectory = scratchDirectory / "Samples";
    fs::create_directories(samplesDirectory);

    std::vector<std::string> copiedPaths;
    std::size_t skippedCount = 0;
    drs::engine::AuthoringImportQueue queue;

    drs::engine::resetSampleImportIoCounters();
    const auto start = std::chrono::steady_clock::now();

    const std::vector<fs::path> selectedFiles {
        corpus.cleanPath,
        corpus.ambiguousPath,
        corpus.conflictPath,
        corpus.canceledPath,
        corpus.policyWarningPath,
        corpus.unsupportedPath,
        corpus.missingPath
    };

    for (const auto& sourcePath : selectedFiles)
    {
        if (!fs::exists(sourcePath))
        {
            ++skippedCount;
            continue;
        }

        const auto managedCopy = samplesDirectory / sourcePath.filename();
        require(drs::engine::copySampleFileForImport(sourcePath.generic_string(),
                                                     managedCopy.generic_string()),
                "Baseline mixed-batch copy must preserve the current synchronous copy behavior.");
        copiedPaths.push_back(managedCopy.generic_string());
    }

    queue = drs::engine::createAuthoringImportQueue(copiedPaths, scratchDirectory.generic_string());
    if (queue.items.size() > 3)
        drs::engine::cancelAuthoringImportQueueItem(queue, queue.items[3].id);

    while (drs::engine::processNextAuthoringImportQueueItem(queue).processed)
    {
    }

    if (!queue.items.empty())
        drs::engine::acceptAuthoringImportQueueItem(queue, queue.items.front().id);

    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    const TimedCounters timed {
        static_cast<std::uint64_t>(duration.count()),
        drs::engine::getSampleImportIoCounters()
    };

    std::uint64_t retainedQueueBytes = 0;
    std::uint64_t estimatedPeakWorkingBytes = 0;
    std::uint64_t largestDecodedSampleBytes = 0;
    for (const auto& item : queue.items)
    {
        if (!item.inspectionResult.inspected || !item.inspectionResult.accepted)
            continue;
    }

    return {
        timed,
        timed,
        queue.metrics,
        skippedCount,
        largestDecodedSampleBytes,
        retainedQueueBytes,
        estimatedPeakWorkingBytes,
        true
    };
}

std::string jsonEscape(const std::string& text)
{
    std::ostringstream escaped;
    for (const auto character : text)
    {
        switch (character)
        {
        case '\\':
            escaped << "\\\\";
            break;
        case '"':
            escaped << "\\\"";
            break;
        case '\n':
            escaped << "\\n";
            break;
        default:
            escaped << character;
            break;
        }
    }

    return escaped.str();
}

void appendCounterJson(std::ostringstream& stream,
                       const drs::engine::SampleImportIoCounters& counters,
                       const int indent)
{
    const std::string pad(static_cast<std::size_t>(indent), ' ');
    stream << pad << "\"fingerprintOpenCount\": " << counters.fingerprintOpenCount << ",\n";
    stream << pad << "\"readerOpenCount\": " << counters.readerOpenCount << ",\n";
    stream << pad << "\"bytesReadCount\": " << counters.bytesReadCount << ",\n";
    stream << pad << "\"fullFrameReadCount\": " << counters.fullFrameReadCount << ",\n";
    stream << pad << "\"copyCount\": " << counters.copyCount << ",\n";
    stream << pad << "\"peakChunkReadCount\": " << counters.peakChunkReadCount;
}

void writeReportFile(const fs::path& outputPath, const std::string& reportJson)
{
    fs::create_directories(outputPath.parent_path());
    std::ofstream output(outputPath, std::ios::binary);
    require(output.good(), "Could not open the WAV baseline report output file for writing.");
    output << reportJson;
    require(output.good(), "Could not finish writing the WAV baseline report output file.");
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto constructorMetrics = measureTimedCounters([&]
        {
            drs::plugin::Processor processor;
            juce::ignoreUnused(processor);
        });

        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "WAV baseline report requires the Phase 2 reference project.");

        drs::plugin::Processor replaceProcessor;
        const auto projectReplaceMetrics = measureTimedCounters([&]
        {
            replaceProcessor.replaceAuthoringProject(projectLoad.project);
        });
        const auto projectReplaceSnapshot = replaceProcessor.getAuthoringImportResponsivenessSnapshot();

        const auto projectPath = drs::engine::getPhase2ReferenceProjectManifestPath();
        drs::plugin::Processor restoreSource;
        restoreSource.prepareToPlay(44100.0, 64);
        require(restoreSource.replaceAuthoringProject(projectLoad.project, juce::File(projectPath)),
                "WAV baseline report source processor must bind the Phase 2 reference project.");
        juce::MemoryBlock stateBlock;
        require(restoreSource.waitForHostStatePublication(),
                "WAV baseline report checkpoint did not reach background host-state publication.");
        restoreSource.getStateInformation(stateBlock);

        drs::plugin::Processor restoreTarget;
        restoreTarget.prepareToPlay(44100.0, 64);
        drs::engine::resetSampleImportIoCounters();
        const auto restoreStart = std::chrono::steady_clock::now();
        restoreTarget.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
        const auto restoreSnapshot = waitForRestore(restoreTarget);
        const auto restoreDuration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - restoreStart);
        const TimedCounters restoreMetrics {
            static_cast<std::uint64_t>(restoreDuration.count()),
            drs::engine::getSampleImportIoCounters()
        };

        const auto batchMetrics = measureSynchronousBatchImport(
            fs::temp_directory_path() / "drs-wav-import-baseline-report");

        std::ostringstream report;
        report << "{\n";
        report << "  \"report\": \"drs.wavImportBaseline\",\n";
        report << "  \"schemaVersion\": 1,\n";
        report << "  \"baselineId\": \"drs.wavImport.syncShellBaseline\",\n";
        report << "  \"capturedOn\": \"2026-07-31\",\n";
        report << "  \"timingUnits\": \"microseconds\",\n";

        report << "  \"constructor\": {\n";
        report << "    \"durationMicros\": " << constructorMetrics.durationMicros << ",\n";
        appendCounterJson(report, constructorMetrics.counters, 4);
        report << "\n  },\n";

        report << "  \"projectReplace\": {\n";
        report << "    \"durationMicros\": " << projectReplaceMetrics.durationMicros << ",\n";
        report << "    \"totalItemCount\": " << projectReplaceSnapshot.totalItemCount << ",\n";
        report << "    \"processedCount\": " << projectReplaceSnapshot.processedCount << ",\n";
        appendCounterJson(report, projectReplaceMetrics.counters, 4);
        report << "\n  },\n";

        report << "  \"restore\": {\n";
        report << "    \"durationMicros\": " << restoreMetrics.durationMicros << ",\n";
        report << "    \"message\": \"" << jsonEscape(restoreSnapshot->message) << "\",\n";
        appendCounterJson(report, restoreMetrics.counters, 4);
        report << "\n  },\n";

        report << "  \"importSubmit\": {\n";
        report << "    \"durationMicros\": " << batchMetrics.importSubmit.durationMicros << ",\n";
        report << "    \"completedInline\": " << (batchMetrics.completedInline ? "true" : "false") << ",\n";
        report << "    \"skippedCount\": " << batchMetrics.skippedCount << ",\n";
        report << "    \"totalItemCount\": " << batchMetrics.queueMetrics.totalItemCount << ",\n";
        report << "    \"processedCount\": " << batchMetrics.queueMetrics.processedCount << ",\n";
        report << "    \"warningItemCount\": " << batchMetrics.queueMetrics.warningItemCount << ",\n";
        report << "    \"failedItemCount\": " << batchMetrics.queueMetrics.failedItemCount << ",\n";
        report << "    \"canceledItemCount\": " << batchMetrics.queueMetrics.canceledItemCount << ",\n";
        report << "    \"acceptedItemCount\": " << batchMetrics.queueMetrics.acceptedItemCount << ",\n";
        appendCounterJson(report, batchMetrics.importSubmit.counters, 4);
        report << "\n  },\n";

        report << "  \"fullBatch\": {\n";
        report << "    \"durationMicros\": " << batchMetrics.fullBatch.durationMicros << ",\n";
        report << "    \"completedInline\": " << (batchMetrics.completedInline ? "true" : "false") << ",\n";
        appendCounterJson(report, batchMetrics.fullBatch.counters, 4);
        report << "\n  },\n";

        report << "  \"memoryShape\": {\n";
        report << "    \"largestDecodedSampleBytes\": " << batchMetrics.largestDecodedSampleBytes << ",\n";
        report << "    \"estimatedRetainedQueueBytes\": " << batchMetrics.retainedQueueBytes << ",\n";
        report << "    \"estimatedPeakWorkingBytes\": " << batchMetrics.estimatedPeakWorkingBytes << "\n";
        report << "  },\n";

        report << "  \"notes\": [\n";
        report << "    \"This historical baseline captured the plugin and standalone WAV shells while they still completed copy and queue processing inline before the submit callback returned.\",\n";
        report << "    \"Import submit and full-batch timings are therefore identical in this historical snapshot because the retired shell path drained the batch synchronously.\"\n";
        report << "  ]\n";
        report << "}\n";

        const auto reportJson = report.str();
        std::cout << reportJson;
        if (argc >= 2)
            writeReportFile(fs::path(argv[1]), reportJson);

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import baseline report failed: " << exception.what() << std::endl;
        return 1;
    }
}
