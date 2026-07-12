ScriptPanel::getChildComponents() -> Array

Thread safety: UNSAFE -- allocates Array
Returns an array of ScriptComponent references for all child components (those whose
parentComponent property points to this component). Does not include this component.
Pair with:
  getChildPanelList -- returns child panels created via addChildPanel() (different hierarchy)
Source:
  ScriptingApiContent.cpp  ScriptComponent::getChildComponents()
