ScriptAudioWaveform::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback, replacing the default
onControl handler. Pass undefined to revert to default.

Anti-patterns:
  - Must be declared with inline function -- regular function references are rejected
  - Must have exactly 2 parameters (component, value) -- reports script error otherwise
  - Do NOT use when the script processor has a DspNetwork forwarding controls

Pair with:
  changed -- triggers the assigned callback

Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback()
    -> validates inline function with 2 parameters
    -> stores as customControlCallback
