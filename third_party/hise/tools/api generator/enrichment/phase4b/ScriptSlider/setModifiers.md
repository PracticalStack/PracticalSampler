ScriptSlider::setModifiers(String action, IndexOrArray modifiers) -> undefined

Thread safety: UNSAFE
Stores modifier mappings for slider interaction actions.
Use action and flag constants from createModifiers().

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  const var mods = sl.createModifiers();

Dispatch/mechanics:
  action key and modifier payload are stored in the internal modObject dynamic property map
  slider core resolves flags at interaction time via modifier-action matching helpers

Pair with:
  createModifiers -- provides canonical action keys and modifier constants

Anti-patterns:
  - Do NOT use unknown action keys -- unmapped keys do not trigger runtime actions.

Source:
  ScriptingApiContent.cpp:2054  setModifiers dynamic object write path
  MacroControlledComponents.cpp:1  SliderWithShiftTextBox modifier action resolver
