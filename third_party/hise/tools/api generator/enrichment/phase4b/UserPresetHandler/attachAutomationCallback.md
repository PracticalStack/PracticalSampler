UserPresetHandler::attachAutomationCallback(String automationId, Function updateCallback, Number isSynchronous) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Attaches a script callback to a custom automation slot. Fires when the
automation value changes from host, MIDI CC, or script. Pass a non-function
as updateCallback to remove the callback for the given automation ID.
Callback signature: f(int automationIndex, double newValue)
Required setup:
  const var uph = Engine.createUserPresetHandler();
  uph.setUseCustomUserPresetModel(onLoad, onSave, false);
  uph.setCustomAutomation([{"ID": "Volume", "min": 0.0, "max": 1.0, "connections": []}]);
Dispatch/mechanics:
  Creates AttachedCallback -> registers with CustomAutomationData dispatch system
  SyncNotification: callback runs on audio thread via customUpdateCallback holder
  AsyncNotification: callback runs on UI thread via customAsyncUpdateCallback holder
  Backend checks audio-thread safety for sync callbacks at registration time
Pair with:
  clearAttachedCallbacks -- remove all attached callbacks at once
  setCustomAutomation -- must register automation slots before attaching callbacks
  setAutomationValue -- programmatically drive values that trigger this callback
Anti-patterns:
  - Do NOT use a regular function with SyncNotification -- must be an inline
    function for audio-thread safety. Backend reports error; exported plugin
    silently registers but may cause glitches.
Source:
  ScriptExpansion.cpp:310  AttachedCallback constructor
    -> cData->dispatcher.addValueListener(this)
    -> dispatches via customUpdateCallback (sync) or customAsyncUpdateCallback (async)
