ScriptSliderPack::setAllValues(NotUndefined value) -> undefined

Thread safety: UNSAFE
Writes slider values in bulk. Accepts scalar fill, Array, or Buffer input.

Dispatch/mechanics:
  Converts AudioData input and writes into active SliderPackData.
  Notification path depends on allValueChangeCausesCallback flag.

Pair with:
  setSliderAtIndex -- single index variant
  setAllValuesWithUndo -- undo-integrated bulk variant
  setAllValueChangeCausesCallback -- control callback fanout during imports

Anti-patterns:
  - Do NOT assume short Array/Buffer clears remaining sliders -- only provided indices are updated.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::setAllValues() bulk copy path
