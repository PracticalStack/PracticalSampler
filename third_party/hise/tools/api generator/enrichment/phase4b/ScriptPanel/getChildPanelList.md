ScriptPanel::getChildPanelList() -> Array

Thread safety: UNSAFE -- allocates Array
Returns an array of ScriptPanel references for all child panels created via
addChildPanel(). Separate from getChildComponents() which returns components
parented via the parentComponent property.
Pair with:
  addChildPanel -- create child panels
  getParentPanel -- navigate to parent
  removeFromParent -- detach a child
Source:
  ScriptingApiContent.cpp  ScriptPanel::getChildPanelList()
