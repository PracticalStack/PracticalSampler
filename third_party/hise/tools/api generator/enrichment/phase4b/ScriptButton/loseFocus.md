ScriptButton::loseFocus() -> undefined

Thread safety: UNSAFE
Notifies all z-level listeners that the component wants to lose keyboard focus.
Triggers the wantsToLoseFocus() callback on all registered ZLevelListener instances.

Pair with:
  grabFocus -- acquire keyboard focus

Source:
  ScriptingApiContent.cpp  ScriptComponent::loseFocus()
