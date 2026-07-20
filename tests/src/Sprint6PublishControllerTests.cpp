#include "drs/engine/PerformancePublishController.h"

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::PerformancePublishResult eligibleResult(
    const drs::engine::PerformancePublishRequestIdentity& identity,
    std::uint64_t preparedBuildId)
{
    drs::engine::PerformancePublishResult result;
    result.identity = identity;
    result.completeProject = true;
    result.activationEligible = true;
    result.preparedBuildId = preparedBuildId;
    result.preparedContentDigest = "prepared:" + std::to_string(preparedBuildId);
    result.preparedMacroSchemaDigest = identity.macroSchemaDigest;
    return result;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        PerformancePublishController controller({ 3 });
        const auto first = controller.request(4, 17, "authored:17", "macros:a", 100);
        require(first.accepted && first.request.identity.requestId != 0
                    && first.request.identity.projectGeneration == 4
                    && first.request.identity.draftRevision == 17,
                "The controller must capture one complete typed Publish request identity.");
        require(controller.getSnapshot().preparationState == PerformancePublishPreparationState::queued,
                "An accepted Publish request must enter Queued.");

        const auto duplicate = controller.request(4, 17, "authored:17", "macros:a", 110);
        require(!duplicate.accepted && duplicate.duplicateSuppressed
                    && duplicate.request.identity == first.request.identity
                    && controller.getSnapshot().requestedCount == 1,
                "An exact live Publish duplicate must collapse without creating identity or work.");

        require(controller.markPreparing(first.request.identity, 120),
                "Queued Publish work must transition to Preparing.");
        const auto second = controller.request(4, 18, "authored:18", "macros:b", 130);
        require(second.accepted && second.supersededPrevious && second.cancellationRequested
                    && second.request.identity.requestId > first.request.identity.requestId
                    && second.request.identity.cancellationGeneration
                        > first.request.identity.cancellationGeneration,
                "A different explicit Publish must supersede and invalidate older preparing work.");

        const auto stale = eligibleResult(first.request.identity, 701);
        require(!controller.acceptPrepared(stale, 150),
                "An older completion must never become Ready.");
        require(controller.markPreparing(second.request.identity, 160),
                "The newest request must launch independently of the stale completion.");
        const auto current = eligibleResult(second.request.identity, 702);
        require(controller.acceptPrepared(current, 220)
                    && controller.markActivationPending(second.request.identity, 230)
                    && controller.markActive(second.request.identity, 250),
                "The exact newest eligible result must progress through Ready, Pending, and Active.");

        const auto active = controller.getSnapshot();
        require(active.hasActiveRequest
                    && active.activeRequestIdentity == second.request.identity
                    && active.activePreparedBuildId == 702
                    && active.activePreparedDigest == "prepared:702"
                    && active.activeMacroSchemaDigest == "macros:b",
                "The immutable snapshot must retain the exact active identity and prepared truth.");

        const auto third = controller.request(4, 19, "authored:19", "macros:c", 300);
        require(third.accepted && controller.markPreparing(third.request.identity, 310),
                "A newer Publish must queue while last-known-good remains independent.");
        require(controller.fail(third.request.identity,
                                { PerformancePublishFindingSeverity::error,
                                  "missing-source", "sampleSources/kick", "Source is unavailable." }),
                "The current request must accept a structured failure.");
        const auto failed = controller.getSnapshot();
        require(failed.preparationState == PerformancePublishPreparationState::failed
                    && failed.hasFailedRequest
                    && failed.failedRequestIdentity == third.request.identity
                    && failed.hasActiveRequest
                    && failed.activeRequestIdentity == second.request.identity
                    && failed.activePreparedBuildId == 702,
                "Failure must remain independent from exact last-known-good identity.");

        const auto retry = controller.request(4, 19, "authored:19", "macros:c", 320);
        require(retry.accepted && retry.request.identity.requestId > third.request.identity.requestId,
                "An explicit retry of a failed captured input must create a new request.");
        require(controller.cancelCurrent()
                    && controller.getSnapshot().preparationState
                        == PerformancePublishPreparationState::canceled,
                "The controller must expose a terminal cancellation path.");
        require(controller.getCompletionRecords().size() <= 3
                    && controller.getSnapshot().retainedCompletionRecordCount <= 3
                    && controller.getSnapshot().maximumPendingDepth == 1,
                "Completion history and pending identity must remain bounded by configuration.");

        PerformancePublishController concurrentController({ 8 });
        std::atomic<bool> stopReader { false };
        std::atomic<bool> coherent { true };
        std::atomic<std::size_t> readCount { 0 };
        std::thread reader([&]
        {
            while (!stopReader.load(std::memory_order_acquire))
            {
                const auto snapshot = concurrentController.getSnapshot();
                if ((snapshot.hasRequest
                     && (snapshot.currentRequest.identity.requestId == 0
                         || snapshot.currentRequest.identity.authoredContentDigest.empty()
                         || snapshot.currentRequest.identity.macroSchemaDigest.empty()))
                    || (snapshot.hasActiveRequest
                        && (snapshot.activePreparedBuildId == 0
                            || snapshot.activePreparedDigest.empty()
                            || snapshot.activeMacroSchemaDigest.empty()))
                    || (snapshot.hasFailedRequest && snapshot.failureFinding.code.empty())
                    || snapshot.pendingDepth > 1)
                {
                    coherent.store(false, std::memory_order_release);
                    break;
                }
                readCount.fetch_add(1, std::memory_order_relaxed);
            }
        });

        for (std::uint64_t revision = 1; revision <= 40; ++revision)
        {
            const auto request = concurrentController.request(
                9, static_cast<std::size_t>(revision),
                "authored:" + std::to_string(revision),
                "macros:" + std::to_string(revision), revision * 100);
            require(request.accepted
                        && concurrentController.markPreparing(request.request.identity,
                                                              revision * 100 + 10),
                    "Concurrent snapshot exercise must launch each explicit request.");
            const auto result = eligibleResult(request.request.identity, 1000 + revision);
            require(concurrentController.acceptPrepared(result, revision * 100 + 20)
                        && concurrentController.markActivationPending(request.request.identity,
                                                                     revision * 100 + 30)
                        && concurrentController.markActive(request.request.identity,
                                                          revision * 100 + 40),
                    "Concurrent snapshot exercise must publish coherent complete states.");
        }
        stopReader.store(true, std::memory_order_release);
        reader.join();
        require(coherent.load(std::memory_order_acquire) && readCount.load() > 0,
                "Atomic immutable Publish snapshots must remain coherent for concurrent readers.");

        concurrentController.reset(true, true);
        const auto reset = concurrentController.getSnapshot();
        require(!reset.hasRequest && !reset.hasActiveRequest
                    && reset.preparationState == PerformancePublishPreparationState::idle,
                "Project reset must clear request and active identities without reviving work.");

        std::cout << "Mini Sprint 6.2 Publish controller matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.2 Publish controller matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
