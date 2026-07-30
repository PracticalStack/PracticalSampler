#include "drs/engine/ProjectRestoreCoordinator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
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

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

void waitForReady(drs::engine::ProjectRestoreCoordinator& coordinator,
                  const std::uint64_t generation)
{
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto snapshot = coordinator.getSnapshot();
        if (snapshot && snapshot->generation == generation
            && snapshot->state == drs::engine::ProjectRestoreState::ready)
            return;
        std::this_thread::yield();
    }
    throw std::runtime_error("Stress restore did not publish its final Ready generation.");
}
} // namespace

int main()
{
    try
    {
        const auto dirtyFixture = readTextFile(
            fs::path(DRS_SOURCE_ROOT)
            / "content/runtime/phase1/host-state/reference/dirty-project.hoststate.json");
        require(!dirtyFixture.empty(), "Stress test requires the dirty host-state fixture.");

        constexpr int teardownPasses = 24;
        constexpr int producerCount = 4;
        constexpr int requestsPerProducer = 24;
        for (int pass = 0; pass < teardownPasses; ++pass)
        {
            drs::engine::ProjectRestoreCoordinator coordinator;
            std::atomic<bool> readersRunning { true };
            std::atomic<std::uint64_t> maximumObservedGeneration { 0 };
            std::thread reader(
                [&]
                {
                    while (readersRunning.load(std::memory_order_acquire))
                    {
                        const auto snapshot = coordinator.getSnapshot();
                        if (snapshot)
                        {
                            auto observed = maximumObservedGeneration.load(
                                std::memory_order_relaxed);
                            while (observed < snapshot->generation
                                   && !maximumObservedGeneration.compare_exchange_weak(
                                       observed,
                                       snapshot->generation,
                                       std::memory_order_release,
                                       std::memory_order_relaxed))
                            {
                            }
                        }
                        std::this_thread::yield();
                    }
                });

            std::vector<std::thread> producers;
            producers.reserve(producerCount);
            for (int producer = 0; producer < producerCount; ++producer)
            {
                producers.emplace_back(
                    [&]
                    {
                        for (int request = 0; request < requestsPerProducer; ++request)
                            coordinator.submit({ dirtyFixture, {}, {} });
                    });
            }
            for (auto& producer : producers)
                producer.join();

            const auto finalGeneration = coordinator.submit({ dirtyFixture, {}, {} });
            waitForReady(coordinator, finalGeneration);
            readersRunning.store(false, std::memory_order_release);
            reader.join();
            require(coordinator.getSnapshot()->generation == finalGeneration
                        && coordinator.latestGeneration() == finalGeneration
                        && maximumObservedGeneration.load(std::memory_order_acquire)
                            <= finalGeneration,
                    "Concurrent submissions must converge on the final immutable generation.");

            coordinator.shutdown();
            require(!coordinator.isWorkerRunning(),
                    "Every stress pass must join its restore worker before teardown.");
        }

        std::cout << "Host-state restore stress tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Host-state restore stress tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
