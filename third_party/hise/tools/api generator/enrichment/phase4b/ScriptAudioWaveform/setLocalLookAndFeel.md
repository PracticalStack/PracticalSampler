ScriptAudioWaveform::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a ScriptedLookAndFeel object to this component and all its children.
Pass undefined to clear. Propagates to all child components automatically.

Anti-patterns:
  - Passing a non-LAF object silently clears the local look and feel instead of
    reporting an error

Pair with:
  setStyleSheetClass -- for CSS-based styling with LAF
  setStyleSheetProperty -- for CSS variable injection

Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel()
    -> iterates children -> sets LAF on each
    -> if CSS mode: calls setStyleSheetClass({})
