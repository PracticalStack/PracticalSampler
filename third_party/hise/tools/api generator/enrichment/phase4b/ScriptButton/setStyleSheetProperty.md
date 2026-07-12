ScriptButton::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from a stylesheet. The type
parameter determines value-to-CSS conversion: "color" (int to #AARRGGBB),
"%" (0.5 to "50%"), "px", "em", "vh", "deg", "path" (Path to base64), "" (as-is).

Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetPseudoState -- set pseudo-state selectors

Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetProperty()
