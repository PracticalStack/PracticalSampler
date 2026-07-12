ScriptDynamicContainer::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from a stylesheet. The type
parameter determines conversion: "path" (base64), "color" (#AARRGGBB), "%"
(percentage), "px", "em", "vh", "deg", "" (no conversion).
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetPseudoState -- set pseudo-state selectors
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetProperty()
