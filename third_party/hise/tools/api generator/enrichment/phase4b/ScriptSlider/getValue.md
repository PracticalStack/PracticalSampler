ScriptSlider::getValue() -> var

Thread safety: SAFE
Returns the current slider value.
Object-valued states use a read lock for access.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  setValue -- writes the current value
  changed -- emits callback/listener notifications after programmatic writes

Anti-patterns:
  - Do NOT treat String as a supported value type -- string component values are not supported.

Source:
  ScriptingApiContent.cpp:2054  ScriptSlider value read path
