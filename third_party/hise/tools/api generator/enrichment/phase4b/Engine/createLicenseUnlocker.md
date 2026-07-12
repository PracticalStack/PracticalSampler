Engine::createLicenseUnlocker() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a reference to the script license manager for HISE's built-in RSA-based copy
protection. Provides methods for checking license status, registering product keys,
and managing trial/demo modes. Requires USE_COPY_PROTECTION preprocessor flag.
Source:
  ScriptingApi.cpp  Engine::createLicenseUnlocker()
    -> new ScriptUnlocker::RefObject
