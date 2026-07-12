ScriptPanel::getParentPanel() -> ScriptPanel

Thread safety: SAFE
Returns the parent ScriptPanel if this panel was created via addChildPanel(),
or undefined if this is a top-level panel. Specific to the child panel hierarchy,
not the parentComponent property system.
Pair with:
  addChildPanel -- create child panels
  getChildPanelList -- list siblings from parent
  removeFromParent -- detach from parent
Source:
  ScriptingApiContent.cpp  ScriptPanel::getParentPanel()
