#pragma once

#include "drs/engine/PerformancePublishContract.h"
#include "drs/engine/PreparedPlayback.h"

namespace drs::engine
{
struct PerformancePublishPreparationResult
{
    bool completeProject = false;
    bool activationEligible = false;
    PerformancePublishResult publishResult;
    std::vector<PlaybackSnapshotFinding> findings;
};

// Performs the all-or-nothing, immutable completion check for Performance
// Publish. This function only inspects worker-produced values; it performs no
// filesystem access, decoding, cache mutation, or document reads.
PerformancePublishPreparationResult validatePerformancePublishPreparation(
    const PerformancePublishRequestIdentity& identity,
    const PlaybackSnapshotBuildResult& snapshotResult,
    const PreparedPlaybackBuildResult& preparedResult);

// The exact Preview-reuse path receives worker-produced objects that are already
// retained behind const activation-payload ownership. It verifies their stored
// identities and complete topology without recomputing the large JSON digests.
PerformancePublishPreparationResult validateExactPreviewReuseForPerformance(
    const PerformancePublishRequestIdentity& identity,
    const PlaybackSnapshotBuildResult& snapshotResult,
    const PreparedPlaybackBuildResult& preparedResult);
} // namespace drs::engine
