#include "drs/engine/ProjectRestoreCoordinator.h"

#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

std::string displayPath(const fs::path& path)
{
    return path.lexically_normal().string();
}

std::string comparisonKey(const fs::path& path)
{
    auto key = displayPath(path);
#if defined(_WIN32)
    std::transform(key.begin(),
                   key.end(),
                   key.begin(),
                   [](const unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
#endif
    return key;
}

void appendCandidate(std::vector<fs::path>& candidates,
                     std::unordered_set<std::string>& seen,
                     const fs::path& candidate)
{
    if (candidate.empty())
        return;

    const auto normalized = candidate.lexically_normal();
    if (seen.insert(comparisonKey(normalized)).second)
        candidates.push_back(normalized);
}

std::vector<fs::path> buildResolutionCandidates(const HostProjectBinding& binding,
                                                const ProjectRestoreRequest& request,
                                                const std::size_t maximumSiblingEntries)
{
    std::vector<fs::path> candidates;
    std::unordered_set<std::string> seen;

    if (!request.locatedManifestPath.empty())
    {
        appendCandidate(candidates, seen, fs::u8path(request.locatedManifestPath));
        return candidates;
    }

    if (!binding.manifestPath.empty())
        appendCandidate(candidates, seen, fs::u8path(binding.manifestPath));

    if (!request.trustedBaseDirectory.empty() && !binding.portableRelativePath.empty())
    {
        appendCandidate(candidates,
                        seen,
                        fs::u8path(request.trustedBaseDirectory)
                            / fs::u8path(binding.portableRelativePath));
    }

    if (!binding.contentRootHint.empty() && !binding.manifestFileName.empty())
    {
        appendCandidate(candidates,
                        seen,
                        fs::u8path(binding.contentRootHint)
                            / fs::u8path(binding.manifestFileName));
    }

    if (binding.manifestPath.empty() || binding.manifestFileName.empty())
        return candidates;

    const auto previousDirectory = fs::u8path(binding.manifestPath).parent_path();
    const auto siblingRoot = previousDirectory.parent_path();
    std::error_code errorCode;
    if (siblingRoot.empty()
        || !fs::exists(siblingRoot, errorCode)
        || !fs::is_directory(siblingRoot, errorCode))
        return candidates;

    std::size_t inspected = 0;
    fs::directory_iterator iterator(siblingRoot,
                                    fs::directory_options::skip_permission_denied,
                                    errorCode);
    const fs::directory_iterator end;
    while (!errorCode && iterator != end && inspected < maximumSiblingEntries)
    {
        ++inspected;
        const auto entry = *iterator;
        iterator.increment(errorCode);
        if (!entry.is_directory(errorCode))
            continue;

        appendCandidate(candidates,
                        seen,
                        entry.path() / fs::u8path(binding.manifestFileName));
    }

    return candidates;
}

ProjectRestoreFinding findingFromHostState(const HostSessionStateParseResult& parsed)
{
    if (parsed.findings.empty())
        return ProjectRestoreFinding::invalidHostState;

    switch (parsed.findings.front().code)
    {
        case HostSessionStateFindingCode::projectIdentityMismatch:
            return ProjectRestoreFinding::identityMismatch;
        case HostSessionStateFindingCode::projectManifestDigestMismatch:
            return ProjectRestoreFinding::contentChanged;
        case HostSessionStateFindingCode::projectSnapshotInvalid:
        case HostSessionStateFindingCode::authoringStateInvalid:
            return ProjectRestoreFinding::checkpointInvalid;
        default:
            return ProjectRestoreFinding::invalidHostState;
    }
}

std::string messageFromHostState(const HostSessionStateParseResult& parsed)
{
    if (parsed.findings.empty())
        return parsed.state;
    return parsed.findings.front().message;
}

RuntimeProjectDocumentCheckpoint makeCheckpoint(const HostSessionState& hostState,
                                                RuntimeProjectModel project)
{
    RuntimeProjectDocumentCheckpoint checkpoint;
    checkpoint.project = std::move(project);
    checkpoint.revision = hostState.authoringState.revision;
    checkpoint.savedRevision = hostState.authoringState.savedRevision;
    checkpoint.dirty = hostState.authoringState.dirty;
    checkpoint.lastChangeLabel = "Restored from DAW host state";
    return checkpoint;
}
} // namespace

ProjectRestoreCoordinator::ProjectRestoreCoordinator(ProjectRestoreCoordinatorOptions coordinatorOptions)
    : options(std::move(coordinatorOptions))
{
    auto initial = std::make_shared<const ProjectRestoreSnapshot>();
    std::atomic_store_explicit(&publishedSnapshot, std::move(initial), std::memory_order_release);
    worker = std::thread([this] { runWorker(); });
}

ProjectRestoreCoordinator::~ProjectRestoreCoordinator()
{
    shutdown();
}

std::uint64_t ProjectRestoreCoordinator::submit(ProjectRestoreRequest request)
{
    if (shutdownRequested.load(std::memory_order_acquire))
        return 0;

    const auto generation = generationCounter.fetch_add(1, std::memory_order_acq_rel) + 1;
    auto observedGeneration = currentGeneration.load(std::memory_order_acquire);
    while (observedGeneration < generation
           && !currentGeneration.compare_exchange_weak(observedGeneration,
                                                       generation,
                                                       std::memory_order_acq_rel,
                                                       std::memory_order_acquire))
    {
    }
    if (currentGeneration.load(std::memory_order_acquire) != generation)
        return generation;

    {
        std::lock_guard<std::mutex> lock(requestMutex);
        if (currentGeneration.load(std::memory_order_acquire) == generation)
            pendingRequest = PendingRequest { generation, std::move(request) };
    }

    ProjectRestoreSnapshot parsing;
    parsing.generation = generation;
    parsing.state = ProjectRestoreState::parsing;
    parsing.message = "Parsing DAW host state";
    publishSnapshot(std::move(parsing));
    requestCondition.notify_one();
    return generation;
}

std::shared_ptr<const ProjectRestoreSnapshot> ProjectRestoreCoordinator::getSnapshot() const
{
    return std::atomic_load_explicit(&publishedSnapshot, std::memory_order_acquire);
}

bool ProjectRestoreCoordinator::publishLifecycleState(const std::uint64_t generation,
                                                      const ProjectRestoreState state,
                                                      const ProjectRestoreFinding finding,
                                                      std::string message)
{
    const auto current = getSnapshot();
    if (!current || current->generation != generation)
        return false;

    auto next = *current;
    next.state = state;
    next.finding = finding;
    next.message = std::move(message);
    return publishSnapshot(std::move(next));
}

void ProjectRestoreCoordinator::shutdown()
{
    const auto wasRequested = shutdownRequested.exchange(true, std::memory_order_acq_rel);
    if (!wasRequested)
    {
        {
            std::lock_guard<std::mutex> lock(requestMutex);
            pendingRequest.reset();
        }
        requestCondition.notify_all();
    }

    if (worker.joinable())
        worker.join();
}

bool ProjectRestoreCoordinator::isWorkerRunning() const noexcept
{
    return workerRunning.load(std::memory_order_acquire);
}

std::uint64_t ProjectRestoreCoordinator::latestGeneration() const noexcept
{
    return currentGeneration.load(std::memory_order_acquire);
}

void ProjectRestoreCoordinator::runWorker()
{
    workerRunning.store(true, std::memory_order_release);
    while (!shutdownRequested.load(std::memory_order_acquire))
    {
        std::optional<PendingRequest> request;
        {
            std::unique_lock<std::mutex> lock(requestMutex);
            requestCondition.wait(lock,
                                  [this]
                                  {
                                      return shutdownRequested.load(std::memory_order_acquire)
                                          || pendingRequest.has_value();
                                  });
            if (shutdownRequested.load(std::memory_order_acquire))
                break;

            request = std::move(pendingRequest);
            pendingRequest.reset();
        }

        if (request.has_value())
            processRequest(*request);
    }
    workerRunning.store(false, std::memory_order_release);
}

void ProjectRestoreCoordinator::processRequest(const PendingRequest& pending)
{
    if (!isCurrent(pending.generation))
        return;

    const auto parsed = parseHostSessionState(pending.request.serializedHostState);
    if (!isCurrent(pending.generation))
        return;

    if (parsed.isLegacyPreset())
    {
        ProjectRestoreSnapshot ready;
        ready.generation = pending.generation;
        ready.state = ProjectRestoreState::ready;
        ready.finding = ProjectRestoreFinding::legacyUnboundProject;
        ready.message = "Legacy preset restored without a project binding";
        ready.legacyPresetOnly = true;
        ready.legacyPreset = parsed.legacyPreset;
        publishSnapshot(std::move(ready));
        return;
    }

    if (!parsed.isValidHostState())
    {
        ProjectRestoreSnapshot failed;
        failed.generation = pending.generation;
        failed.state = ProjectRestoreState::failed;
        failed.finding = findingFromHostState(parsed);
        failed.message = messageFromHostState(parsed);
        publishSnapshot(std::move(failed));
        return;
    }

    const auto& hostState = *parsed.hostState;
    if (hostState.performancePackageBinding.has_value())
    {
        const auto& binding = *hostState.performancePackageBinding;
        const auto packagePath = pending.request.locatedManifestPath.empty()
            ? binding.packagePath : pending.request.locatedManifestPath;
        ProjectRestoreSnapshot loading;
        loading.generation = pending.generation;
        loading.state = ProjectRestoreState::loading;
        loading.expectedProjectId = binding.packageId;
        loading.resolvedManifestPath = packagePath;
        loading.performancePackageOnly = true;
        loading.hostState = hostState;
        loading.message = "Loading performance package metadata";
        if (!publishSnapshot(std::move(loading)) || !isCurrent(pending.generation))
            return;

        const auto dispatch = dispatchPerformancePackageReader(packagePath);
        PreparedPerformancePackageActivationResult activation;
        if (dispatch.format == PerformancePackageDiskFormat::version2)
        {
            const auto metadata = loadPerformancePackageV2Metadata(packagePath);
            if (metadata.loaded)
            {
                activation = preparePerformancePackageV2Activation(
                    metadata.metadata, metadata.package, metadata.sampleDescriptors);
            }
            else
            {
                activation.state = metadata.state;
                activation.issues = metadata.issues;
            }
        }
        else if (dispatch.format == PerformancePackageDiskFormat::version3)
        {
            const auto security = options.v3SecurityContextProvider
                ? options.v3SecurityContextProvider() : nullptr;
            if (security != nullptr)
            {
                const auto metadata = loadPerformancePackageV3Metadata(packagePath, *security);
                if (metadata.loaded)
                {
                    activation = preparePerformancePackageV3Activation(
                        metadata.metadata, metadata.package, metadata.contentKey,
                        metadata.sampleDescriptors);
                }
                else
                {
                    activation.state = metadata.state;
                    activation.issues = metadata.issues;
                }
            }
            else
            {
                activation.state = "Performance package V3 activation rejected";
                activation.issues = { "V3 activation rejected [configuration]." };
            }
        }
        else if (dispatch.opened)
        {
            const auto package = loadPerformancePackage(packagePath);
            if (package.loaded)
                activation = preparePerformancePackageActivation(package);
            else
            {
                activation.state = package.state;
                activation.issues = package.issues;
            }
        }

        if (!isCurrent(pending.generation))
            return;
        if (!activation.prepared
            || activation.packageLoad.manifest.packageId != binding.packageId)
        {
            ProjectRestoreSnapshot failed;
            failed.generation = pending.generation;
            failed.state = fs::is_regular_file(fs::path(packagePath))
                ? ProjectRestoreState::failed : ProjectRestoreState::needsLocation;
            failed.finding = fs::is_regular_file(fs::path(packagePath))
                ? ProjectRestoreFinding::performancePackageInvalid
                : ProjectRestoreFinding::candidateMissing;
            failed.expectedProjectId = binding.packageId;
            failed.resolvedManifestPath = packagePath;
            failed.performancePackageOnly = true;
            failed.hostState = hostState;
            failed.message = activation.issues.empty()
                ? (activation.state.empty()
                       ? std::string("The saved performance package could not be prepared.")
                       : activation.state)
                : activation.issues.front();
            publishSnapshot(std::move(failed));
            return;
        }

        ProjectRestoreSnapshot ready;
        ready.generation = pending.generation;
        ready.state = ProjectRestoreState::ready;
        ready.expectedProjectId = binding.packageId;
        ready.resolvedManifestPath = packagePath;
        ready.performancePackageOnly = true;
        ready.hostState = hostState;
        ready.packageActivation
            = std::make_shared<PreparedPerformancePackageActivationResult>(std::move(activation));
        ready.message = "Performance package activation is prepared";
        publishSnapshot(std::move(ready));
        return;
    }

    ProjectRestoreSnapshot resolving;
    resolving.generation = pending.generation;
    resolving.state = ProjectRestoreState::resolving;
    resolving.expectedProjectId = hostState.projectBinding.projectId;
    resolving.hostState = hostState;
    resolving.message = "Resolving authored project";
    if (!publishSnapshot(std::move(resolving)))
        return;

    RuntimeProjectModel resolvedProject;
    std::string resolvedManifestPath;
    bool fromSnapshot = false;

    if (hostState.authoringState.projectSnapshot.has_value())
    {
        resolvedProject = *hostState.authoringState.projectSnapshot;
        resolvedManifestPath = hostState.projectBinding.manifestPath;
        fromSnapshot = true;
    }
    else
    {
        std::vector<fs::path> candidates;
        try
        {
            candidates = buildResolutionCandidates(hostState.projectBinding,
                                                   pending.request,
                                                   options.maximumSiblingEntries);
        }
        catch (const fs::filesystem_error& exception)
        {
            ProjectRestoreSnapshot failed;
            failed.generation = pending.generation;
            failed.state = ProjectRestoreState::failed;
            failed.finding = ProjectRestoreFinding::projectLoadFailed;
            failed.expectedProjectId = hostState.projectBinding.projectId;
            failed.hostState = hostState;
            failed.message = exception.what();
            publishSnapshot(std::move(failed));
            return;
        }

        auto bestFinding = ProjectRestoreFinding::candidateMissing;
        std::string bestMessage = "The authored project manifest could not be located.";
        for (const auto& candidate : candidates)
        {
            if (!isCurrent(pending.generation))
                return;

            ProjectRestoreSnapshot loading;
            loading.generation = pending.generation;
            loading.state = ProjectRestoreState::loading;
            loading.expectedProjectId = hostState.projectBinding.projectId;
            loading.resolvedManifestPath = displayPath(candidate);
            loading.hostState = hostState;
            loading.message = "Loading authored project candidate";
            if (!publishSnapshot(std::move(loading)))
                return;

            const auto load = loadRuntimeProjectManifest(displayPath(candidate));
            if (!load.loaded)
            {
                if (load.manifestFound)
                {
                    bestFinding = ProjectRestoreFinding::projectLoadFailed;
                    bestMessage = load.issues.empty() ? load.state : load.issues.front();
                }
                continue;
            }

            const auto verification = verifyHostProjectBinding(hostState.projectBinding,
                                                               load.project,
                                                               displayPath(candidate));
            if (verification.match == HostProjectBindingMatch::projectIdentityMismatch)
            {
                bestFinding = ProjectRestoreFinding::identityMismatch;
                bestMessage = "The located manifest belongs to a different project ID.";
                continue;
            }
            if (verification.match == HostProjectBindingMatch::manifestDigestMismatch)
            {
                bestFinding = ProjectRestoreFinding::contentChanged;
                bestMessage = "The located manifest content differs from the saved DAW binding.";
                continue;
            }

            resolvedProject = load.project;
            resolvedManifestPath = displayPath(candidate);
            break;
        }

        if (resolvedProject.projectId.empty())
        {
            ProjectRestoreSnapshot needsLocation;
            needsLocation.generation = pending.generation;
            needsLocation.state = ProjectRestoreState::needsLocation;
            needsLocation.finding = bestFinding;
            needsLocation.expectedProjectId = hostState.projectBinding.projectId;
            needsLocation.hostState = hostState;
            needsLocation.message = std::move(bestMessage);
            publishSnapshot(std::move(needsLocation));
            return;
        }
    }

    if (!isCurrent(pending.generation))
        return;

    auto checkpoint = makeCheckpoint(hostState, std::move(resolvedProject));
    RuntimeProjectDocumentCheckpointConstraints constraints;
    constraints.expectedProjectId = hostState.projectBinding.projectId;
    constraints.manifestPath = resolvedManifestPath;
    const auto validation = validateRuntimeProjectDocumentCheckpoint(checkpoint, constraints);
    if (!validation.valid)
    {
        ProjectRestoreSnapshot failed;
        failed.generation = pending.generation;
        failed.state = ProjectRestoreState::failed;
        failed.finding = ProjectRestoreFinding::checkpointInvalid;
        failed.expectedProjectId = hostState.projectBinding.projectId;
        failed.resolvedManifestPath = resolvedManifestPath;
        failed.hostState = hostState;
        failed.message = validation.issues.empty()
            ? "The authored project checkpoint is invalid."
            : validation.issues.front();
        publishSnapshot(std::move(failed));
        return;
    }

    ProjectRestoreSnapshot preparing;
    preparing.generation = pending.generation;
    preparing.state = ProjectRestoreState::preparing;
    preparing.expectedProjectId = hostState.projectBinding.projectId;
    preparing.resolvedManifestPath = resolvedManifestPath;
    preparing.restoredFromEmbeddedSnapshot = fromSnapshot;
    preparing.hostState = hostState;
    preparing.checkpoint = checkpoint;
    preparing.message = "Preparing validated authored project checkpoint";
    if (!publishSnapshot(std::move(preparing)) || !isCurrent(pending.generation))
        return;

    ProjectRestoreSnapshot ready;
    ready.generation = pending.generation;
    ready.state = ProjectRestoreState::ready;
    ready.expectedProjectId = hostState.projectBinding.projectId;
    ready.resolvedManifestPath = resolvedManifestPath;
    ready.restoredFromEmbeddedSnapshot = fromSnapshot;
    ready.hostState = hostState;
    ready.checkpoint = std::move(checkpoint);
    ready.message = "Authored project checkpoint is ready";
    publishSnapshot(std::move(ready));
}

bool ProjectRestoreCoordinator::isCurrent(const std::uint64_t generation) const noexcept
{
    return !shutdownRequested.load(std::memory_order_acquire)
        && currentGeneration.load(std::memory_order_acquire) == generation;
}

bool ProjectRestoreCoordinator::publishSnapshot(ProjectRestoreSnapshot snapshot)
{
    if (!isCurrent(snapshot.generation))
        return false;

    const auto generation = snapshot.generation;
    const auto state = snapshot.state;
    std::shared_ptr<const ProjectRestoreSnapshot> immutable
        = std::make_shared<const ProjectRestoreSnapshot>(std::move(snapshot));
    std::atomic_store_explicit(&publishedSnapshot, std::move(immutable), std::memory_order_release);
    if (options.stageObserver)
        options.stageObserver(generation, state);
    return isCurrent(generation);
}

const char* toString(const ProjectRestoreState state) noexcept
{
    switch (state)
    {
        case ProjectRestoreState::idle: return "Idle";
        case ProjectRestoreState::parsing: return "Parsing";
        case ProjectRestoreState::resolving: return "Resolving";
        case ProjectRestoreState::needsLocation: return "NeedsLocation";
        case ProjectRestoreState::loading: return "Loading";
        case ProjectRestoreState::preparing: return "Preparing";
        case ProjectRestoreState::ready: return "Ready";
        case ProjectRestoreState::active: return "Active";
        case ProjectRestoreState::degraded: return "Degraded";
        case ProjectRestoreState::failed: return "Failed";
    }
    return "Failed";
}

const char* toString(const ProjectRestoreFinding finding) noexcept
{
    switch (finding)
    {
        case ProjectRestoreFinding::none: return "None";
        case ProjectRestoreFinding::legacyUnboundProject: return "LegacyUnboundProject";
        case ProjectRestoreFinding::requestSuperseded: return "RequestSuperseded";
        case ProjectRestoreFinding::requestCanceled: return "RequestCanceled";
        case ProjectRestoreFinding::invalidHostState: return "InvalidHostState";
        case ProjectRestoreFinding::candidateMissing: return "CandidateMissing";
        case ProjectRestoreFinding::identityMismatch: return "IdentityMismatch";
        case ProjectRestoreFinding::contentChanged: return "ContentChanged";
        case ProjectRestoreFinding::projectLoadFailed: return "ProjectLoadFailed";
        case ProjectRestoreFinding::checkpointInvalid: return "CheckpointInvalid";
        case ProjectRestoreFinding::projectBindingInvalid: return "ProjectBindingInvalid";
        case ProjectRestoreFinding::performancePackageInvalid: return "PerformancePackageInvalid";
        case ProjectRestoreFinding::presetStateInvalid: return "PresetStateInvalid";
        case ProjectRestoreFinding::articulationMismatch: return "ArticulationMismatch";
        case ProjectRestoreFinding::draftPlaybackFailed: return "DraftPlaybackFailed";
        case ProjectRestoreFinding::publishSchedulingFailed: return "PublishSchedulingFailed";
        case ProjectRestoreFinding::publishedIdentityMismatch: return "PublishedIdentityMismatch";
        case ProjectRestoreFinding::preparationFailed: return "PreparationFailed";
        case ProjectRestoreFinding::shutdown: return "Shutdown";
    }
    return "InvalidHostState";
}
} // namespace drs::engine
