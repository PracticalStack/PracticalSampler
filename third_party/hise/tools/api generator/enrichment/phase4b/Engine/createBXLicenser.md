Engine::createBXLicenser() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a BX Licenser object for copy protection using the proprietary BX SDK.
Requires HISE_INCLUDE_BX_LICENSER preprocessor flag at compile time.
Anti-patterns:
  - Do NOT call without HISE_INCLUDE_BX_LICENSER enabled -- throws script error at runtime
Source:
  ScriptingApi.cpp  Engine::createBXLicenser()
    -> #if HISE_INCLUDE_BX_LICENSER: new ScriptBXLicenser
    -> else: reportScriptError("BX Licenser is not enabled")
