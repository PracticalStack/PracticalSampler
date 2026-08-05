#include "Phase1PerformancePackageSupport.h"
#include "shared/PerformancePackageExportService.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::app::PerformancePackageExportRequest makeRequest(const fs::path& outputPackagePath)
{
    drs::app::PerformancePackageExportRequest request;
    request.project = drs::tests::performance_package::buildAuthoringProjectFixture();
    request.sessionState.loadProfileId = "balanced";
    request.projectId = request.project.projectId;
    request.baseRevision = 1;
    request.packagePath = outputPackagePath.generic_string();
    return request;
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        using Stage = PerformancePackageExportStage;
        require(isPerformancePackageExportStageTransitionAllowed(Stage::idle, Stage::queued)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::queued, Stage::validating)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::validating, Stage::compiling)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::compiling, Stage::writingStream)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::writingStream, Stage::sealingPackage)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::sealingPackage, Stage::verifying)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::verifying, Stage::completed)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::completed, Stage::consumed),
                "The owned playable-package export lifecycle must remain executable.");
        require(isPerformancePackageExportStageTransitionAllowed(Stage::queued, Stage::canceled)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::writingStream, Stage::canceled)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::sealingPackage, Stage::failed),
                "Queued and in-flight export work must retain terminal exits.");
        require(!isPerformancePackageExportStageTransitionAllowed(Stage::idle, Stage::completed)
                    && !isPerformancePackageExportStageTransitionAllowed(Stage::queued, Stage::sealingPackage)
                    && !isPerformancePackageExportStageTransitionAllowed(Stage::failed, Stage::completed),
                "Invalid export lifecycle shortcuts must be rejected.");

        const auto tempRoot = fs::temp_directory_path() / "drs-performance-package-export-lifecycle";
        fs::remove_all(tempRoot);
        fs::create_directories(tempRoot);

        std::mutex gateMutex;
        std::condition_variable gateCondition;
        auto streamReached = false;
        auto releaseStream = false;

        PerformancePackageExportServiceOptions options;
        options.stageObserver = [&](const Stage stage)
        {
            if (stage != Stage::writingStream)
                return;

            std::unique_lock<std::mutex> lock(gateMutex);
            streamReached = true;
            gateCondition.notify_all();
            gateCondition.wait(lock, [&] { return releaseStream; });
        };

        PerformancePackageExportService service(std::move(options));
        auto client = service.openClient();
        auto request = makeRequest(tempRoot / "cancel-me.drpkg");

        const auto accepted = client.submit(request);
        require(accepted.disposition == PerformancePackageExportSubmitDisposition::accepted,
                "The first export request must be accepted.");
        const auto duplicate = client.submit(request);
        require(duplicate.disposition == PerformancePackageExportSubmitDisposition::busy,
                "A duplicate export request must be rejected as busy.");

        {
            std::unique_lock<std::mutex> lock(gateMutex);
            require(gateCondition.wait_for(lock, 20s, [&] { return streamReached; }),
                    "The deterministic hook must pause at the stream-writing checkpoint.");
        }

        require(client.cancel("Lifecycle test cancellation"),
                "An active export request must accept cancellation.");
        {
            std::lock_guard<std::mutex> lock(gateMutex);
            releaseStream = true;
        }
        gateCondition.notify_all();

        require(client.waitForTerminal(20s),
                "Canceled export must reach a terminal state.");
        const auto canceled = client.getSnapshot();
        require(canceled && canceled->identity.generation == accepted.identity.generation
                    && canceled->stage == Stage::canceled,
                "The current generation must publish one typed canceled export snapshot.");

        const auto metrics = service.getMetrics();
        require(metrics.requestedCount == 1
                    && metrics.rejectedBusyCount == 1
                    && metrics.canceledCount == 1
                    && metrics.maximumPendingCount <= 1
                    && metrics.maximumInFlightCount == 1,
                "Lifecycle metrics must prove bounded export scheduling and cancellation.");

        service.shutdown();
        service.shutdown();
        require(service.getMetrics().liveWorkerCount == 0,
                "Idempotent export shutdown must leave no live worker.");

        PerformancePackageExportService completionService;
        auto completionClient = completionService.openClient();
        const auto completedAccepted = completionClient.submit(makeRequest(tempRoot / "completed.drpkg"));
        require(completedAccepted.disposition == PerformancePackageExportSubmitDisposition::accepted,
                "The completion export request must be accepted.");
        require(completionClient.waitForTerminal(60s),
                "A valid export request must reach a terminal state.");

        const auto completed = completionClient.getSnapshot();
        require(completed && completed->stage == Stage::completed && completed->result != nullptr
                    && completed->result->exported,
                "A successful export must publish a completed snapshot with an exported result.");
        require(fs::exists(tempRoot / "completed.drpkg"),
                "The completed export should materialize the playable package on disk.");
        require(completed->result->packageBytes > 0 && completed->result->payloadCount > 0,
                "The completed export should report non-empty package metrics.");
        completionService.shutdown();

        fs::remove_all(tempRoot);

        std::cout << "Playable package export lifecycle tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Playable package export lifecycle tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
