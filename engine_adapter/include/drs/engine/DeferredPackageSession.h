#pragma once

#include "drs/engine/PerformancePackage.h"
#include "drs/engine/SamplerPlaybackContext.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace drs::engine
{
enum class DeferredPackageSessionStage
{
    idle,
    locatorPending,
    metadataReady,
    openingSources,
    preparingHeads,
    buildingModel,
    playable,
    pendingActivation,
    active,
    degraded,
    failed,
    cancelled
};

struct DeferredPackageSessionSnapshot
{
    DeferredPackageSessionStage stage = DeferredPackageSessionStage::idle;
    std::uint64_t requestGeneration = 0;
    std::uint64_t activeGeneration = 0;
    std::string packagePath;
    std::string packageId;
    std::size_t readyHeadCount = 0;
    std::size_t totalHeadCount = 0;
    bool metadataAccepted = false;
    bool headReady = false;
    bool playable = false;
    bool active = false;
    bool retryable = false;
    std::string failureCategory;
    std::string failureSourceId;
    std::string status;
};

struct DeferredPackageSessionPlan
{
    std::string packagePath;
    std::shared_ptr<const PackageV2OpenResult> package;
    std::vector<std::shared_ptr<PackagePagedSampleDataSource>> sources;
    std::function<SamplerRenderModelPtr()> buildRenderModel;
};

struct DeferredPackageStatusProjection
{
    std::string headline;
    std::string detail;
    bool showRetry = false;
};

class DeferredPackageSession final
{
public:
    bool begin(DeferredPackageSessionPlan plan);
    bool requestLocatorRestore(
        std::string packagePath,
        std::function<DeferredPackageSessionPlan(const std::string&)> resolver);
    bool serviceNextWorkerStep();
    bool stagePlayableActivation(SamplerPlaybackContext& context);
    bool observeAudioCutover(const SamplerPlaybackContext& context);
    void cancel(SamplerPlaybackContext* context = nullptr);
    void close(SamplerPlaybackContext* context = nullptr);
    DeferredPackageSessionSnapshot snapshot() const { return sessionSnapshot; }
    SamplerRenderModelPtr playableModel() const { return preparedModel; }

private:
    void fail(const std::string& category, const std::string& sourceId,
              const std::string& status, bool retryable);
    void resetRequestState();

    DeferredPackageSessionPlan currentPlan;
    DeferredPackageSessionSnapshot sessionSnapshot;
    SamplerRenderModelPtr preparedModel;
    std::size_t nextHeadIndex = 0;
    std::uint64_t nextRequestGeneration = 1;
    std::size_t expectedRevision = 0;
    std::uint64_t expectedPreparedBuildId = 0;
    std::function<DeferredPackageSessionPlan(const std::string&)> locatorResolver;
};

const char* toString(DeferredPackageSessionStage stage) noexcept;
DeferredPackageStatusProjection projectDeferredPackageStatus(
    const DeferredPackageSessionSnapshot& snapshot);
PackageSessionReadiness projectDeferredPackageReadiness(
    const DeferredPackageSessionSnapshot& snapshot) noexcept;
} // namespace drs::engine
