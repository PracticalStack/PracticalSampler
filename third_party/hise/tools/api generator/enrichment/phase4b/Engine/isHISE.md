Engine::isHISE() -> Integer

Thread safety: SAFE -- returns compile-time constant
Returns true in HISE IDE (USE_BACKEND defined), false in compiled plugins.
Source:
  ScriptingApi.h  inline -> #if USE_BACKEND: true / else: false
