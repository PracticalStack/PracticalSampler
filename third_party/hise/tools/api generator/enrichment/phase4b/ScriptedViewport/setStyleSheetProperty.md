ScriptedViewport::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on this component queryable from a stylesheet. The type parameter determines value-to-CSS conversion: "path" (base64), "color" (#AARRGGBB), "%" (percentage), "px" (pixels), "em", "vh", "deg", "" (no conversion).
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setStyleSheetProperty("bg", Colours.red, "color");
Pair with: setStyleSheetClass (sets CSS classes), setStyleSheetPseudoState (sets pseudo-states), setLocalLookAndFeel (attaches LAF with CSS)
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetProperty() -> getOrCreateStyleSheetProperties() -> convertAndStore()
