Modulator::addGlobalModulator(Integer chainIndex, ScriptObject globalMod, String modName) -> ScriptObject

Thread safety: UNSAFE -- creates a global receiver module via ModuleHandler,
coordinates with KillStateHandler, heap allocates the wrapper.
Creates a time-variant global modulator receiver in one of this modulator's
internal chains and connects it to the specified source modulator. The globalMod
parameter must reference a modulator inside a GlobalModulatorContainer. Returns
a Modulator handle for the receiver, or undefined on failure.

Dispatch/mechanics:
  moduleHandler.addAndConnectToGlobalModulator(chain, globalMod, modName, false)
    -> creates GlobalTimeVariantModulator receiver
    -> connects to source modulator in GlobalModulatorContainer

Pair with:
  addStaticGlobalModulator -- CPU-efficient alternative for voice-start sources
  setIntensity -- set modulation depth on the returned receiver
  Synth.removeModulator -- remove the connection

Anti-patterns:
  - Do NOT use for velocity/note-number/random sources -- use
    addStaticGlobalModulator instead. Dynamic variant wastes CPU polling a value
    that never changes mid-note.
  - Do NOT add duplicate connections without tracking -- each call adds another
    modulator to the chain, stacking modulation depth.

Source:
  ScriptingApiObjects.cpp  addGlobalModulator()
    -> moduleHandler.addAndConnectToGlobalModulator(chain, globalMod, name, false)
