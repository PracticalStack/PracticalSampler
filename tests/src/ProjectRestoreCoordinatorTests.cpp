#include "drs/engine/HostSessionState.h"
#include "drs/engine/ProjectRestoreCoordinator.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
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

void writeTextFile(const fs::path& path, const std::string& text)
{
    std::error_code errorCode;
    fs::create_directories(path.parent_path(), errorCode);
    if (errorCode)
        throw std::runtime_error("Could not create restore test directory.");

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << text;
    if (!output.good())
        throw std::runtime_error("Could not write restore test manifest.");
}

drs::engine::RuntimePresetState loadLegacyPreset()
{
    const auto fixture = fs::path(DRS_SOURCE_ROOT)
        / "content/runtime/phase1/host-state/legacy/lead-performance.preset-state.json";
    const auto parsed = drs::engine::parseHostSessionState(readTextFile(fixture));
    require(parsed.isLegacyPreset(), "Coordinator tests require the valid legacy preset fixture.");
    return *parsed.legacyPreset;
}

drs::engine::HostSessionState makeSavedState(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& manifestPath,
    const drs::engine::RuntimePresetState& preset)
{
    drs::engine::HostSessionState state;
    state.presetState = preset;
    state.projectBinding.projectId = project.projectId;
    state.projectBinding.manifestPath = manifestPath;
    state.projectBinding.manifestFileName = fs::u8path(manifestPath).filename().string();
    state.projectBinding.manifestDigest
        = drs::engine::computeHostProjectManifestDigest(project, manifestPath);
    state.projectBinding.contentRootHint = fs::u8path(manifestPath).parent_path().string();
    state.authoringState.revision = 0;
    state.authoringState.savedRevision = 0;
    state.authoringState.dirty = false;
    return state;
}

std::string serializeState(const drs::engine::HostSessionState& state)
{
    const auto serialized = drs::engine::serializeHostSessionState(state);
    require(serialized.serialized,
            serialized.findings.empty()
                ? "Host state did not serialize."
                : serialized.findings.front().message);
    return serialized.text;
}

std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> waitForTerminal(
    drs::engine::ProjectRestoreCoordinator& coordinator,
    const std::uint64_t generation)
{
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto snapshot = coordinator.getSnapshot();
        if (snapshot && snapshot->generation == generation
            && (snapshot->state == drs::engine::ProjectRestoreState::ready
                || snapshot->state == drs::engine::ProjectRestoreState::needsLocation
                || snapshot->state == drs::engine::ProjectRestoreState::failed))
            return snapshot;

        std::this_thread::sleep_for(2ms);
    }

    throw std::runtime_error("Timed out waiting for a terminal coordinator snapshot.");
}

bool containsState(const std::vector<drs::engine::ProjectRestoreState>& states,
                   const drs::engine::ProjectRestoreState expected)
{
    return std::find(states.begin(), states.end(), expected) != states.end();
}
} // namespace

