#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>

namespace drs::engine
{
std::string getPhase1RuntimeRootPath();
std::string getPhase1ReferenceCorpusIndexPath();
std::string getPhase1ReferenceBenchmarkScenePath();
std::string getPhase1ReferenceBaselinePath();
std::string getPhase1ReferenceProjectManifestPath();
std::string getPhase1ReferenceInstrumentManifestPath();
std::string getPhase1ReferencePackageManifestPath();
std::string getPhase2RuntimeRootPath();
std::string getPhase2ReferenceProjectManifestPath();

RuntimeProjectLoadResult loadRuntimeProjectManifest(const std::string& manifestPath);
RuntimeProjectLoadResult loadPhase1ReferenceProjectManifest();
RuntimeProjectLoadResult loadPhase2ReferenceProjectManifest();
RuntimeManifestLoadResult loadRuntimeInstrumentManifest(const std::string& manifestPath);
RuntimeManifestLoadResult loadPhase1ReferenceInstrumentManifest();
RuntimeProjectValidationResult validateRuntimeProjectModel(const RuntimeProjectModel& project);
RuntimeProjectMigrationResult migrateRuntimeProjectToPhase2Authoring(const RuntimeProjectModel& project);

std::string serializeRuntimeProjectManifest(const RuntimeProjectModel& project, const std::string& manifestPath);
std::string serializeRuntimeInstrumentManifest(const RuntimeInstrumentModel& instrument, const std::string& manifestPath);
} // namespace drs::engine
