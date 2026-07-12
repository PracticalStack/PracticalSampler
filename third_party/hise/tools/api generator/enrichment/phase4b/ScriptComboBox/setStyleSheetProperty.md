ScriptComboBox::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from a stylesheet. The type
parameter determines value conversion: "color" (int to #AARRGGBB), "%" (0.5
to "50%"), "px", "em", "vh", "deg", "path" (Path to base64), "" (no conversion).
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setLocalLookAndFeel -- attach CSS-based LAF
Source:
  ScriptingApiContent.h  ScriptComponent::setStyleSheetProperty()
