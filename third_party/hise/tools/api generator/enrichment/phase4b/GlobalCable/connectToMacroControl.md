GlobalCable::connectToMacroControl(Integer macroIndex, Integer macroIsTarget, Integer filterRepetitions) -> undefined

Thread safety: UNSAFE -- allocates target objects and modifies the cable target list
Connects the cable to a macro control so cable values are forwarded to the macro. The cable's normalised 0..1 value is scaled to 0..127 for the macro system. Pass `macroIndex` as -1 to remove all macro connections.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.connectToMacroControl(0, true, true);
```
Dispatch/mechanics: Removes any existing `MacroCableTarget` for the same index (or all if macroIndex == -1). If macroIndex >= 0, creates a new `MacroCableTarget` that scales values via `127.0 * jlimit(0.0, 1.0, v)` and calls `macroData->setValue()`. The `filterRepetitions` flag skips consecutive identical values.
Pair with: `connectToModuleParameter` (connect to module params instead), `connectToGlobalModulator` (connect modulator as source)
Anti-patterns: Passing `macroIsTarget=false` triggers a debug assertion and does nothing -- always pass `true`. The cable's local input range does not affect macro values; the raw normalised value is used.
Source:
  ScriptingApiObjects.cpp:9345  connectToMacroControl() -> removeTarget() -> new MacroCableTarget() -> Cable::addTarget()
  ScriptingApiObjects.cpp:9295  MacroCableTarget::sendValue() -> jlimit() -> macroData->setValue()
