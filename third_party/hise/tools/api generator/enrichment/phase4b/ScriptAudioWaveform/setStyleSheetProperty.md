ScriptAudioWaveform::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from a stylesheet. The type
parameter controls value conversion: "color" (int to #AARRGGBB), "%" (0.5 to
"50%"), "px", "em", "vh", "deg", "path" (Path to base64), "" (no conversion).

Pair with:
  setLocalLookAndFeel -- must have a CSS-enabled LAF attached
  setStyleSheetClass -- to assign CSS class selectors

Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetProperty()
    -> creates ComponentStyleSheetProperties ValueTree if needed
    -> converts value by type -> stores as CSS variable
