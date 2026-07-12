ScriptImage::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a scripted look and feel object to this component and all children.
Pass undefined to clear. Note: ScriptImage has no LAF drawing functions --
CSS rendering via StyleSheetLookAndFeel can intercept via drawImageOnComponent().
Dispatch/mechanics:
  Validates ScriptedLookAndFeel instance -> propagates to all child components
  If CSS mode: calls setStyleSheetClass({}) and initializes colour properties
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetProperty -- set CSS variables
Anti-patterns:
  - Propagates to ALL child components automatically -- may override children's LAF
Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel()
    -> iterates child components -> sets localLookAndFeel on each
    -> if CSS: initializes StyleSheetProperties
