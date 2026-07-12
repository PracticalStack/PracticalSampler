ScriptedViewport::setStyleSheetPseudoState(String pseudoState) -> undefined

Thread safety: UNSAFE
Sets CSS pseudo-state selectors on this component. Multiple states can be combined (e.g. ":hover:active"). Pass "" to clear all. Valid states: :first-child, :last-child, :root, :hover, :active, :focus, :disabled, :hidden, :checked. Automatically calls sendRepaintMessage() after setting.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setStyleSheetPseudoState(":hover");
Pair with: setStyleSheetClass (sets CSS classes), setStyleSheetProperty (sets CSS variables), setLocalLookAndFeel (attaches LAF with CSS)
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetPseudoState() -> getOrCreateStyleSheetProperties() -> sendRepaintMessage()
