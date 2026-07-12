ScriptedViewport::loseFocus() -> undefined

Thread safety: UNSAFE
Notifies all z-level listeners that the component wants to lose keyboard focus. Triggers wantsToLoseFocus() on all registered ZLevelListener instances.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.loseFocus();
Pair with: grabFocus (grabs keyboard focus)
Source:
  ScriptingApiContent.cpp  ScriptComponent::loseFocus() -> zLevelListeners[*]->wantsToLoseFocus()
