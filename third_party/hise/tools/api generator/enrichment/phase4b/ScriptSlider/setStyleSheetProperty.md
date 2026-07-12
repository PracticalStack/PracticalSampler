ScriptSlider::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on the component and optionally converts value format by type.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  setStyleSheetClass/setStyleSheetPseudoState -- class and pseudo-state selectors controlling CSS evaluation

Source:
  ScriptingApiContent.cpp:2054  CSS variable conversion and storage path
