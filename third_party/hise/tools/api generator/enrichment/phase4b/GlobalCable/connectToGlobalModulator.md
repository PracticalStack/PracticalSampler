GlobalCable::connectToGlobalModulator(String lfoId, Integer addToMod) -> undefined

Thread safety: UNSAFE -- performs processor lookups and modifies connection state
Connects the cable to a global modulator (LFO, envelope, or voice start modulator) inside a `GlobalModulatorContainer` as a source. When connected, the modulator's output is sent to the cable each processing block. Pass `addToMod=false` to disconnect.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.connectToGlobalModulator("GlobalLFO", true);
```
Dispatch/mechanics: Looks up the processor by name via `ProcessorHelpers::getFirstProcessorWithName()`, verifies its parent is a `GlobalModulatorContainer`, then calls `gc->connectToGlobalCable(modulator, cable, addToMod)`. Different modulator types (time-variant, voice-start, envelope) are handled automatically.
Pair with: `connectToMacroControl` (macro target alternative), `connectToModuleParameter` (module parameter target alternative)
Anti-patterns: The modulator must be inside a `GlobalModulatorContainer`. If its parent is not a `GlobalModulatorContainer`, the call silently does nothing.
Source:
  ScriptingApiObjects.cpp:9378  connectToGlobalModulator() -> ProcessorHelpers::getFirstProcessorWithName() -> GlobalModulatorContainer::connectToGlobalCable()
