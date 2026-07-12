ScriptPanel::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE -- modifies CSS class selectors, creates ValueTree if needed
Sets CSS class selectors for this component. The component's type class
(.scriptpanel) is automatically prepended. Creates ComponentStyleSheetProperties
ValueTree if it does not exist.
Pair with:
  setStyleSheetProperty -- set CSS variables
  setStyleSheetPseudoState -- set CSS pseudo-states
  setLocalLookAndFeel -- attach a CSS-enabled LAF
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass()
