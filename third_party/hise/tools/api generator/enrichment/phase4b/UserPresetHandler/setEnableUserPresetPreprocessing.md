UserPresetHandler::setEnableUserPresetPreprocessing(Integer processBeforeLoading, Integer shouldUnpackComplexData) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Configures preprocessing mode for user preset loading. When enabled, the
pre-callback receives a JSON object instead of a ScriptFile:

  { "version": "1.0.0", "Content": [ ... ], "Modules": { ... },
    "MidiAutomation": { ... }, "MPEData": { ... } }

The JSON can be modified in-place before load.
When shouldUnpackComplexData=true, JSON-prefixed strings and Base64 data
properties are decoded into native objects.
Dispatch/mechanics:
  Without preprocessing: pre-callback receives ScriptFile, ValueTree passes through
  With preprocessing: ValueTree -> convertToJson() -> pre-callback modifies JSON
    -> applyJSON() converts back to ValueTree for load
Pair with:
  setPreCallback -- preprocessing has no effect without a pre-callback
  isOldVersion -- check preset version inside the pre-callback
Anti-patterns:
  - Enabling preprocessing without a pre-callback adds unnecessary JSON conversion
    overhead on every preset load with no benefit
Source:
  ScriptExpansion.cpp  setEnableUserPresetPreprocessing()
    -> sets enablePreprocessing and shouldUnpackComplexData flags
    -> convertToJson() / applyJSON() called during prePresetLoad
