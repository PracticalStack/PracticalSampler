ScriptSlider::grabFocus() -> undefined

Thread safety: UNSAFE
Requests keyboard focus by notifying the first registered z-level listener.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  loseFocus -- explicit focus release path

Source:
  ScriptingApiContent.cpp:2054  z-level listener focus notification path
