Effect::addModulator(Integer chainIndex, String typeName, String modName) -> ScriptObject

Thread safety: UNSAFE -- creates a new processor in the module tree (heap allocation, module tree modification via ModuleHandler.addModule).
Adds a new modulator of the specified type to one of the effect's internal
modulator chains. Returns a Modulator handle, or undefined if creation fails.
Reports a script error if chainIndex does not reference a valid modulator chain.
Dispatch/mechanics:
  moduleHandler.addModule(chain, typeName, modName)
    -> creates processor, adds to Chain at chainIndex
    -> wraps result in ScriptingModulator handle
Pair with:
  getModulatorChain -- inspect the chain before/after adding
  addGlobalModulator -- connect to a global modulator instead of creating a new one
Anti-patterns:
  - Do NOT assume success -- returns undefined without error if module creation
    fails internally (e.g., invalid type name). Always check the return value.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::addModulator()
    -> moduleHandler.addModule(chain, type, id, index)
