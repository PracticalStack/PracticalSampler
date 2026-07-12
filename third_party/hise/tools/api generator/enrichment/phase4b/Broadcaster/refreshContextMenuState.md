Broadcaster::refreshContextMenuState() -> undefined

Thread safety: UNSAFE -- calls stateCallback.callSync(), string allocations for dynamic text
Refreshes cached tick/enabled/text state for all context menu items. Call explicitly when
external state changes should be reflected before the user opens the menu.
Silently no-ops if no ContextMenuListener is attached.
Dispatch/mechanics:
  Iterates attachedListeners for ContextMenuListener instances.
  Calls stateCallback with ('active', idx), ('enabled', idx), ('text', idx) per item.
  Silently no-ops if no ContextMenuListener attached.
Pair with:
  attachToContextMenu -- must be called first for this to have any effect
Anti-patterns:
  - No-op without prior attachToContextMenu -- no error or warning.
  - State also auto-refreshes after each menu selection.
Source:
  ScriptBroadcaster.cpp  refreshContextMenuState()
