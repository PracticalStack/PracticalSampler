#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>

namespace drs::engine
{
std::string buildPhase1RuntimeBaselineReportJson(const RuntimeManifestLoadResult& coldResult,
                                                 const RuntimeManifestLoadResult& warmResult);

std::string buildPhase1CheckedInBaselineSnapshotJson(const RuntimeManifestLoadResult& coldResult,
                                                     const RuntimeManifestLoadResult& warmResult,
                                                     const std::string& capturedOnIsoDate);
} // namespace drs::engine
