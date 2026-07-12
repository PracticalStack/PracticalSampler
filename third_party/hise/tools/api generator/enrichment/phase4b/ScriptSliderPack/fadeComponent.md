ScriptSliderPack::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Animates visibility changes through the global UI animator.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  showControl -- immediate visibility toggle without animation

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits ScriptComponent visibility API
