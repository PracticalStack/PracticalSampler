ScriptPanel::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE -- propagates LAF to all child components
Attaches a scripted look and feel object to this component and all its children.
Pass undefined to clear. If the LAF uses CSS, automatically calls
setStyleSheetClass({}) to initialize the class selector.
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetProperty -- set CSS variables
  setStyleSheetPseudoState -- set CSS pseudo-states
Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel()
