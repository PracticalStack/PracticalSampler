#include "drs/engine/HiseFrontendBridge.h"

#include "AppConfig.h"
#include "hi_core/BuildVersion.h"

namespace drs::engine
{
HiseLinkedFrontendSnapshot getLinkedHiseFrontendSnapshot()
{
    static_assert(USE_BACKEND == 0, "Linked frontend profile bridge must not enable the HISE backend.");
    static_assert(USE_FRONTEND == 1, "Linked frontend profile bridge must enable the HISE frontend profile.");
    static_assert(FRONTEND_IS_PLUGIN == 1, "Linked frontend profile bridge must model the plugin frontend target.");
    static_assert(IS_STANDALONE_APP == 0, "Linked frontend profile bridge must not model the standalone target.");
    static_assert(IS_STANDALONE_FRONTEND == 0, "Linked frontend profile bridge must stay in plugin mode.");

    return {
        true,
        JucePlugin_Name,
        JucePlugin_Manufacturer,
        BUILD_SUB_VERSION,
        USE_BACKEND != 0,
        USE_FRONTEND != 0,
        FRONTEND_IS_PLUGIN != 0,
        IS_STANDALONE_APP != 0,
        IS_STANDALONE_FRONTEND != 0
    };
}
} // namespace drs::engine
