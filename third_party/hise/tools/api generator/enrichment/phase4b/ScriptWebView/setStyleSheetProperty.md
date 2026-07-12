ScriptWebView::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from a stylesheet. The type
parameter determines value conversion: "path" (Path to base64), "color" (int
to #AARRGGBB), "%" (number to percentage), "px", "em", "vh", "deg", or ""
(no conversion).
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetPseudoState -- set pseudo-state selectors
  setLocalLookAndFeel -- required for CSS-based styling
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetProperty() (base class)
