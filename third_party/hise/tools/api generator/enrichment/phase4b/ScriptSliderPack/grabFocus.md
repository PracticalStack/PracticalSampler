ScriptSliderPack::grabFocus() -> undefined

Thread safety: UNSAFE
Requests keyboard focus for this component via z-level focus listeners.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  loseFocus -- releases focus
  setKeyPressCallback -- process consumed key and focus events

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits ScriptComponent focus API
