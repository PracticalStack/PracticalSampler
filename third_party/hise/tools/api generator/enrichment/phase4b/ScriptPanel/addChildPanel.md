ScriptPanel::addChildPanel() -> ScriptPanel

Thread safety: UNSAFE -- allocates a new ScriptPanel, modifies child panel list
Creates a new ScriptPanel as a child of this panel and returns it. The child is a
full ScriptPanel instance with its own paint routine, mouse callbacks, timers, and
data object.
Dispatch/mechanics:
  Calls ScriptPanel constructor with parent pointer -> adds to childPanels array
  -> sets isChildPanel = true on the new panel
Pair with:
  getChildPanelList -- enumerate child panels
  getParentPanel -- navigate back to parent from child
  removeFromParent -- detach child from parent
Source:
  ScriptingApiContent.cpp:4220+  ScriptPanel::addChildPanel()