int main()
{
    try
    {
        const auto preset = loadLegacyPreset();
        const auto projectPath = drs::engine::getPhase2ReferenceProjectManifestPath();
        const auto projectLoad = drs::engine::loadRuntimeProjectManifest(projectPath);
        require(projectLoad.loaded, "Coordinator tests require the Phase 2 reference project.");

        std::mutex observedMutex;
        std::vector<drs::engine::ProjectRestoreState> observedStates;
        drs::engine::ProjectRestoreCoordinatorOptions observedOptions;
        observedOptions.stageObserver =
            [&](const std::uint64_t, const drs::engine::ProjectRestoreState state)
            {
                std::lock_guard<std::mutex> lock(observedMutex);
                observedStates.push_back(state);
            };
        drs::engine::ProjectRestoreCoordinator coordinator(std::move(observedOptions));

        const auto savedState = makeSavedState(projectLoad.project, projectPath, preset);
        const auto savedGeneration = coordinator.submit({ serializeState(savedState), {}, {} });
        const auto savedReady = waitForTerminal(coordinator, savedGeneration);
        require(savedReady->state == drs::engine::ProjectRestoreState::ready,
                "An exact saved-project locator must restore to Ready.");
        require(savedReady->checkpoint.has_value()
                    && savedReady->checkpoint->project.projectId == projectLoad.project.projectId
                    && savedReady->checkpoint->revision == 0
                    && !savedReady->checkpoint->dirty,
                "Saved-project resolution must produce a validated document checkpoint.");
        require(savedReady->resolvedManifestPath == fs::u8path(projectPath).lexically_normal().string(),
                "Exact resolution must publish the validated manifest path.");

        {
            std::lock_guard<std::mutex> lock(observedMutex);
            require(containsState(observedStates, drs::engine::ProjectRestoreState::parsing)
                        && containsState(observedStates, drs::engine::ProjectRestoreState::resolving)
                        && containsState(observedStates, drs::engine::ProjectRestoreState::loading)
                        && containsState(observedStates, drs::engine::ProjectRestoreState::preparing)
                        && containsState(observedStates, drs::engine::ProjectRestoreState::ready),
                    "Successful restore must publish Parsing, Resolving, Loading, Preparing, and Ready.");
        }

        require(coordinator.publishLifecycleState(savedGeneration,
                                                  drs::engine::ProjectRestoreState::active,
                                                  {},
                                                  "Matching playback active"),
                "The consumer must be able to publish Active for the current generation.");
        require(coordinator.getSnapshot()->state == drs::engine::ProjectRestoreState::active,
                "Active lifecycle publication must be observable.");
        require(coordinator.publishLifecycleState(savedGeneration,
                                                  drs::engine::ProjectRestoreState::degraded,
                                                  drs::engine::ProjectRestoreFinding::projectLoadFailed,
                                                  "Samples require repair"),
                "The consumer must be able to publish Degraded for the current generation.");
        require(coordinator.getSnapshot()->state == drs::engine::ProjectRestoreState::degraded,
                "Degraded lifecycle publication must be observable.");

        auto dirtyState = makeSavedState(projectLoad.project,
                                         "C:/Unsaved/dirty-authoring.drsproj",
                                         preset);
        dirtyState.projectBinding.manifestDigest
            = drs::engine::computeHostProjectManifestDigest(
                projectLoad.project,
                dirtyState.projectBinding.manifestPath);
        dirtyState.authoringState.revision = 7;
        dirtyState.authoringState.savedRevision = 6;
        dirtyState.authoringState.dirty = true;
        dirtyState.authoringState.projectSnapshot = projectLoad.project;
        const auto dirtyGeneration = coordinator.submit({ serializeState(dirtyState), {}, {} });
        const auto dirtyReady = waitForTerminal(coordinator, dirtyGeneration);
        require(dirtyReady->state == drs::engine::ProjectRestoreState::ready
                    && dirtyReady->restoredFromEmbeddedSnapshot
                    && dirtyReady->checkpoint.has_value()
                    && dirtyReady->checkpoint->project.projectId
                        == savedReady->checkpoint->project.projectId
                    && dirtyReady->checkpoint->revision == 7
                    && dirtyReady->checkpoint->savedRevision == 6
                    && dirtyReady->checkpoint->dirty,
                "Embedded dirty snapshots and saved files must yield the same validated checkpoint type.");

        auto missingState = savedState;
        missingState.projectBinding.manifestPath
            = "C:/DefinitelyMissing/authoring-project.drsproj";
        missingState.projectBinding.contentRootHint.clear();
        missingState.projectBinding.portableRelativePath.clear();
        missingState.projectBinding.manifestFileName = "authoring-project.drsproj";
        missingState.projectBinding.manifestDigest
            = drs::engine::computeHostProjectManifestDigest(
                projectLoad.project,
                missingState.projectBinding.manifestPath);
        const auto missingGeneration = coordinator.submit({ serializeState(missingState), {}, {} });
        const auto needsLocation = waitForTerminal(coordinator, missingGeneration);
        require(needsLocation->state == drs::engine::ProjectRestoreState::needsLocation
                    && needsLocation->finding
                        == drs::engine::ProjectRestoreFinding::candidateMissing,
                "An unresolved bounded search must publish NeedsLocation/CandidateMissing.");

        const auto phase1Path = drs::engine::getPhase1ReferenceProjectManifestPath();
        const auto identityGeneration = coordinator.submit(
            { serializeState(savedState), {}, phase1Path });
        const auto identityMismatch = waitForTerminal(coordinator, identityGeneration);
        require(identityMismatch->state == drs::engine::ProjectRestoreState::needsLocation
                    && identityMismatch->finding
                        == drs::engine::ProjectRestoreFinding::identityMismatch,
                "A located manifest with the wrong project ID must publish IdentityMismatch.");

        const auto tempRoot = fs::temp_directory_path() / "drs-project-restore-coordinator-tests";
        std::error_code cleanupError;
        fs::remove_all(tempRoot, cleanupError);

        const auto changedPath = tempRoot / "changed/content-changed.drsproj";
        auto changedProject = projectLoad.project;
        changedProject.displayName += " changed";
        writeTextFile(changedPath,
                      drs::engine::serializeRuntimeProjectManifest(
                          changedProject,
                          changedPath.string()));
        auto changedBindingState = makeSavedState(projectLoad.project,
                                                  changedPath.string(),
                                                  preset);
        const auto contentGeneration = coordinator.submit(
            { serializeState(changedBindingState), {}, changedPath.string() });
        const auto contentMismatch = waitForTerminal(coordinator, contentGeneration);
        require(contentMismatch->state == drs::engine::ProjectRestoreState::needsLocation
                    && contentMismatch->finding
                        == drs::engine::ProjectRestoreFinding::contentChanged,
                "A same-ID manifest with changed canonical content must publish ContentChanged.");

        auto portableState = savedState;
        const auto trustedBase = fs::u8path(projectPath).parent_path().parent_path();
        portableState.projectBinding.manifestPath = "C:/OldLocation/missing.drsproj";
        portableState.projectBinding.contentRootHint.clear();
        portableState.projectBinding.portableRelativePath
            = fs::relative(fs::u8path(projectPath), trustedBase).generic_string();
        const auto portableGeneration = coordinator.submit(
            { serializeState(portableState), trustedBase.string(), {} });
        const auto portableReady = waitForTerminal(coordinator, portableGeneration);
        require(portableReady->state == drs::engine::ProjectRestoreState::ready,
                "A trusted base plus portableRelativePath must resolve a saved project.");

        auto rootHintState = savedState;
        rootHintState.projectBinding.manifestPath = "C:/OldLocation/missing.drsproj";
        rootHintState.projectBinding.contentRootHint
            = fs::u8path(projectPath).parent_path().string();
        rootHintState.projectBinding.manifestFileName
            = fs::u8path(projectPath).filename().string();
        rootHintState.projectBinding.portableRelativePath.clear();
        const auto rootHintGeneration = coordinator.submit({ serializeState(rootHintState), {}, {} });
        const auto rootHintReady = waitForTerminal(coordinator, rootHintGeneration);
        require(rootHintReady->state == drs::engine::ProjectRestoreState::ready,
                "A bounded content-root hint must resolve the expected filename.");

        const auto movedPath = tempRoot / "siblings/new-home/sibling-project.drsproj";
        writeTextFile(movedPath,
                      drs::engine::serializeRuntimeProjectManifest(
                          projectLoad.project,
                          movedPath.string()));
        auto siblingState = makeSavedState(projectLoad.project,
                                           movedPath.string(),
                                           preset);
        siblingState.projectBinding.manifestPath
            = (tempRoot / "siblings/old-home/sibling-project.drsproj").string();
        siblingState.projectBinding.contentRootHint.clear();
        siblingState.projectBinding.portableRelativePath.clear();
        const auto siblingGeneration = coordinator.submit({ serializeState(siblingState), {}, {} });
        const auto siblingReady = waitForTerminal(coordinator, siblingGeneration);
        require(siblingReady->state == drs::engine::ProjectRestoreState::ready
                    && fs::equivalent(fs::u8path(siblingReady->resolvedManifestPath), movedPath),
                "One bounded sibling-directory search must find the expected filename.");

        const auto nestedPath = tempRoot / "bounded/new-home/deeper/nested-project.drsproj";
        writeTextFile(nestedPath,
                      drs::engine::serializeRuntimeProjectManifest(
                          projectLoad.project,
                          nestedPath.string()));
        auto nestedState = makeSavedState(projectLoad.project,
                                          nestedPath.string(),
                                          preset);
        nestedState.projectBinding.manifestPath
            = (tempRoot / "bounded/old-home/nested-project.drsproj").string();
        nestedState.projectBinding.contentRootHint.clear();
        nestedState.projectBinding.portableRelativePath.clear();
        const auto nestedGeneration = coordinator.submit({ serializeState(nestedState), {}, {} });
        const auto nestedNeedsLocation = waitForTerminal(coordinator, nestedGeneration);
        require(nestedNeedsLocation->state == drs::engine::ProjectRestoreState::needsLocation,
                "Sibling resolution must not recurse into arbitrary descendant directories.");

        require(std::string(drs::engine::toString(drs::engine::ProjectRestoreState::idle)) == "Idle"
                    && std::string(drs::engine::toString(
                           drs::engine::ProjectRestoreState::needsLocation)) == "NeedsLocation"
                    && std::string(drs::engine::toString(
                           drs::engine::ProjectRestoreState::failed)) == "Failed",
                "Restore lifecycle labels must preserve the public status vocabulary.");

        std::mutex blockMutex;
        std::condition_variable blockCondition;
        bool firstBlocked = false;
        bool releaseFirst = false;
        drs::engine::ProjectRestoreCoordinatorOptions supersessionOptions;
        supersessionOptions.stageObserver =
            [&](const std::uint64_t generation, const drs::engine::ProjectRestoreState state)
            {
                if (generation != 1 || state != drs::engine::ProjectRestoreState::resolving)
                    return;

                std::unique_lock<std::mutex> lock(blockMutex);
                firstBlocked = true;
                blockCondition.notify_all();
                blockCondition.wait(lock, [&] { return releaseFirst; });
            };
        drs::engine::ProjectRestoreCoordinator supersessionCoordinator(
            std::move(supersessionOptions));
        const auto firstGeneration = supersessionCoordinator.submit(
            { serializeState(savedState), {}, {} });
        {
            std::unique_lock<std::mutex> lock(blockMutex);
            require(blockCondition.wait_for(lock, 2s, [&] { return firstBlocked; }),
                    "The first generation must reach the deterministic test barrier.");
        }
        const auto secondGeneration = supersessionCoordinator.submit(
            { serializeState(dirtyState), {}, {} });
        {
            std::lock_guard<std::mutex> lock(blockMutex);
            releaseFirst = true;
        }
        blockCondition.notify_all();
        const auto supersedingReady = waitForTerminal(supersessionCoordinator, secondGeneration);
        require(firstGeneration < secondGeneration
                    && supersedingReady->generation == secondGeneration
                    && supersedingReady->checkpoint.has_value()
                    && supersedingReady->checkpoint->revision == 7,
                "A new generation must supersede blocked work and stale completion must not publish.");
        require(!supersessionCoordinator.publishLifecycleState(
                    firstGeneration,
                    drs::engine::ProjectRestoreState::active),
                "A stale generation must not mutate lifecycle state.");

        std::atomic<std::size_t> shutdownObserverCalls { 0 };
        drs::engine::ProjectRestoreCoordinatorOptions shutdownOptions;
        shutdownOptions.stageObserver =
            [&](const std::uint64_t, const drs::engine::ProjectRestoreState)
            {
                shutdownObserverCalls.fetch_add(1, std::memory_order_relaxed);
            };
        drs::engine::ProjectRestoreCoordinator shutdownCoordinator(std::move(shutdownOptions));
        shutdownCoordinator.submit({ serializeState(savedState), {}, {} });
        shutdownCoordinator.shutdown();
        const auto callsAtShutdown = shutdownObserverCalls.load(std::memory_order_acquire);
        std::this_thread::sleep_for(25ms);
        require(!shutdownCoordinator.isWorkerRunning()
                    && shutdownObserverCalls.load(std::memory_order_acquire) == callsAtShutdown,
                "Shutdown must join owned work and prevent post-shutdown restore callbacks.");
        require(shutdownCoordinator.submit({ serializeState(savedState), {}, {} }) == 0,
                "A shut-down coordinator must reject new work.");

        coordinator.shutdown();
        supersessionCoordinator.shutdown();
        fs::remove_all(tempRoot, cleanupError);

        std::cout << "Project restore coordinator tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Project restore coordinator tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
