Effect::addStaticGlobalModulator(Integer chainIndex, ScriptObject timeVariantMod, String modName) -> ScriptObject

Thread safety: UNSAFE -- creates a GlobalStaticTimeVariantModulator receiver and connects it (heap allocation, module tree modification).
Creates a GlobalStaticTimeVariantModulator receiver in the specified modulator
chain. Unlike addGlobalModulator which continuously tracks the source, the
static variant uses a constant value snapshot updated only at voice start.
The timeVariantMod must reference a modulator inside a GlobalModulatorContainer.
Returns a Modulator handle, or undefined if the connection fails.
Dispatch/mechanics:
  moduleHandler.addAndConnectToGlobalModulator(chain, mod, name, true)
    -> creates GlobalStaticTimeVariantModulator receiver
    -> connects to source in GlobalModulatorContainer
Pair with:
  addGlobalModulator -- continuous tracking variant
Anti-patterns:
  - Do NOT assume success -- silently returns undefined without error if
    timeVariantMod is not a valid Modulator handle. Always check the return value.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::addStaticGlobalModulator()
    -> moduleHandler.addAndConnectToGlobalModulator(chain, mod, name, true)
