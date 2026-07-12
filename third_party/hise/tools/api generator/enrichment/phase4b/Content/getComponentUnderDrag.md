Content::getComponentUnderDrag() -> String

Thread safety: SAFE -- iterates RebuildListeners (lock-free observer list) and returns a string.
Returns the component ID of the component currently being dragged. Returns an empty
string if no drag operation is active.

Dispatch/mechanics:
  Queries RebuildListeners via DragAction::Query
  Returns component ID string from the active drag source

Source:
  ScriptingApiContent.cpp  Content::getComponentUnderDrag()
    -> iterates RebuildListeners with DragAction::Query
