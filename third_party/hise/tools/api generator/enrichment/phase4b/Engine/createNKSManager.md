Engine::createNKSManager() -> ScriptObject

Thread safety: UNSAFE -- heap allocation (when SDK available)
Creates an NKS manager for Native Instruments hardware controller integration.
Requires HISE_INCLUDE_NKS_SDK preprocessor flag at compile time.
Anti-patterns:
  - Do NOT call without HISE_INCLUDE_NKS_SDK enabled -- throws script error
    "NKS support is not enabled" with no way to check at script level first
Source:
  ScriptingApi.cpp  Engine::createNKSManager()
    -> #if HISE_INCLUDE_NKS_SDK: new ScriptNKSManager
    -> else: reportScriptError("NKS support is not enabled")
