ScriptSlider::changed() -> undefined

Thread safety: SAFE
Triggers the slider control callback and notifies registered value listeners.
If deferred callbacks are enabled, execution is queued instead of immediate.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  setControlCallback -- provides the callback function that changed triggers
  setValue/setValueNormalized -- update value then emit callback/listener notifications

Anti-patterns:
  - Do NOT call during onInit -- it logs and returns without normal callback behavior.
  - Do NOT assume code after changed always runs -- thrown callback errors abort continuation.

Source:
  ScriptingApiContent.cpp:2054  ScriptSlider callback and listener dispatch path
