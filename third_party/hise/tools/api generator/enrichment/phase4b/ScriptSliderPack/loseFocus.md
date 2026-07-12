ScriptSliderPack::loseFocus() -> undefined

Thread safety: UNSAFE
Releases keyboard focus for this component.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  grabFocus -- acquires focus
  setKeyPressCallback -- observes focus-change events

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits ScriptComponent focus API
