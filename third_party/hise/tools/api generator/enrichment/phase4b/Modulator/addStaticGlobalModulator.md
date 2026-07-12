Modulator::addStaticGlobalModulator(Integer chainIndex, ScriptObject timeVariantMod, String modName) -> ScriptObject

Thread safety: UNSAFE -- creates a static global receiver module via
ModuleHandler, coordinates with KillStateHandler, heap allocates the wrapper.
Creates a static time-variant global modulator receiver. Unlike addGlobalModulator,
the static variant samples the source modulator's value only at voice start,
rather than continuously tracking it. More CPU-efficient when per-block modulation
updates are not needed. The timeVariantMod must reference a modulator inside a
GlobalModulatorContainer.

Dispatch/mechanics:
  moduleHandler.addAndConnectToGlobalModulator(chain, timeVariantMod, modName, true)
    -> creates GlobalStaticTimeVariantModulator receiver
    -> samples source value at voice start only

Pair with:
  addGlobalModulator -- dynamic variant for continuous per-block tracking
  setIntensity -- set modulation depth on the returned receiver

Source:
  ScriptingApiObjects.cpp  addStaticGlobalModulator()
    -> moduleHandler.addAndConnectToGlobalModulator(chain, mod, name, true)
