ScriptTable::setStyleSheetPseudoState(String pseudoState) -> undefined

Thread safety: UNSAFE
Sets CSS pseudo-state flags for this component and triggers repaint.

Pair with:
  setStyleSheetClass -- class and pseudo-state selectors are evaluated together
  setStyleSheetProperty -- pseudo-state rules can consume component CSS variables

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> pseudo-state setter and repaint
