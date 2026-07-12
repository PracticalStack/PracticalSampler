Content::refreshDragImage() -> Integer

Thread safety: SAFE -- iterates RebuildListeners (lock-free) and triggers repaint via DragAction::Repaint.
Triggers a repaint of the current drag image by notifying RebuildListeners via the
DragAction::Repaint action. Returns true (1) if a listener handled the repaint,
false (0) otherwise.

Pair with:
  getComponentUnderDrag -- query which component is being dragged

Source:
  ScriptingApiContent.cpp  Content::refreshDragImage()
    -> iterates RebuildListeners with DragAction::Repaint
