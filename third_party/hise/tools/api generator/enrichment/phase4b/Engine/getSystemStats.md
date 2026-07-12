Engine::getSystemStats() -> JSON

Thread safety: UNSAFE -- allocates DynamicObject, constructs String properties
Returns JSON with 15 system properties: OperatingSystemName, OperatingSystem64Bit,
LogonName, FullUserName, ComputerName, UserLanguage, UserRegion, DisplayLanguage,
NumCpus, NumPhysicalCpus, CpuSpeedInMegahertz, CpuVendor, CpuModel,
MemorySizeInMegabytes, isDarkMode.
Source:
  ScriptingApi.cpp  Engine::getSystemStats()
    -> new DynamicObject with JUCE SystemStats calls
