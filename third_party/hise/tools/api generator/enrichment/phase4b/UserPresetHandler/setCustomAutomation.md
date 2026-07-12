UserPresetHandler::setCustomAutomation(Array automationData) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Defines the custom automation slot layout for host and MIDI parameter mapping.
Each slot becomes a DAW-visible plugin parameter (if allowHostAutomation=true)
and can be mapped to MIDI CC (if allowMidiAutomation=true). Connections route
values to module parameters, other automation slots (meta), or global cables.
Required setup:
  const var uph = Engine.createUserPresetHandler();
  uph.setUseCustomUserPresetModel(onLoad, onSave, false);
Dispatch/mechanics:
  Parses each JSON object into CustomAutomationData
    -> resolves ProcessorConnection (by processorId/parameterId)
    -> resolves MetaConnection (by automationId, must appear earlier in array)
    -> resolves CableConnection (by cableId, via GlobalRoutingManager)
    -> creates CustomAutomationParameter for DAW-visible slots
Anti-patterns:
  - Throws script error if setUseCustomUserPresetModel has not been called first
  - MetaConnection targets must appear earlier in the array than the slot
    referencing them -- forward references are not resolved
Source:
  ScriptExpansion.cpp  setCustomAutomation()
    -> CustomAutomationData constructor parses JSON per slot
    -> UserPresetHandler::setCustomAutomationData(newList)
    -> PluginParameterAudioProcessor registers DAW parameters
