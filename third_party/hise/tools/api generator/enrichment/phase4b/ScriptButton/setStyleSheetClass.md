ScriptButton::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE
Sets the CSS class selectors for this component. The component's own type class
(.scriptbutton) is automatically prepended. Creates ComponentStyleSheetProperties
value tree if it does not yet exist.

Pair with:
  setStyleSheetProperty -- set CSS variables on the component
  setStyleSheetPseudoState -- set pseudo-state selectors
  setLocalLookAndFeel -- attach LAF with CSS stylesheet support

Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass()
