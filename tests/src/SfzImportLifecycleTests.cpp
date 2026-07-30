#include "shared/SfzImportReviewService.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
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

fs::path resolveFixturePath()
{
    const auto relativeFixture = fs::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr")
        / "jRhodes3d-mono/_jRhodes3d-mono-flac.sfz";
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);
    const auto fixture = sourceRoot / relativeFixture;
    if (fs::exists(fixture))
        return fixture;

    const auto workspaceFixture = sourceRoot.parent_path() / relativeFixture;
    require(fs::exists(workspaceFixture), "SFZ lifecycle tests require the checked-in reference fixture.");
    return workspaceFixture;
}

drs::engine::RuntimeProjectModel makeBaseProject(const fs::path& fixture)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.runtimeProject";
    project.schemaVersion = 2;
    project.projectId = "sfz-lifecycle-project";
    project.displayName = "SFZ lifecycle project";
    project.contentRootPath = fixture.parent_path().generic_string();
    project.authoring.schemaName = "drs.authoringState";
    project.authoring.schemaVersion = 2;
    return project;
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        using Stage = SfzImportReviewServiceStage;
        require(isSfzImportReviewServiceStageTransitionAllowed(Stage::idle, Stage::queued)
                    && isSfzImportReviewServiceStageTransitionAllowed(Stage::queued, Stage::analyzing)
                    && isSfzImportReviewServiceStageTransitionAllowed(Stage::analyzing, Stage::projecting)
                    && isSfzImportReviewServiceStageTransitionAllowed(Stage::projecting, Stage::reviewReady)
                    && isSfzImportReviewServiceStageTransitionAllowed(Stage::reviewReady, Stage::consumed),
                "The ordinary owned SFZ review lifecycle must remain executable.");
        require(isSfzImportReviewServiceStageTransitionAllowed(Stage::queued, Stage::canceled)
                    && isSfzImportReviewServiceStageTransitionAllowed(Stage::analyzing, Stage::canceled)
                    && isSfzImportReviewServiceStageTransitionAllowed(Stage::projecting, Stage::failed),
                "Queued and in-flight review work must have terminal exits.");
        require(!isSfzImportReviewServiceStageTransitionAllowed(Stage::idle, Stage::reviewReady)
                    && !isSfzImportReviewServiceStageTransitionAllowed(Stage::queued, Stage::projecting)
                    && !isSfzImportReviewServiceStageTransitionAllowed(Stage::canceled, Stage::reviewReady),
                "Invalid review lifecycle shortcuts must be rejected.");

        const auto fixture = resolveFixturePath();
        const auto project = makeBaseProject(fixture);

        std::mutex gateMutex;
        std::condition_variable gateCondition;
        auto analyzingReached = false;
        auto releaseAnalyzing = false;

        SfzImportReviewServiceOptions options;
        options.stageObserver = [&](const Stage stage)
        {
            if (stage != Stage::analyzing)
                return;

            std::unique_lock<std::mutex> lock(gateMutex);
            analyzingReached = true;
            gateCondition.notify_all();
            gateCondition.wait(lock, [&] { return releaseAnalyzing; });
        };

        SfzImportReviewService service(std::move(options));
        auto client = service.openClient();

        SfzImportReviewRequest request;
        request.baseProject = project;
        request.sfzPath = fixture.generic_string();
        request.projectId = project.projectId;
        request.baseRevision = 7;

        const auto accepted = client.submit(request);
        require(accepted.disposition == SfzImportReviewSubmitDisposition::accepted,
                "The first review request must be accepted.");
        const auto duplicate = client.submit(request);
        require(duplicate.disposition == SfzImportReviewSubmitDisposition::busy,
                "A duplicate review request must be rejected as busy.");

        {
            std::unique_lock<std::mutex> lock(gateMutex);
            require(gateCondition.wait_for(lock, 5s, [&] { return analyzingReached; }),
                    "The deterministic hook must pause at the analyzing checkpoint.");
        }

        require(client.cancel("Lifecycle test cancellation"),
                "An active analyzing request must accept cancellation.");
        {
            std::lock_guard<std::mutex> lock(gateMutex);
            releaseAnalyzing = true;
        }
        gateCondition.notify_all();

        require(client.waitForTerminal(5s),
                "Canceled analysis must reach a terminal state.");
        const auto canceled = client.getSnapshot();
        require(canceled && canceled->identity.generation == accepted.identity.generation
                    && canceled->stage == Stage::canceled,
                "The current generation must publish one typed canceled snapshot.");

        const auto metrics = service.getMetrics();
        require(metrics.requestedCount == 1
                    && metrics.rejectedBusyCount == 1
                    && metrics.canceledCount == 1
                    && metrics.maximumPendingCount <= 1
                    && metrics.maximumInFlightCount == 1,
                "Lifecycle metrics must prove bounded scheduling and cancellation.");

        service.shutdown();
        service.shutdown();
        require(service.getMetrics().liveWorkerCount == 0,
                "Idempotent shutdown must leave no live worker.");

        // A second service proves that processor-style shutdown also joins a
        // worker paused in the projection stage, rather than abandoning it.
        std::mutex projectionMutex;
        std::condition_variable projectionCondition;
        auto projectionReached = false;
        auto releaseProjection = false;
        SfzImportReviewServiceOptions projectionOptions;
        projectionOptions.stageObserver = [&](const Stage stage)
        {
            if (stage != Stage::projecting)
                return;
            std::unique_lock<std::mutex> lock(projectionMutex);
            projectionReached = true;
            projectionCondition.notify_all();
            projectionCondition.wait(lock, [&] { return releaseProjection; });
        };

        SfzImportReviewService projectionService(std::move(projectionOptions));
        auto projectionClient = projectionService.openClient();
        const auto projectionAccepted = projectionClient.submit(request);
        require(projectionAccepted.disposition == SfzImportReviewSubmitDisposition::accepted,
                "The projection lifecycle request must be accepted.");
        {
            std::unique_lock<std::mutex> lock(projectionMutex);
            require(projectionCondition.wait_for(lock, 5s, [&] { return projectionReached; }),
                    "The deterministic hook must pause at the projection checkpoint.");
        }

        auto shutdownFinished = false;
        std::thread shutdownThread([&]
        {
            projectionService.shutdown();
            std::lock_guard<std::mutex> lock(projectionMutex);
            shutdownFinished = true;
            projectionCondition.notify_all();
        });
        {
            std::lock_guard<std::mutex> lock(projectionMutex);
            require(!shutdownFinished,
                    "Shutdown must wait for the paused projection worker to reach a terminal state.");
            releaseProjection = true;
        }
        projectionCondition.notify_all();
        shutdownThread.join();
        require(projectionService.getMetrics().liveWorkerCount == 0,
                "Projection shutdown must join the owned worker.");
        const auto projectionCanceled = projectionClient.getSnapshot();
        require(projectionCanceled && projectionCanceled->stage == Stage::canceled,
                "Processor-style shutdown must publish a canceled projection terminal state.");

        // Repeated owner lifetimes exercise the same barrier used by editor and
        // standalone close paths without relying on timing-sensitive UI code.
        for (int cycle = 0; cycle < 100; ++cycle)
        {
            SfzImportReviewService repeatedService;
            auto repeatedClient = repeatedService.openClient();
            auto repeatedRequest = request;
            repeatedRequest.sfzPath = (fixture.parent_path() / "missing-cycle.sfz").generic_string();
            repeatedRequest.projectId = "cycle-" + std::to_string(cycle);
            const auto repeatedAccepted = repeatedClient.submit(std::move(repeatedRequest));
            require(repeatedAccepted.disposition == SfzImportReviewSubmitDisposition::accepted,
                    "Repeated lifecycle request must be accepted.");
            require(repeatedClient.waitForTerminal(5s), "Repeated lifecycle request must terminate.");
            require(repeatedClient.getSnapshot() != nullptr, "Repeated lifecycle must publish a snapshot.");
            repeatedService.shutdown();
        }

        std::cout << "SFZ import lifecycle contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "SFZ import lifecycle contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
