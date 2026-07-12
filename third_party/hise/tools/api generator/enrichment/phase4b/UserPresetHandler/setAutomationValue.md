UserPresetHandler::setAutomationValue(Integer automationIndex, Double newValue) -> Integer

Thread safety: SAFE -- lock-free path: sanitize, clamp, snap, dispatch through connections
Sets the value of a custom automation slot by index. Value is sanitized,
clamped to range, and snapped to step size. Dispatches synchronously through
all connections (processor, meta, cable). Returns true if the custom data model
is active and the index is valid.
Required setup:
  const var uph = Engine.createUserPresetHandler();
  var idx = uph.getAutomationIndex("Volume");
Dispatch/mechanics:
  CustomAutomationData::call(newValue, sendNotificationSync)
    -> sanitize + clamp to range + snap to stepSize
    -> iterates connectionList: ProcessorConnection, MetaConnection, CableConnection
    -> dispatches through dispatch system
Pair with:
  getAutomationIndex -- convert string ID to index
  sendParameterGesture -- wrap in gesture pairs for DAW automation recording
Anti-patterns:
  - Returns false silently if custom data model is not active or index is out of
    range. No error thrown -- check the return value.
Source:
  ScriptExpansion.cpp  setAutomationValue()
    -> CustomAutomationData::call(newValue, sendNotificationSync)
