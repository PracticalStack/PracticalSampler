ScriptDynamicContainer::getChildComponents() -> Array

Thread safety: UNSAFE
Returns an array of ScriptComponent references for all child components whose
parentComponent property is set to this container. Returns regular ScriptComponent
children, not dyncomp children created via setData(). DynComp children are managed
through ContainerChild references.
Source:
  ScriptingApiContent.cpp  ScriptComponent::getChildComponents()
    -> iterates Content children -> checks parentComponent match
