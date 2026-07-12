ScriptedViewport::grabFocus() -> undefined

Thread safety: UNSAFE
Notifies z-level listeners that the component wants to grab keyboard focus. Only notifies the first listener (exclusive operation).
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.grabFocus();
Pair with: loseFocus (releases keyboard focus)
Source:
  ScriptingApiContent.cpp  ScriptComponent::grabFocus() -> zLevelListeners[0]->wantsToGrabFocus()
