ScriptButton::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a ScriptedLookAndFeel object to this button and all its children. Pass
undefined to clear it. For ScriptButton, the relevant LAF function is drawToggleButton.
A custom LAF takes priority over filmstrip rendering.

Dispatch/mechanics:
  Sets LAF on this component -> propagates to ALL child components
  If LAF uses CSS, automatically calls setStyleSheetClass({}) to initialize

Pair with:
  setStyleSheetClass / setStyleSheetProperty -- CSS variable system for LAF styling
  setStyleSheetPseudoState -- programmatic pseudo-state for CSS selectors

Anti-patterns:
  - Be aware that the LAF propagates to all child components automatically --
    may override children's individual look and feel

Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel()
    -> iterates child ScriptComponents, sets localLookAndFeel
    -> if CSS mode: setStyleSheetClass({})
