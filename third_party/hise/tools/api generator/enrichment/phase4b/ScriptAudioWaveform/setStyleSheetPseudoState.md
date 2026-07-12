ScriptAudioWaveform::setStyleSheetPseudoState(String pseudoState) -> undefined

Thread safety: UNSAFE
Sets CSS pseudo-state selectors. Supports: ":hover", ":active", ":focus",
":disabled", ":hidden", ":checked", ":first-child", ":last-child", ":root".
Combine multiple (e.g. ":hover:active"). Pass "" to clear all.
Automatically triggers a repaint.

Pair with:
  setStyleSheetClass -- for class selectors
  setStyleSheetProperty -- for CSS variables

Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetPseudoState()
    -> parses pseudo-state bitmask -> sendRepaintMessage()
