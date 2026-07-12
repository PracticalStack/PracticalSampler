ScriptTable::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable value on this component with optional unit or type conversion.

Pair with:
  setStyleSheetClass -- class selectors consume CSS variables
  setStyleSheetPseudoState -- pseudo-state variants can use same variables

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> CSS variable conversion and storage
