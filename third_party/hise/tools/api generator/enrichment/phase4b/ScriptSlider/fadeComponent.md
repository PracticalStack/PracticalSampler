ScriptSlider::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Animates visibility changes with a fade over the requested duration.
No fade message is sent if visibility state already matches.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  showControl -- immediate visibility toggle without animation

Source:
  ScriptingApiContent.cpp:2054  ScriptComponent visibility update messaging
