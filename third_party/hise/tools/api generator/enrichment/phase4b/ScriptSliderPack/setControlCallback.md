ScriptSliderPack::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline control callback for slider-pack edits.
Callback receives the edited index as value payload.
Callback signature: f(ScriptComponent component, var value)

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  setSliderAtIndex -- callback can fire from indexed writes
  setAllValues -- callback can fire from bulk writes
  setAllValueChangeCausesCallback -- control callback policy for script writes

Anti-patterns:
  - Do NOT pass non-inline callbacks or wrong arity -- callback must be inline with exactly 2 parameters.

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits typed ScriptComponent callback registration
