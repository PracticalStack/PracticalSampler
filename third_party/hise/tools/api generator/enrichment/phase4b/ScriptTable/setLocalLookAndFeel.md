ScriptTable::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Assigns a local scripted look-and-feel object to this component and its children.

Pair with:
  setStyleSheetClass -- combine local LAF draw callbacks with CSS class selection
  setStyleSheetProperty -- set runtime CSS variable values
  setStyleSheetPseudoState -- toggle pseudo-state selectors

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> local LAF assignment path
