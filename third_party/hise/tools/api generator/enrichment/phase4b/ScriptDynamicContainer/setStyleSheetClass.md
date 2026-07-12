ScriptDynamicContainer::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE
Sets CSS class selectors on this component. The component's own type class
(.scriptdynamiccontainer) is automatically prepended. Creates the stylesheet
properties ValueTree if it does not yet exist.
Pair with:
  setStyleSheetProperty -- set CSS variables
  setStyleSheetPseudoState -- set pseudo-state selectors
  setLocalLookAndFeel -- alternative styling for non-dyncomp children
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass()
