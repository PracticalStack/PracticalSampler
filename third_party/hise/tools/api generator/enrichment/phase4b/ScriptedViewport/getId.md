ScriptedViewport::getId() -> String

Thread safety: WARNING -- String involvement, atomic ref-count operations
Returns the component's ID as a string (the variable name used when creating the component).
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var id = vp.getId();
Source:
  ScriptingApiContent.cpp  ScriptComponent::getId() -> getName().toString()
