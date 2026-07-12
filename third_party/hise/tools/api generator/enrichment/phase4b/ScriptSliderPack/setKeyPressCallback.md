ScriptSliderPack::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers key/focus event callback for consumed key presses.
Requires setConsumedKeyPresses first.
Callback signature: f(Object event)

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);
  spk.setConsumedKeyPresses("all");

Pair with:
  setConsumedKeyPresses -- defines event capture scope
  grabFocus/loseFocus -- trigger focus-change events

Anti-patterns:
  - Do NOT call this before setConsumedKeyPresses -- callback setup is incomplete.

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits typed ScriptComponent key callback registration
