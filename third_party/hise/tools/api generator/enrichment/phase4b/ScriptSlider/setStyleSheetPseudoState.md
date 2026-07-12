ScriptSlider::setStyleSheetPseudoState(String pseudoState) -> undefined

Thread safety: UNSAFE
Sets CSS pseudo-state flags for this component and triggers repaint.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  sendRepaintMessage -- async repaint path used for style updates

Source:
  ScriptingApiContent.cpp:2054  CSS pseudo-state setter and repaint trigger
