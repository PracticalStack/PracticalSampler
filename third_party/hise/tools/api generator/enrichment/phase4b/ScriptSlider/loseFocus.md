ScriptSlider::loseFocus() -> undefined

Thread safety: UNSAFE
Requests focus loss by notifying registered z-level listeners.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  grabFocus -- explicit focus acquisition path

Source:
  ScriptingApiContent.cpp:2054  z-level listener focus notification path
