ScriptPanel::removeFromParent() -> Integer

Thread safety: UNSAFE -- modifies parent's child panel list
Removes this panel from its parent panel's child list. Returns 1 if successfully
removed, 0 if it has no parent or removal failed. Only applies to panels created
via addChildPanel().
Pair with:
  addChildPanel -- create child panels
  getParentPanel -- check parent before removing
  getChildPanelList -- verify removal
Source:
  ScriptingApiContent.cpp  ScriptPanel::removeFromParent()
