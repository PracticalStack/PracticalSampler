#include "AppConfig.h"
#include "hi_frontend/hi_frontend.h"

namespace drs::engine
{
const char* getHiseFrontendPluginProbeName()
{
    static_assert(USE_BACKEND == 0, "Plugin frontend probe must not enable the HISE backend.");
    static_assert(USE_FRONTEND == 1, "Plugin frontend probe must enable the HISE frontend.");
    static_assert(FRONTEND_IS_PLUGIN == 1, "Plugin frontend probe must model the plugin frontend target.");
    static_assert(IS_STANDALONE_APP == 0, "Plugin frontend probe must not model the standalone target.");
    static_assert(IS_STANDALONE_FRONTEND == 0, "Plugin frontend probe must stay in plugin mode.");

    return JucePlugin_Name;
}
} // namespace drs::engine
