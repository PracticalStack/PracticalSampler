MacroHandler::setMacroDataFromObject(Array jsonData) -> undefined

Thread safety: UNSAFE -- clears and rebuilds all macro connections. Allocates Strings, modifies processor state, sends connection change notifications.
Clears all existing macro connections and rebuilds them from the provided JSON
array. Each element must follow the format returned by getMacroDataObject(),
with an additional optional `converter` property (String) for display formatting.
Individual connection notifications are suppressed during rebuild -- the update
callback fires once after all connections are restored.

Required setup:
  const var mh = Engine.createMacroHandler();
  var data = mh.getMacroDataObject();

Dispatch/mechanics:
  ScopedUpdateDelayer suppresses callbacks during rebuild.
  Clears all HISE_NUM_MACROS macro slots via sendMacroConnectionChangeMessageForAll(false).
  Iterates jsonData, calls setFromCallbackArg() per element:
    -> resolves Processor by name, resolves Attribute by parameter index or CustomAutomation ID
    -> reads Full and Active ranges via RangeHelpers::getDoubleRange()
    -> calls MacroControlData::addParameter() with range, converter, custom automation flag
  On ScopedUpdateDelayer destruction, fires a single coalesced update callback.

Pair with:
  getMacroDataObject -- read current connections before modifying (read-modify-write cycle)
  setUpdateCallback -- notified once after rebuild completes

Anti-patterns:
  - [BUG] Silently does nothing if jsonData is not an Array -- no error reported, no
    connections modified, no callback fired.
  - Do NOT pass a partial list expecting additive merge -- all existing connections are
    cleared first. Always include the full desired connection set.

Source:
  ScriptingApiObjects.cpp  ScriptedMacroHandler::setMacroDataFromObject()
    -> validates MacroIndex/Attribute/Processor properties (reportScriptError on missing)
    -> ScopedUpdateDelayer RAII guard
    -> sendMacroConnectionChangeMessageForAll(false) clears all
    -> setFromCallbackArg() per element -> MacroControlData::addParameter()
