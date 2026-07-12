Engine::getMemoryUsage() -> Double

Thread safety: UNSAFE -- iterates expansion pools (potential lock acquisitions)
Returns total memory usage of all loaded samples in megabytes (MB).
Source:
  ScriptingApi.cpp  Engine::getMemoryUsage()
    -> sums getMemoryUsageForAllSamples() across all expansion pools / (1024*1024)
