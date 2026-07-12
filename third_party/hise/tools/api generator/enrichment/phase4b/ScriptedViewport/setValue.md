ScriptedViewport::setValue(NotUndefined newValue) -> undefined

Thread safety: WARNING -- Safe in list/viewport mode; allocates UndoableTableSelection in table MultiColumnMode
Sets the component's value. In table mode with MultiColumnMode, passing [column, row] triggers UndoableTableSelection (respects useUndoManager property). In list mode, pass integer row index. Always calls base ScriptComponent::setValue() for thread-safe storage, UI update, and linked component propagation.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setValue(0);
Dispatch/mechanics: In table MultiColumnMode with 2-element array: creates UndoableTableSelection, performs via UndoManager if useUndoManager is set, otherwise performs directly. Always calls ScriptComponent::setValue() which acquires SimpleReadWriteLock write lock, stores value, sends async UI update.
Pair with: getValue (retrieves value), setValueWithUndo (explicit undo support), setTableCallback (receives SetValue events in table mode)
Anti-patterns: Do NOT pass a String value -- reports a script error. Value set during onInit will NOT be restored after recompilation. In table MultiColumnMode, same-cell selection may be skipped.
Source:
  ScriptingApiContent.cpp:5483  ScriptedViewport::setValue() -> UndoableTableSelection -> ScriptComponent::setValue()
