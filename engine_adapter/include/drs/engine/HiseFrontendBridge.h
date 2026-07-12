#pragma once

#include <string>

namespace drs::engine
{
struct HiseLinkedFrontendSnapshot
{
    bool linked = false;
    std::string pluginName;
    std::string manufacturer;
    int buildSubVersion = 0;
    bool useBackend = false;
    bool useFrontend = false;
    bool frontendIsPlugin = false;
    bool isStandaloneApp = false;
    bool isStandaloneFrontend = false;
};

HiseLinkedFrontendSnapshot getLinkedHiseFrontendSnapshot();
} // namespace drs::engine
