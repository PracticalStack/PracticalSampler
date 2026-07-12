ScriptSlider::addToMacroControl(Integer macroIndex) -> undefined

Thread safety: UNSAFE
Assigns this slider to a macro controller slot by writing the connected macro index.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Anti-patterns:
  - Do NOT pass indices outside 0..7 -- mapping is only defined for macro slots 0 to 7.

Source:
  ScriptingApiContent.cpp:2054  ScriptSlider method block and property wiring
