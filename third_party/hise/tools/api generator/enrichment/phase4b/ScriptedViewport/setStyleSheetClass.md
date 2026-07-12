ScriptedViewport::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE
Sets CSS class selectors for this component. The component's own type class (lowercased, prefixed with ".") is automatically prepended. Creates ComponentStyleSheetProperties value tree if needed.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setStyleSheetClass(".active");
Pair with: setStyleSheetProperty (sets CSS variables), setStyleSheetPseudoState (sets pseudo-states), setLocalLookAndFeel (attaches LAF with CSS)
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass() -> getOrCreateStyleSheetProperties() -> setProperty("class", ...)
