Engine::isPlugin() -> Integer

Thread safety: SAFE -- returns compile-time constant
Returns true if running as VST/AU/AAX plugin, false if standalone.
Pair with:
  quit -- only works in standalone (IS_STANDALONE_APP)
Source:
  ScriptingApi.h  inline -> #if IS_STANDALONE_APP: false / else: true
