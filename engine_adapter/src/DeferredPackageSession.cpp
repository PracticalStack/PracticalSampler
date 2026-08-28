#include "drs/engine/DeferredPackageSession.h"

namespace drs::engine
{
const char* toString(const DeferredPackageSessionStage stage) noexcept
{
    switch (stage)
    {
        case DeferredPackageSessionStage::idle: return "idle";
        case DeferredPackageSessionStage::locatorPending: return "locator-pending";
        case DeferredPackageSessionStage::metadataReady: return "metadata-ready";
        case DeferredPackageSessionStage::openingSources: return "opening-sources";
        case DeferredPackageSessionStage::preparingHeads: return "preparing-heads";
        case DeferredPackageSessionStage::buildingModel: return "building-model";
        case DeferredPackageSessionStage::playable: return "playable";
        case DeferredPackageSessionStage::pendingActivation: return "pending-activation";
        case DeferredPackageSessionStage::active: return "active";
        case DeferredPackageSessionStage::degraded: return "degraded";
        case DeferredPackageSessionStage::failed: return "failed";
        case DeferredPackageSessionStage::cancelled: return "cancelled";
    }
    return "unknown";
}

void DeferredPackageSession::resetRequestState()
{
    preparedModel.reset();
    nextHeadIndex = 0;
    expectedRevision = 0;
    expectedPreparedBuildId = 0;
}

bool DeferredPackageSession::begin(DeferredPackageSessionPlan plan)
{
    resetRequestState();
    currentPlan = std::move(plan);
    sessionSnapshot.requestGeneration = nextRequestGeneration++;
    sessionSnapshot.packagePath = currentPlan.packagePath;
    sessionSnapshot.packageId = ! currentPlan.authenticatedPackageId.empty()
        ? currentPlan.authenticatedPackageId
        : (currentPlan.package == nullptr ? std::string {} : currentPlan.package->packageId);
    sessionSnapshot.readyHeadCount = 0;
    sessionSnapshot.totalHeadCount = currentPlan.sources.size();
    sessionSnapshot.metadataAccepted = false;
    sessionSnapshot.headReady = false;
    sessionSnapshot.playable = false;
    sessionSnapshot.active = sessionSnapshot.activeGeneration != 0;
    sessionSnapshot.retryable = false;
    sessionSnapshot.failureCategory.clear();
    sessionSnapshot.failureSourceId.clear();
    const auto hasAuthenticatedMetadata = ! currentPlan.authenticatedPackageId.empty()
        || (currentPlan.package != nullptr && currentPlan.package->opened);
    if (! hasAuthenticatedMetadata
        || currentPlan.packagePath.empty() || currentPlan.sources.empty()
        || !currentPlan.buildRenderModel)
    {
        fail("package-metadata", {}, "Package metadata or deferred source plan is invalid.", false);
        return false;
    }
    sessionSnapshot.stage = DeferredPackageSessionStage::metadataReady;
    sessionSnapshot.metadataAccepted = true;
    sessionSnapshot.status = "Package metadata accepted; audio is not playable yet.";
    return true;
}

bool DeferredPackageSession::requestLocatorRestore(
    std::string packagePath,
    std::function<DeferredPackageSessionPlan(const std::string&)> resolver)
{
    if (packagePath.empty() || !resolver)
        return false;
    locatorResolver = std::move(resolver);
    sessionSnapshot.packagePath = std::move(packagePath);
    sessionSnapshot.stage = DeferredPackageSessionStage::locatorPending;
    sessionSnapshot.metadataAccepted = false;
    sessionSnapshot.headReady = false;
    sessionSnapshot.playable = false;
    sessionSnapshot.active = sessionSnapshot.activeGeneration != 0;
    sessionSnapshot.status = "Package locator queued for background resolution.";
    return true;
}

bool DeferredPackageSession::serviceNextWorkerStep()
{
    if (sessionSnapshot.stage == DeferredPackageSessionStage::locatorPending)
    {
        auto resolver = std::move(locatorResolver);
        const auto path = sessionSnapshot.packagePath;
        return resolver && begin(resolver(path));
    }
    switch (sessionSnapshot.stage)
    {
        case DeferredPackageSessionStage::metadataReady:
            sessionSnapshot.stage = DeferredPackageSessionStage::openingSources;
            sessionSnapshot.status = "Opening package sample sources.";
            return true;
        case DeferredPackageSessionStage::openingSources:
            for (const auto& source : currentPlan.sources)
                if (source == nullptr
                    || source->descriptor().kind != SampleDataSourceKind::packageRecord)
                {
                    fail("source-open", source == nullptr ? std::string {}
                                                           : source->descriptor().sourceId,
                         "A package sample source could not be opened.", true);
                    return true;
                }
            sessionSnapshot.stage = DeferredPackageSessionStage::preparingHeads;
            sessionSnapshot.status = "Preparing bounded package heads.";
            return true;
        case DeferredPackageSessionStage::preparingHeads:
            if (nextHeadIndex < currentPlan.sources.size())
            {
                const auto& source = currentPlan.sources[nextHeadIndex];
                if (!source->prepareHead())
                {
                    fail("head-auth-or-read", source->descriptor().sourceId,
                         source->lastFailure(), true);
                    return true;
                }
                ++nextHeadIndex;
                sessionSnapshot.readyHeadCount = nextHeadIndex;
                if (nextHeadIndex < currentPlan.sources.size())
                    return true;
            }
            sessionSnapshot.headReady = true;
            sessionSnapshot.stage = DeferredPackageSessionStage::buildingModel;
            sessionSnapshot.status = "Package heads ready; building render model.";
            return true;
        case DeferredPackageSessionStage::buildingModel:
            preparedModel = currentPlan.buildRenderModel();
            if (preparedModel == nullptr)
            {
                fail("render-model", {}, "Package render model construction failed.", true);
                return true;
            }
            expectedRevision = preparedModel->getRevision();
            expectedPreparedBuildId = preparedModel->getPreparedBuildId();
            sessionSnapshot.stage = DeferredPackageSessionStage::playable;
            sessionSnapshot.playable = true;
            sessionSnapshot.status = "Package is playable and awaiting activation staging.";
            return true;
        default:
            return false;
    }
}

bool DeferredPackageSession::stagePlayableActivation(SamplerPlaybackContext& context)
{
    if (sessionSnapshot.stage != DeferredPackageSessionStage::playable
        || preparedModel == nullptr || !context.stageActivation(preparedModel))
        return false;
    sessionSnapshot.stage = DeferredPackageSessionStage::pendingActivation;
    sessionSnapshot.status = "Package activation is pending the next audio block boundary.";
    return true;
}

bool DeferredPackageSession::observeAudioCutover(const SamplerPlaybackContext& context)
{
    if (sessionSnapshot.stage != DeferredPackageSessionStage::pendingActivation)
        return false;
    const auto contextSnapshot = context.getSnapshot();
    if (!contextSnapshot.hasActiveActivation
        || contextSnapshot.activeRevision != expectedRevision
        || contextSnapshot.activePreparedBuildId != expectedPreparedBuildId)
        return false;
    sessionSnapshot.stage = DeferredPackageSessionStage::active;
    sessionSnapshot.active = true;
    sessionSnapshot.playable = true;
    sessionSnapshot.activeGeneration = sessionSnapshot.requestGeneration;
    sessionSnapshot.status = "Package activation is active on audio.";
    return true;
}

void DeferredPackageSession::cancel(SamplerPlaybackContext* context)
{
    if (context != nullptr)
        context->cancelPendingActivation();
    resetRequestState();
    locatorResolver = {};
    sessionSnapshot.stage = DeferredPackageSessionStage::cancelled;
    sessionSnapshot.metadataAccepted = false;
    sessionSnapshot.headReady = false;
    sessionSnapshot.playable = false;
    sessionSnapshot.active = sessionSnapshot.activeGeneration != 0;
    sessionSnapshot.status = "Deferred package request cancelled.";
}

void DeferredPackageSession::close(SamplerPlaybackContext* context)
{
    cancel(context);
    currentPlan = {};
    locatorResolver = {};
    sessionSnapshot.packagePath.clear();
    sessionSnapshot.packageId.clear();
    sessionSnapshot.status = "Package session closed.";
}

void DeferredPackageSession::fail(const std::string& category,
                                  const std::string& sourceId,
                                  const std::string& status,
                                  const bool retryable)
{
    preparedModel.reset();
    sessionSnapshot.stage = sessionSnapshot.activeGeneration == 0
        ? DeferredPackageSessionStage::failed : DeferredPackageSessionStage::degraded;
    sessionSnapshot.playable = false;
    sessionSnapshot.active = sessionSnapshot.activeGeneration != 0;
    sessionSnapshot.retryable = retryable;
    sessionSnapshot.failureCategory = category;
    sessionSnapshot.failureSourceId = sourceId;
    sessionSnapshot.status = status;
}

DeferredPackageStatusProjection projectDeferredPackageStatus(
    const DeferredPackageSessionSnapshot& snapshot)
{
    DeferredPackageStatusProjection projection;
    projection.headline = std::string("Package ") + toString(snapshot.stage);
    projection.detail = snapshot.status;
    if (snapshot.totalHeadCount != 0
        && snapshot.stage == DeferredPackageSessionStage::preparingHeads)
        projection.detail += " " + std::to_string(snapshot.readyHeadCount) + "/"
            + std::to_string(snapshot.totalHeadCount) + " heads ready.";
    if (!snapshot.failureSourceId.empty())
        projection.detail += " Source: " + snapshot.failureSourceId + ".";
    projection.showRetry = snapshot.retryable;
    return projection;
}

PackageSessionReadiness projectDeferredPackageReadiness(
    const DeferredPackageSessionSnapshot& snapshot) noexcept
{
    switch (snapshot.stage)
    {
        case DeferredPackageSessionStage::idle:
        case DeferredPackageSessionStage::locatorPending:
            return PackageSessionReadiness::playbackDeferred;
        case DeferredPackageSessionStage::metadataReady:
            return PackageSessionReadiness::metadataLoaded;
        case DeferredPackageSessionStage::openingSources:
            return PackageSessionReadiness::openingSources;
        case DeferredPackageSessionStage::preparingHeads:
            return PackageSessionReadiness::preparingHeads;
        case DeferredPackageSessionStage::buildingModel:
            return PackageSessionReadiness::buildingModel;
        case DeferredPackageSessionStage::playable:
            return PackageSessionReadiness::playable;
        case DeferredPackageSessionStage::pendingActivation:
            return PackageSessionReadiness::pendingActivation;
        case DeferredPackageSessionStage::active:
            return PackageSessionReadiness::active;
        case DeferredPackageSessionStage::degraded:
            return PackageSessionReadiness::degraded;
        case DeferredPackageSessionStage::failed:
            return PackageSessionReadiness::failed;
        case DeferredPackageSessionStage::cancelled:
            return PackageSessionReadiness::cancelled;
    }
    return PackageSessionReadiness::playbackDeferred;
}
} // namespace drs::engine
