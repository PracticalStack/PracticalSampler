#include "plugin/PluginProcessor.h"
#include "../support/WavImportTestSupport.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <filesystem>
#include <iostream>
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

fs::path makeScratchDirectory()
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto path = fs::temp_directory_path() / ("drs-wav-import-processor-responsiveness-" + unique);
    fs::create_directories(path);
    return path;
}

drs::engine::RuntimeProjectModel makeProject(const fs::path& root)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 5;
    project.projectId = "wav-responsiveness-project";
    project.displayName = "WAV Responsiveness Project";
    project.contentRootPath = root.generic_string();
    project.defaultInstrumentManifestPath = (root / "wav-responsiveness-project.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 4;
    project.authoring.notes = { "Responsiveness test project" };
    return project;
}

bool waitForState(drs::plugin::Processor& processor,
                  const std::string& expectedState,
                  const std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto snapshot = processor.getAuthoringImportResponsivenessSnapshot();
        if (snapshot.state == expectedState)
            return true;
        std::this_thread::sleep_for(2ms);
    }
    return false;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto scratch = makeScratchDirectory();
        const auto corpus = drs::tests::createGeneratedWavImportBatchCorpus(scratch / "corpus");
        const auto projectRoot = scratch / "project";
        fs::create_directories(projectRoot / "Samples");

        drs::plugin::Processor processor;
        require(processor.replaceAuthoringProject(makeProject(projectRoot)),
                "Responsiveness test processor must accept the temporary project.");

        const auto baseline = processor.getAuthoringImportResponsivenessSnapshot();
        require(baseline.available
                    && baseline.state == "idle"
                    && baseline.totalItemCount == 0
                    && baseline.processedCount == 0,
                "A temporary project with no configured sample sources should publish an idle baseline.");

        auto activeClient = processor.getWavImportService().openClient();
        drs::app::WavImportRequest activeRequest;
        activeRequest.projectId = "wav-responsiveness-project";
        activeRequest.contentRootPath = projectRoot.generic_string();
        activeRequest.sourcePaths.assign(4, corpus.cleanPath.generic_string());
        const auto activeSubmit = activeClient.submit(activeRequest);
        require(activeSubmit.wasAccepted(),
                "The active responsiveness request must be accepted.");
        require(waitForState(processor, "active", 2s),
                "The processor responsiveness snapshot should surface an active state for queued or in-flight WAV imports.");
        require(activeClient.waitForTerminal(10s),
                "The active responsiveness request must reach a terminal state.");
        const auto completed = processor.getAuthoringImportResponsivenessSnapshot();
        require((completed.state == "completed" || completed.state == "completed-partial")
                    && completed.totalItemCount == 4
                    && completed.processedCount == 4
                    && completed.acceptedItemCount == 4
                    && completed.pendingCount == 0,
                "Completed WAV responsiveness metrics should reflect the finished batch counts.");
        require(activeClient.consume(),
                "The completed responsiveness request should be consumable.");
        const auto consumed = processor.getAuthoringImportResponsivenessSnapshot();
        require(consumed.state == completed.state
                    && consumed.processedCount == completed.processedCount
                    && consumed.acceptedItemCount == completed.acceptedItemCount,
                "Consumed WAV responsiveness snapshots should preserve the latest completed batch metrics.");

        auto failedClient = processor.getWavImportService().openClient();
        drs::app::WavImportRequest failedRequest;
        failedRequest.projectId = "wav-responsiveness-project";
        failedRequest.contentRootPath = projectRoot.generic_string();
        failedRequest.sourcePaths = { (scratch / "definitely-missing.wav").generic_string() };
        const auto failedSubmit = failedClient.submit(failedRequest);
        require(failedSubmit.wasAccepted(),
                "The failed responsiveness request must still be accepted for processing.");
        require(failedClient.waitForTerminal(10s),
                "The failed responsiveness request must reach a terminal state.");
        const auto failed = processor.getAuthoringImportResponsivenessSnapshot();
        require(failed.state == "failed",
                "The processor responsiveness snapshot should surface a failed state for failed WAV batches.");

        auto canceledClient = processor.getWavImportService().openClient();
        drs::app::WavImportRequest canceledRequest;
        canceledRequest.projectId = "wav-responsiveness-project";
        canceledRequest.contentRootPath = projectRoot.generic_string();
        canceledRequest.sourcePaths.assign(8, corpus.cleanPath.generic_string());
        const auto canceledSubmit = canceledClient.submit(canceledRequest);
        require(canceledSubmit.wasAccepted(),
                "The canceled responsiveness request must be accepted.");
        require(canceledClient.cancel("Canceled by responsiveness test"),
                "The canceled responsiveness request must accept a cancellation signal.");
        require(canceledClient.waitForTerminal(10s),
                "The canceled responsiveness request must reach a terminal state.");
        const auto canceled = processor.getAuthoringImportResponsivenessSnapshot();
        require(canceled.state == "canceled",
                "The processor responsiveness snapshot should surface a canceled state for canceled WAV batches.");

        std::cout << "WAV import processor responsiveness tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import processor responsiveness tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
