Broadcaster::attachToModuleParameter(var moduleIds, var parameterIds, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires on module parameter changes. Broadcaster must have 3 args
(processorId, parameterId, value). Supports special IDs: "Bypassed", "Enabled", "Intensity".
Multiple modules must be same type. Auto-enables queue mode.
Dispatch/mechanics:
  Registers via Processor::AttributeListener (new dispatch) or OtherListener (legacy).
  Special IDs: 'Bypassed'/'Enabled' -> BypassListener, 'Intensity' -> Modulation::intensityBroadcaster.
  Auto-enables queue mode.
Pair with:
  addModuleParameterSyncer -- target that sets a module parameter from broadcast values
  addListener -- for custom parameter change handling
Anti-patterns:
  - moduleIds must be string IDs, not scripting object references.
  - Multiple modules must be same processor type.
  - "Intensity" only valid on Modulator processors.
  - Queue mode enabled as side effect.
Source:
  ScriptBroadcaster.cpp:4073  ModuleParameterListener constructor
