ScriptPanel::setDraggingBounds(Array area) -> undefined

Thread safety: UNSAFE -- stores drag constraint rectangle
Sets the rectangular area [x, y, width, height] that constrains this panel's
drag movement when allowDragging is enabled.
Pair with:
  startInternalDrag -- initiate internal UI drag
Source:
  ScriptingApiContent.cpp  ScriptPanel::setDraggingBounds()
