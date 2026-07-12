ScriptWebView::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE
Sets the CSS class selectors for this component. The component's own type class
(.scriptwebview) is automatically prepended.
Pair with:
  setStyleSheetProperty -- set CSS variables on the component
  setStyleSheetPseudoState -- set pseudo-state selectors
  setLocalLookAndFeel -- required for CSS-based styling
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass() (base class)
