#include "shared/WavImportService.h"
#include "../support/WavImportTestSupport.h"

#include <algorithm>
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

struct RequestFixture
{
    fs::path root;
    drs::tests::GeneratedWavImportBatchCorpus corpus;
    drs::app::WavImportRequest request;
};

RequestFixture makeRequestFixture(const std::string& projectId)
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    RequestFixture fixture;
    fixture.root = fs::temp_directory_path() / ("drs-wav-import-lifecycle-" + projectId + "-" + unique);
    const auto sourceDirectory = fixture.root / "source";
    const auto contentRoot = fixture.root / "project";
    fs::create_directories(contentRoot);
    fixture.corpus = drs::tests::createGeneratedWavImportBatchCorpus(sourceDirectory);

    fixture.request.sourcePaths = {
        fixture.corpus.cleanPath.generic_string(),
        fixture.corpus.sparseOnePath.generic_string(),
    };
    fixture.request.projectId = projectId;
    fixture.request.baseRevision = 7;
    fixture.request.contentRootPath = contentRoot.generic_string();
    fixture.request.selectedGroupId = "drums";
    return fixture;
}

fs::path samplesDirectoryFor(const RequestFixture& fixture)
{
    return fs::path(fixture.request.contentRootPath) / "Samples";
}

fs::path stageDirectoryFor(const drs::app::WavImportRequestIdentity& identity)
{
    return fs::path(identity.contentRootPath) / "Samples" / ".staging"
        / ("wav-import-" + std::to_string(identity.ownerId) + "-" + std::to_string(identity.generation));
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

std::string describeImportIoCounters(const drs::engine::SampleImportIoCounters& counters)
{
    return "fingerprintOpenCount=" + std::to_string(counters.fingerprintOpenCount)
        + ", readerOpenCount=" + std::to_string(counters.readerOpenCount)
        + ", bytesReadCount=" + std::to_string(counters.bytesReadCount)
        + ", fullFrameReadCount=" + std::to_string(counters.fullFrameReadCount)
        + ", copyCount=" + std::to_string(counters.copyCount)
        + ", peakChunkReadCount=" + std::to_string(counters.peakChunkReadCount);
}

void requireNoImportIo(const std::string& context)
{
    const auto counters = drs::engine::getSampleImportIoCounters();
    require(counters.fingerprintOpenCount == 0
                && counters.readerOpenCount == 0
                && counters.bytesReadCount == 0
                && counters.fullFrameReadCount == 0
                && counters.copyCount == 0
                && counters.peakChunkReadCount == 0,
            context + " unexpectedly performed sample import IO inline: "
                + describeImportIoCounters(counters));
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        std::mutex submitMutex;
        std::condition_variable submitCondition;
        auto submitPausedAtStaging = false;
        auto releaseSubmitStaging = false;

        WavImportServiceOptions submitOptions;
        submitOptions.stageObserver = [&](const WavImportBatchStage stage)
        {
            if (stage != WavImportBatchStage::staging)
                return;

            std::unique_lock<std::mutex> lock(submitMutex);
            submitPausedAtStaging = true;
            submitCondition.notify_all();
            submitCondition.wait(lock, [&] { return releaseSubmitStaging; });
        };

        WavImportService submitService(std::move(submitOptions));
        auto submitClient = submitService.openClient();
        const auto submitFixture = makeRequestFixture("wav-submit-nonblocking-project");
        drs::engine::resetSampleImportIoCounters();
        const auto submitAccepted = submitClient.submit(submitFixture.request);
        require(submitAccepted.disposition == WavImportSubmitDisposition::accepted,
                "The paused-submit lifecycle request must be accepted.");
        {
            std::unique_lock<std::mutex> lock(submitMutex);
            require(submitCondition.wait_for(lock, 5s, [&] { return submitPausedAtStaging; }),
                    "The paused-submit lifecycle request must reach the staging checkpoint.");
        }

        const auto pausedSubmitSnapshot = submitClient.getSnapshot();
        require(pausedSubmitSnapshot != nullptr
                    && pausedSubmitSnapshot->stage == WavImportBatchStage::staging,
                "The paused-submit lifecycle request must remain in staging while the worker is blocked.");
        require(std::all_of(pausedSubmitSnapshot->items.begin(),
                            pausedSubmitSnapshot->items.end(),
                            [](const WavImportItemProgress& item)
                            {
                                return item.bytesProcessed == 0
                                    && item.fingerprintBytesProcessed == 0;
                            }),
                "Paused staging coverage must prove submit returned before copy or fingerprint progress advanced.");
        require(regularFileCount(samplesDirectoryFor(submitFixture)) == 0,
                "Paused staging coverage must prove submit returned before any staged files were written.");
        requireNoImportIo("Paused staging submit coverage");

        {
            std::lock_guard<std::mutex> lock(submitMutex);
            releaseSubmitStaging = true;
        }
        submitCondition.notify_all();
        require(submitClient.waitForTerminal(5s),
                "The paused-submit lifecycle request must still reach a terminal state after release.");

        std::mutex stagingMutex;
        std::condition_variable stagingCondition;
        auto stagingReached = false;
        auto releaseStaging = false;

        WavImportServiceOptions options;
        options.stageObserver = [&](const WavImportBatchStage stage)
        {
            if (stage != WavImportBatchStage::staging)
                return;

            std::unique_lock<std::mutex> lock(stagingMutex);
            stagingReached = true;
            stagingCondition.notify_all();
            stagingCondition.wait(lock, [&] { return releaseStaging; });
        };

        WavImportService service(std::move(options));
        auto client = service.openClient();
        const auto requestFixture = makeRequestFixture("wav-lifecycle-project");
        const auto request = requestFixture.request;

        const auto accepted = client.submit(request);
        require(accepted.disposition == WavImportSubmitDisposition::accepted,
                "The first WAV import request must be accepted.");
        const auto duplicate = client.submit(request);
        require(duplicate.disposition == WavImportSubmitDisposition::busy,
                "A duplicate WAV import request must be rejected as busy.");

        {
            std::unique_lock<std::mutex> lock(stagingMutex);
            if (!stagingCondition.wait_for(lock, 5s, [&] { return stagingReached; }))
            {
                const auto snapshot = client.getSnapshot();
                const auto status = snapshot == nullptr
                    ? "no snapshot"
                    : std::string(toString(snapshot->stage)) + " / " + snapshot->status;
                require(false, "The deterministic hook must pause at the staging checkpoint. Current snapshot: "
                                   + status);
            }
        }

        require(client.cancel("Lifecycle test cancellation"),
                "An active staging request must accept cancellation.");
        {
            std::lock_guard<std::mutex> lock(stagingMutex);
            releaseStaging = true;
        }
        stagingCondition.notify_all();

        require(client.waitForTerminal(5s),
                "Canceled staging work must reach a terminal state.");
        const auto canceled = client.getSnapshot();
        require(canceled && canceled->identity.generation == accepted.identity.generation
                    && canceled->stage == WavImportBatchStage::canceled
                    && canceled->terminalDisposition == WavImportTerminalDisposition::canceled,
                "The current generation must publish one typed canceled WAV snapshot.");
        require(!fs::exists(stageDirectoryFor(accepted.identity))
                    && regularFileCount(samplesDirectoryFor(requestFixture)) == 0,
                "Canceled WAV work must clean its private staging directory and leave no project files behind.");

        const auto canceledMetrics = service.getMetrics();
        require(canceledMetrics.requestedCount == 1
                    && canceledMetrics.rejectedBusyCount == 1
                    && canceledMetrics.canceledCount == 1
                    && canceledMetrics.maximumPendingCount <= 1
                    && canceledMetrics.maximumInFlightCount == 1
                    && canceledMetrics.lastTerminalGeneration == accepted.identity.generation,
                "Lifecycle metrics must prove bounded scheduling and cancellation.");

        service.shutdown();
        service.shutdown();
        require(service.getMetrics().liveWorkerCount == 0,
                "Idempotent shutdown must leave no live WAV worker.");

        WavImportService completionService;
        auto completionClient = completionService.openClient();
        const auto completionFixture = makeRequestFixture("wav-completion-project");
        const auto completionAccepted = completionClient.submit(completionFixture.request);
        require(completionAccepted.disposition == WavImportSubmitDisposition::accepted,
                "The completion lifecycle request must be accepted.");
        require(completionClient.waitForTerminal(5s),
                "A normal WAV lifecycle request must reach completion.");

        const auto completed = completionClient.getSnapshot();
        require(completed && completed->stage == WavImportBatchStage::completed
                    && completed->terminalDisposition == WavImportTerminalDisposition::completed
                    && completed->completion != nullptr
                    && completed->completion->identity.projectId == "wav-completion-project"
                    && completed->completion->identity.selectedGroupId == "drums"
                    && completed->completion->items.size() == 2
                    && completed->completion->items.front().fingerprint.fingerprinted
                    && completed->completion->items.front().inspection.accepted,
                "Completed WAV requests must publish an immutable completion payload.");
        require(fs::exists(stageDirectoryFor(completionAccepted.identity)),
                "Completed WAV requests must retain staged files until the result is explicitly consumed.");
        require(completionClient.consume(),
                "Completed WAV requests must allow explicit result consumption.");
        const auto consumed = completionClient.getSnapshot();
        require(consumed && consumed->stage == WavImportBatchStage::consumed
                    && consumed->terminalDisposition == WavImportTerminalDisposition::consumed,
                "Consumed WAV requests must publish a consumed terminal snapshot.");
        require(!fs::exists(stageDirectoryFor(completionAccepted.identity))
                    && regularFileCount(samplesDirectoryFor(completionFixture)) == 0,
                "Consuming a completed WAV result must clean stale staged artifacts without mutating the project.");

        std::mutex supersedeMutex;
        std::condition_variable supersedeCondition;
        auto supersedeReached = false;
        auto releaseSupersede = false;
        WavImportServiceOptions supersedeOptions;
        supersedeOptions.stageObserver = [&](const WavImportBatchStage stage)
        {
            if (stage != WavImportBatchStage::staging)
                return;

            std::unique_lock<std::mutex> lock(supersedeMutex);
            supersedeReached = true;
            supersedeCondition.notify_all();
            supersedeCondition.wait(lock, [&] { return releaseSupersede; });
        };

        WavImportService supersedeService(std::move(supersedeOptions));
        auto supersedeClient = supersedeService.openClient();
        const auto supersedeFixture = makeRequestFixture("wav-supersede-project");
        const auto supersedeAccepted = supersedeClient.submit(supersedeFixture.request);
        require(supersedeAccepted.disposition == WavImportSubmitDisposition::accepted,
                "The supersede lifecycle request must be accepted.");
        {
            std::unique_lock<std::mutex> lock(supersedeMutex);
            require(supersedeCondition.wait_for(lock, 5s, [&] { return supersedeReached; }),
                    "The supersede lifecycle request must reach the staging checkpoint.");
        }
        require(supersedeClient.cancel("WAV import superseded by newer request"),
                "An in-flight WAV request must accept a supersede cancellation reason.");
        {
            std::lock_guard<std::mutex> lock(supersedeMutex);
            releaseSupersede = true;
        }
        supersedeCondition.notify_all();
        require(supersedeClient.waitForTerminal(5s),
                "Superseded WAV work must reach a terminal state.");
        const auto superseded = supersedeClient.getSnapshot();
        require(superseded && superseded->stage == WavImportBatchStage::superseded
                    && superseded->terminalDisposition == WavImportTerminalDisposition::superseded
                    && !fs::exists(stageDirectoryFor(supersedeAccepted.identity))
                    && regularFileCount(samplesDirectoryFor(supersedeFixture)) == 0,
                "Superseded WAV work must publish a distinct terminal disposition and leave no artifacts behind.");

        const auto copyFailureFixture = makeRequestFixture("wav-copy-failure-project");
        drs::app::WavImportRequest copyFailureRequest = copyFailureFixture.request;
        copyFailureRequest.sourcePaths = { copyFailureRequest.sourcePaths.front() };
        std::mutex copyFailureMutex;
        std::condition_variable copyFailureCondition;
        auto copyFailureReached = false;
        auto releaseCopyFailure = false;
        WavImportServiceOptions copyFailureOptions;
        copyFailureOptions.stageObserver = [&](const WavImportBatchStage stage)
        {
            if (stage != WavImportBatchStage::staging)
                return;

            std::unique_lock<std::mutex> lock(copyFailureMutex);
            copyFailureReached = true;
            copyFailureCondition.notify_all();
            copyFailureCondition.wait(lock, [&] { return releaseCopyFailure; });
        };

        WavImportService copyFailureService(std::move(copyFailureOptions));
        auto copyFailureClient = copyFailureService.openClient();
        const auto copyFailureAccepted = copyFailureClient.submit(copyFailureRequest);
        require(copyFailureAccepted.disposition == WavImportSubmitDisposition::accepted,
                "The copy-failure lifecycle request must be accepted.");
        {
            std::unique_lock<std::mutex> lock(copyFailureMutex);
            require(copyFailureCondition.wait_for(lock, 5s, [&] { return copyFailureReached; }),
                    "The copy-failure request must reach the staging checkpoint.");
        }
        const auto copyFailureSnapshot = copyFailureClient.getSnapshot();
        require(copyFailureSnapshot != nullptr && copyFailureSnapshot->items.size() == 1,
                "The copy-failure request must publish one staged item before the injected failure.");
        fs::create_directories(fs::path(copyFailureSnapshot->items.front().stagedPath));
        {
            std::lock_guard<std::mutex> lock(copyFailureMutex);
            releaseCopyFailure = true;
        }
        copyFailureCondition.notify_all();
        require(copyFailureClient.waitForTerminal(5s),
                "Injected copy failure must reach a terminal state.");
        const auto copyFailed = copyFailureClient.getSnapshot();
        require(copyFailed && copyFailed->stage == WavImportBatchStage::failed
                    && copyFailed->terminalDisposition == WavImportTerminalDisposition::failed
                    && !fs::exists(stageDirectoryFor(copyFailureAccepted.identity))
                    && regularFileCount(samplesDirectoryFor(copyFailureFixture)) == 0,
                "Copy failures must clean the request staging directory and leave no committed files behind.");

        const auto inspectionFailureFixture = makeRequestFixture("wav-inspection-failure-project");
        drs::app::WavImportRequest inspectionFailureRequest = inspectionFailureFixture.request;
        inspectionFailureRequest.sourcePaths = {
            inspectionFailureFixture.corpus.unsupportedPath.generic_string()
        };
        WavImportService inspectionFailureService;
        auto inspectionFailureClient = inspectionFailureService.openClient();
        const auto inspectionFailureAccepted = inspectionFailureClient.submit(inspectionFailureRequest);
        require(inspectionFailureAccepted.disposition == WavImportSubmitDisposition::accepted,
                "The inspection-failure lifecycle request must be accepted.");
        require(inspectionFailureClient.waitForTerminal(5s),
                "Inspection failure coverage must reach a terminal state.");
        const auto inspectionFailed = inspectionFailureClient.getSnapshot();
        require(inspectionFailed && inspectionFailed->stage == WavImportBatchStage::failed
                    && inspectionFailed->terminalDisposition == WavImportTerminalDisposition::failed
                    && !fs::exists(stageDirectoryFor(inspectionFailureAccepted.identity))
                    && regularFileCount(samplesDirectoryFor(inspectionFailureFixture)) == 0,
                "Inspection failures must clean staged artifacts and leave the project untouched.");

        std::mutex ownerMutex;
        std::condition_variable ownerCondition;
        auto inspectingReached = false;
        auto releaseInspecting = false;
        auto ownerClosed = false;
        WavImportServiceOptions ownerOptions;
        ownerOptions.stageObserver = [&](const WavImportBatchStage stage)
        {
            if (stage != WavImportBatchStage::inspecting)
                return;

            std::unique_lock<std::mutex> lock(ownerMutex);
            inspectingReached = true;
            ownerCondition.notify_all();
            ownerCondition.wait(lock, [&] { return releaseInspecting; });
        };

        WavImportService ownerService(std::move(ownerOptions));
        WavImportSubmitResult ownerAccepted;
        const auto ownerFixture = makeRequestFixture("wav-owner-close-project");
        std::thread ownerThread([&]
        {
            {
                auto ownerClient = ownerService.openClient();
                ownerAccepted = ownerClient.submit(ownerFixture.request);
                require(ownerAccepted.disposition == WavImportSubmitDisposition::accepted,
                        "The owner-close lifecycle request must be accepted.");
                {
                    std::unique_lock<std::mutex> lock(ownerMutex);
                    if (!ownerCondition.wait_for(lock, 5s, [&] { return inspectingReached; }))
                    {
                        const auto snapshot = ownerService.getSnapshot(ownerAccepted.identity.ownerId,
                                                                       ownerAccepted.identity.generation);
                        const auto status = snapshot == nullptr
                            ? "no snapshot"
                            : std::string(toString(snapshot->stage)) + " / " + snapshot->status;
                        require(false, "The deterministic hook must pause at the inspecting checkpoint. Current snapshot: "
                                           + status);
                    }
                }
            }

            std::lock_guard<std::mutex> lock(ownerMutex);
            ownerClosed = true;
            ownerCondition.notify_all();
        });

        {
            std::unique_lock<std::mutex> lock(ownerMutex);
            if (!ownerCondition.wait_for(lock, 5s, [&] { return inspectingReached; }))
            {
                const auto snapshot = ownerService.getSnapshot(ownerAccepted.identity.ownerId,
                                                               ownerAccepted.identity.generation);
                const auto status = snapshot == nullptr
                    ? "no snapshot"
                    : std::string(toString(snapshot->stage)) + " / " + snapshot->status;
                require(false, "The owner-close request must reach the inspecting checkpoint. Current snapshot: "
                                   + status);
            }
            require(!ownerClosed,
                    "Dropping the client while the worker is paused must wait for terminal publication.");
            releaseInspecting = true;
        }
        ownerCondition.notify_all();
        ownerThread.join();

        const auto ownerSnapshot = ownerService.getSnapshot(ownerAccepted.identity.ownerId,
                                                            ownerAccepted.identity.generation);
        require(ownerSnapshot && ownerSnapshot->stage == WavImportBatchStage::canceled,
                "Client teardown must cancel the owned request and publish a terminal state.");
        require(!fs::exists(stageDirectoryFor(ownerAccepted.identity))
                    && regularFileCount(samplesDirectoryFor(ownerFixture)) == 0,
                "Client teardown cancellation must also clean the owned request staging artifacts.");
        ownerService.shutdown();
        require(ownerService.getMetrics().liveWorkerCount == 0,
                "The owned WAV worker must not survive service shutdown.");

        const auto shutdownFixture = makeRequestFixture("wav-shutdown-project");
        std::mutex shutdownMutex;
        std::condition_variable shutdownCondition;
        auto shutdownReached = false;
        auto releaseShutdown = false;
        WavImportServiceOptions shutdownOptions;
        shutdownOptions.stageObserver = [&](const WavImportBatchStage stage)
        {
            if (stage != WavImportBatchStage::staging)
                return;

            std::unique_lock<std::mutex> lock(shutdownMutex);
            shutdownReached = true;
            shutdownCondition.notify_all();
            shutdownCondition.wait(lock, [&] { return releaseShutdown; });
        };

        WavImportService shutdownService(std::move(shutdownOptions));
        auto shutdownClient = shutdownService.openClient();
        const auto shutdownAccepted = shutdownClient.submit(shutdownFixture.request);
        require(shutdownAccepted.disposition == WavImportSubmitDisposition::accepted,
                "The shutdown lifecycle request must be accepted.");
        {
            std::unique_lock<std::mutex> lock(shutdownMutex);
            require(shutdownCondition.wait_for(lock, 5s, [&] { return shutdownReached; }),
                    "The shutdown coverage request must reach the staging checkpoint.");
        }
        std::thread shutdownThread([&] { shutdownService.shutdown(); });
        {
            std::lock_guard<std::mutex> lock(shutdownMutex);
            releaseShutdown = true;
        }
        shutdownCondition.notify_all();
        shutdownThread.join();
        require(shutdownService.getMetrics().liveWorkerCount == 0
                    && !fs::exists(stageDirectoryFor(shutdownAccepted.identity))
                    && regularFileCount(samplesDirectoryFor(shutdownFixture)) == 0,
                "Processor shutdown must cancel the active WAV batch, join the worker, and clean staged artifacts.");

        std::cout << "WAV import lifecycle tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import lifecycle tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
