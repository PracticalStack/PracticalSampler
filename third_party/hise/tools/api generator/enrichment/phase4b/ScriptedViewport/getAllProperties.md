ScriptedViewport::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of strings with all active (non-deactivated) property IDs. For ScriptedViewport, macroControl, min, and max are excluded.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var props = vp.getAllProperties();
Pair with: get (reads a property value), set (writes a property value)
Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties() -> iterates propertyIds, skips deactivated
