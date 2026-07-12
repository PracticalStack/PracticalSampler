ScriptComboBox::setStyleSheetPseudoState(String pseudoState) -> undefined

Thread safety: UNSAFE
Sets CSS pseudo-state selectors. Multiple states can be combined (e.g.
":hover:active"). Pass "" to clear all. Automatically calls sendRepaintMessage().
Valid states: :first-child, :last-child, :root, :hover, :active, :focus,
:disabled, :hidden, :checked.
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetProperty -- set CSS variables
Source:
  ScriptingApiContent.h  ScriptComponent::setStyleSheetPseudoState()
