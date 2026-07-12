MacroHandler::getMacroDataObject() -> Array

Thread safety: UNSAFE -- allocates Array and DynamicObject per connection; constructs Strings for processor IDs and parameter names.
Returns an array of JSON objects representing all active macro-to-parameter
connections across all macro slots. Each element has this shape:

  { "MacroIndex": 0, "Processor": "Simple Gain1", "Attribute": "Gain",
    "CustomAutomation": false,
    "FullStart": 0.0, "FullEnd": 1.0, "Start": 0.0, "End": 1.0,
    "Interval": 0.01, "Skew": 1.0, "Inverted": false }

Required setup:
  const var mh = Engine.createMacroHandler();

Dispatch/mechanics:
  Iterates HISE_NUM_MACROS (default 8, max 64) macro slots on the master chain.
  For each slot, iterates MacroControlledParameterData entries, builds a DynamicObject
  per connection via getCallbackArg(), and pushes it to the result Array.

Pair with:
  setMacroDataFromObject -- write modified connections back (read-modify-write cycle)
  setUpdateCallback -- watch for changes instead of polling

Anti-patterns:
  - Do NOT modify objects in the returned array and expect changes to apply -- the array
    is a snapshot. Call setMacroDataFromObject(modifiedArray) to write back.
  - [BUG] When a macro slot has multiple connected parameters, the range properties on
    each returned object reflect the LAST parameter in the slot (internal iteration
    overwrites a single object). Be aware of this when reading per-connection ranges.

Source:
  ScriptingApiObjects.cpp:9784  ScriptedMacroHandler::getMacroDataObject()
    -> iterates macroChain->getMacroControlData(i)->getParameter(j)
    -> getCallbackArg() builds DynamicObject with MacroIds + RangeHelpers::storeDoubleRange()
