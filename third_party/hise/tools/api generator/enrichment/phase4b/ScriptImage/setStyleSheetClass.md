ScriptImage::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE
Sets CSS class selectors for this component. The component's type class
(.scriptimage) is automatically prepended. Creates ComponentStyleSheetProperties
if needed.
Required setup:
  const var img = Content.addImage("MyImage", 0, 0);
  // Attach a CSS-enabled LAF first
  img.setLocalLookAndFeel(cssLaf);
Pair with:
  setLocalLookAndFeel -- must attach CSS LAF first
  setStyleSheetProperty -- set CSS variables
  setStyleSheetPseudoState -- set pseudo-states
Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass()
    -> prepends type class -> creates/updates ComponentStyleSheetProperties
