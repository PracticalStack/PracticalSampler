ScriptImage::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from stylesheets. The type parameter
determines value-to-CSS conversion: "color" (int to #AARRGGBB), "%" (0.5 to "50%"),
"px", "em", "vh", "deg", "path" (Path to base64), "" (no conversion).
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setLocalLookAndFeel -- must attach CSS LAF first
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetProperty()
    -> converts value based on type -> stores in ComponentStyleSheetProperties
