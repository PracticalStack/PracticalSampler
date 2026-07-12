ScriptAudioWaveform::changed() -> undefined

Thread safety: SAFE
Triggers the control callback (custom or default onControl). Also notifies
any registered value listeners.

Anti-patterns:
  - Do NOT call during onInit -- logs a console message and returns without executing
  - If the callback throws an error, further script execution after changed() is aborted

Pair with:
  setControlCallback -- to set a custom callback handler
  getValue -- to read the value before triggering

Source:
  ScriptingApiContent.cpp  ScriptComponent::changed()
    -> invokes controlCallback or default onControl
    -> notifies value listeners
