ScriptWebView::setStyleSheetPseudoState(String pseudoState) -> undefined

Thread safety: UNSAFE
Sets CSS pseudo-state selectors on this component. Multiple states can be
combined (e.g. ":hover:active"). Pass "" to clear all pseudo-states.
Automatically calls sendRepaintMessage() after setting.
Valid states: :first-child, :last-child, :root, :hover, :active, :focus,
:disabled, :hidden, :checked
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetProperty -- set CSS variables
  setLocalLookAndFeel -- required for CSS-based styling
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetPseudoState() (base class)
