ScriptSlider::setZLevel(String zLevel) -> undefined

Thread safety: UNSAFE
Sets component z-order (Back, Default, Front, AlwaysOnTop) and notifies listeners on change.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  grabFocus/loseFocus -- z-level listener ecosystem also handles focus notifications

Source:
  ScriptingApiContent.cpp:2054  z-level property update and listener notification path
