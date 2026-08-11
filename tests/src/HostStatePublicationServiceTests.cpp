#include "drs/engine/HostStatePublicationService.h"
#include "drs/engine/RuntimeLoader.h"

#include <chrono>
#include <iostream>
#include <memory>
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

drs::engine::RuntimeProjectModel buildLargeProject()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded && !loaded.project.sampleSources.empty()
                && !loaded.project.authoring.zones.empty(),
            "Host-state publication coverage requires the authored reference project.");
    auto project = loaded.project;
    const auto sampleTemplate = project.sampleSources.front();
    const auto zoneTemplate = project.authoring.zones.front();
    for (int index = 0; index < 600; ++index)
    {
        auto sample = sampleTemplate;
        sample.id = "host-state-source-" + std::to_string(index);
        project.sampleSources.push_back(sample);
        auto zone = zoneTemplate;
        zone.id = "host-state-zone-" + std::to_string(index);
        zone.sampleSourceId = sample.id;
        project.authoring.zones.push_back(zone);
    }
    return project;
}

drs::engine::HostStatePublicationRequest makeRequest(
    std::uint64_t requestId,
    std::size_t revision,
    const std::shared_ptr<const drs::engine::RuntimeProjectModel>& project,
    const drs::engine::RuntimePresetState& preset)
{
    drs::engine::HostStatePublicationRequest request;
    request.requestId = requestId;
    request.publicationKey = "host-state-key-" + std::to_string(requestId);
    request.kind = drs::engine::HostStatePublicationKind::authoringProject;
    request.presetState = preset;
    request.project = project;
    request.projectBinding.projectId = project->projectId;
    request.projectBinding.manifestFileName = "host-state-publication-test.drsproj";
    request.revision = revision;
    request.savedRevision = 0;
    request.dirty = true;
    return request;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        const auto manifest = loadPhase1ReferenceInstrumentManifest();
        require(manifest.loaded, "Host-state publication coverage requires the reference instrument.");
        const auto preset = captureRuntimePresetState(buildDefaultRuntimeSessionState(manifest));
        const auto project = std::make_shared<const RuntimeProjectModel>(buildLargeProject());

        HostStatePublicationService service;
        require(service.submit(makeRequest(1, 1, project, preset)),
                "The background serializer rejected its first immutable checkpoint.");
        const auto inFlightDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!service.getStatus().inFlight && std::chrono::steady_clock::now() < inFlightDeadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(service.getStatus().inFlight,
                "The serialization worker did not claim the first immutable checkpoint.");

        for (std::uint64_t requestId = 2; requestId <= 101; ++requestId)
        {
            require(service.submit(makeRequest(requestId,
                                               static_cast<std::size_t>(requestId),
                                               project,
                                               preset)),
                    "The serializer rejected a newer coalescing checkpoint.");
        }
        require(service.waitForIdle(30000),
                "The coalescing host-state serializer did not become idle.");

        const auto status = service.getStatus();
        const auto completions = service.drainCompleted();
        require(status.submittedCount == 101
                    && status.latestSubmittedRequestId == 101
                    && status.latestCompletedRequestId == 101
                    && status.latestCompletedRevision == 101
                    && status.coalescedCount >= 99
                    && status.maximumPendingCount == 1
                    && status.maximumPendingCompletionCount == 1
                    && !status.inFlight && status.pendingCount == 0,
                "Host-state churn must retain one bounded newest checkpoint and complete it last.");
        require(status.completedCount == 2
                    && completions.size() == 1
                    && completions.back().requestId == 101
                    && completions.back().serialized,
                "The worker must complete the in-flight and newest requests while retaining only the newest success.");

        const auto parsed = parseHostSessionState(completions.back().text,
                                                  "host-state-publication-test.drsproj");
        require(parsed.isValidHostState()
                    && parsed.hostState->authoringState.revision == 101
                    && parsed.hostState->authoringState.projectSnapshot.has_value()
                    && parsed.hostState->authoringState.projectSnapshot->projectId == project->projectId,
                "The newest serialized checkpoint must round-trip with exact revision and project identity.");

        for (std::uint64_t requestId = 102; requestId <= 111; ++requestId)
        {
            require(service.submit(makeRequest(requestId,
                                               static_cast<std::size_t>(requestId),
                                               project,
                                               preset))
                        && service.waitForIdle(30000),
                    "Sequential mailbox-bound coverage did not complete a checkpoint.");
        }
        const auto mailboxStatus = service.getStatus();
        require(mailboxStatus.pendingCompletionCount == 1
                    && mailboxStatus.maximumPendingCompletionCount == 1
                    && service.drainCompleted().back().requestId == 111,
                "Undrained successful publications must remain a one-result newest-wins mailbox.");

        const auto shutdownStarted = std::chrono::steady_clock::now();
        {
            HostStatePublicationService shutdownService;
            require(shutdownService.submit(makeRequest(1, 1, project, preset)),
                    "Shutdown coverage could not queue an immutable checkpoint.");
        }
        const auto shutdownMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - shutdownStarted).count());
        require(shutdownMicros < 30000000,
                "Worker shutdown did not join safely inside the qualification timeout.");

        std::cout << "Host-state background publication passed: submitted="
                  << status.submittedCount << " coalesced=" << status.coalescedCount
                  << " completed=" << status.completedCount
                  << " maxSerializeUs=" << status.maximumDurationMicros
                  << " shutdownUs=" << shutdownMicros << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Host-state background publication failed: " << error.what() << std::endl;
        return 1;
    }
}
