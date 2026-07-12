#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>

namespace drs::engine
{
std::string getPhase1RuntimeRootPath();
std::string getPhase1ReferenceCorpusIndexPath();
std::string getPhase1ReferenceBaselinePath();
std::string getPhase1ReferenceProjectManifestPath();
std::string getPhase1ReferenceInstrumentManifestPath();

RuntimeProjectLoadResult loadRuntimeProjectManifest(const std::string& manifestPath);
RuntimeProjectLoadResult loadPhase1ReferenceProjectManifest();
RuntimeManifestLoadResult loadRuntimeInstrumentManifest(const std::string& manifestPath);
RuntimeManifestLoadResult loadPhase1ReferenceInstrumentManifest();

std::string serializeRuntimeProjectManifest(const RuntimeProjectModel& project, const std::string& manifestPath);
std::string serializeRuntimeInstrumentManifest(const RuntimeInstrumentModel& instrument, const std::string& manifestPath);
} // namespace drs::engine
