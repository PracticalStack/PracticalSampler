#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>
#include <vector>

namespace drs::engine
{
enum class HiseFrontendTargetKind
{
    plugin,
    standalone
};

struct HiseFrontendExportProfile
{
    std::string name;
    HiseFrontendTargetKind targetKind;
    bool useFrontend = false;
    bool isStandaloneApp = false;
    bool frontendIsPlugin = false;
    bool isStandaloneFrontend = false;
    bool requiresAsioSdk = false;
    bool requiresVst3Sdk = false;
    std::string sourceTemplate;
    std::string summary;
};

struct EngineStatusSnapshot
{
    std::string mode;
    std::string integrationState;
    std::string detail;
    std::vector<std::string> nextSteps;
};

class EngineFacade
{
public:
    std::vector<HiseFrontendExportProfile> getFrontendExportProfiles() const;
    EngineStatusSnapshot getStatusSnapshot() const;
    RuntimeManifestLoadResult loadPhase1ReferenceInstrument() const;
};
} // namespace drs::engine
