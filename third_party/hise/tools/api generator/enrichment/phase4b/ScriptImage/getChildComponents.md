ScriptImage::getChildComponents() -> Array

Thread safety: UNSAFE
Returns an array of ScriptComponent references for all child components (components
whose parentComponent property is set to this component). Does not include the
component itself.
Source:
  ScriptingApiContent.cpp  ScriptComponent::getChildComponents()
    -> iterates all content components -> filters by parentComponent match
