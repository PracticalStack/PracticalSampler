#include "drs/engine/Phase1Baseline.h"

#include "drs/engine/RuntimeLoader.h"

#include <json/json.hpp>

namespace drs::engine
{
namespace
{
using ordered_json = nlohmann::ordered_json;

ordered_json buildStaticExpectations(const RuntimeManifestLoadResult& referenceResult)
{
    ordered_json expectations;
    expectations["manifestBytes"] = referenceResult.metrics.manifestSizeBytes;
    expectations["sourceProjectResolved"] = referenceResult.metrics.sourceProjectResolved;
    expectations["compiledStreamAssetResolved"] = referenceResult.metrics.compiledStreamAssetResolved;
    expectations["macroCount"] = referenceResult.metrics.macroCount;
    expectations["articulationCount"] = referenceResult.metrics.articulationCount;
    expectations["groupCount"] = referenceResult.metrics.groupCount;
    expectations["zoneCount"] = referenceResult.metrics.zoneCount;
    expectations["referencedSampleCount"] = referenceResult.metrics.referencedSampleCount;
    expectations["totalPrefetchBytes"] = referenceResult.metrics.totalPrefetchBytes;
    return expectations;
}
} // namespace

std::string buildPhase1RuntimeBaselineReportJson(const RuntimeManifestLoadResult& coldResult,
                                                 const RuntimeManifestLoadResult& warmResult)
{
    ordered_json report;
    report["report"] = "drs.phase1.runtimeBaseline";
    report["manifestPath"] = coldResult.manifestPath;
    report["instrumentId"] = coldResult.instrument.instrumentId;
    report["displayName"] = coldResult.instrument.displayName;
    report["manifestBytes"] = coldResult.metrics.manifestSizeBytes;
    report["coldLoadMicros"] = coldResult.metrics.loadDurationMicros;
    report["warmLoadMicros"] = warmResult.metrics.loadDurationMicros;
    report["sourceProjectResolved"] = coldResult.metrics.sourceProjectResolved;
    report["compiledStreamAssetResolved"] = coldResult.metrics.compiledStreamAssetResolved;
    report["macroCount"] = coldResult.metrics.macroCount;
    report["articulationCount"] = coldResult.metrics.articulationCount;
    report["groupCount"] = coldResult.metrics.groupCount;
    report["zoneCount"] = coldResult.metrics.zoneCount;
    report["referencedSampleCount"] = coldResult.metrics.referencedSampleCount;
    report["totalPrefetchBytes"] = coldResult.metrics.totalPrefetchBytes;
    return report.dump(2) + "\n";
}

std::string buildPhase1CheckedInBaselineSnapshotJson(const RuntimeManifestLoadResult& coldResult,
                                                     const RuntimeManifestLoadResult& warmResult,
                                                     const std::string& capturedOnIsoDate)
{
    ordered_json snapshot;
    snapshot["schemaName"] = "drs.runtimeBaseline";
    snapshot["schemaVersion"] = 1;
    snapshot["baselineId"] = coldResult.instrument.instrumentId;
    snapshot["referenceManifestPath"] = "content/runtime/phase1/reference-corpus/tiny-open-instrument/tiny-open-instrument.drinst";
    snapshot["timingUnits"] = "microseconds";
    snapshot["staticExpectations"] = buildStaticExpectations(coldResult);

    ordered_json latestObserved;
    latestObserved["capturedOn"] = capturedOnIsoDate;
    latestObserved["coldLoadMicros"] = coldResult.metrics.loadDurationMicros;
    latestObserved["warmLoadMicros"] = warmResult.metrics.loadDurationMicros;
    snapshot["latestObserved"] = std::move(latestObserved);

    ordered_json driftPolicy;
    driftPolicy["allowedPositiveDriftMicros"] = 5000;
    driftPolicy["allowedNegativeDriftMicros"] = 5000;
    snapshot["driftPolicy"] = std::move(driftPolicy);

    snapshot["notes"] = {
        "This file is the checked-in Sprint 1 baseline snapshot for the tiny open instrument.",
        "Timing values are observational and may change as the runtime evolves.",
        "Static expectations should only change when the reference fixture contract changes intentionally."
    };

    return snapshot.dump(2) + "\n";
}
} // namespace drs::engine
