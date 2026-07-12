ScriptPanel::grabFocus() -> undefined

Thread safety: UNSAFE -- notifies z-level listeners
Notifies z-level listeners that the component wants to grab keyboard focus.
Only notifies the first listener (exclusive operation).
Pair with:
  loseFocus -- release keyboard focus
Source:
  ScriptingApiContent.cpp  ScriptComponent::grabFocus()
