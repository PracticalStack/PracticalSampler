ScriptedViewport::getChildComponents() -> Array

Thread safety: UNSAFE
Returns an array of ScriptComponent references for all child components (those whose parentComponent is set to this component). Does not include the component itself.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var children = vp.getChildComponents();
Source:
  ScriptingApiContent.cpp  ScriptComponent::getChildComponents() -> iterates content children, checks parentComponent
