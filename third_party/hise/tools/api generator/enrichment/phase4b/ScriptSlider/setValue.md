ScriptSlider::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets slider value, schedules async UI update, and notifies value listeners.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Dispatch/mechanics:
  value is written into component state and pushed through listener notification path
  UI refresh is asynchronous through wrapper/update dispatcher bridge

Pair with:
  getValue -- reads back current value
  changed -- explicitly trigger control callback flow after scripted updates
  setValueWithUndo -- same write intent with undo event generation

Anti-patterns:
  - Do NOT pass String values -- script error is reported.
  - Do NOT assume onInit assignments are restore-safe -- values set in onInit are not restored on recompile.

Source:
  ScriptingApiContent.cpp:2054  ScriptSlider::setValue() state write and listener notification
