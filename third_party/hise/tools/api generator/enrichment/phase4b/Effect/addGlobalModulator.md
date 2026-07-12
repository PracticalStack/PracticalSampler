Effect::addGlobalModulator(Integer chainIndex, ScriptObject globalMod, String modName) -> ScriptObject

Thread safety: UNSAFE -- creates a GlobalTimeVariantModulator receiver and connects it to the global source (heap allocation, module tree modification).
Creates a GlobalTimeVariantModulator receiver in the specified modulator chain
and connects it to an existing global modulator. The globalMod must reference a
modulator inside a GlobalModulatorContainer. The receiver continuously tracks
the global modulator's output. Returns a Modulator handle, or undefined if
the connection fails.
Dispatch/mechanics:
  moduleHandler.addAndConnectToGlobalModulator(chain, globalMod, name, false)
    -> creates GlobalTimeVariantModulator receiver
    -> connects to source in GlobalModulatorContainer
Pair with:
  addStaticGlobalModulator -- static variant (snapshot at voice start, not continuous)
  addModulator -- add a local modulator instead
Anti-patterns:
  - Do NOT assume success -- silently returns undefined without error if
    globalMod is not a valid Modulator handle. Always check the return value.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::addGlobalModulator()
    -> moduleHandler.addAndConnectToGlobalModulator(chain, mod, name, false)
