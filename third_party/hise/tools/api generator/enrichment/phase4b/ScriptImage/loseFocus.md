ScriptImage::loseFocus() -> undefined

Thread safety: UNSAFE
Notifies all z-level listeners that the component wants to lose keyboard focus.
Pair with:
  grabFocus -- grab keyboard focus
Source:
  ScriptingApiContent.cpp  ScriptComponent::loseFocus()
    -> notifies all ZLevelListeners via wantsToLoseFocus()
