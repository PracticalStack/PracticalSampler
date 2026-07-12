Engine::getLorisManager() -> ScriptObject

Thread safety: UNSAFE -- creates new ScriptLorisManager wrapper each call
Returns a Loris spectral analysis manager reference. Requires HISE_INCLUDE_LORIS.
Returns undefined silently without error if Loris is not compiled in.
Source:
  ScriptingApi.cpp  Engine::getLorisManager()
    -> #if HISE_INCLUDE_LORIS: new ScriptLorisManager
    -> else: returns undefined
