ScriptPanel::setMouseCallback(Function mouseFunction) -> undefined

Thread safety: UNSAFE -- registers mouse callback via WeakCallbackHolder
Registers a callback for mouse events. The allowCallbacks property MUST be set
to a level that enables the desired events before this call.
Callback signature: f(Object event)

| Property | Description |
|----------|-------------|
| `x`, `y` | Current mouse position inside the panel |
| `mouseDownX`, `mouseDownY` | Mouse position where the current click or drag started |
| `clicked`, `doubleClick`, `rightClick`, `mouseUp`, `drag`, `isDragOnly`, `insideDrag`, `hover` | Event-state flags for the current mouse action |
| `dragX`, `dragY` | Drag distance from the original mouse-down position |
| `shiftDown`, `cmdDown`, `altDown`, `ctrlDown` | Modifier-key state during the event |
| `result`, `itemText` | Popup menu result data after a popup menu selection |

Required setup:
  const var pnl = Content.addPanel("Panel1", 0, 0);
  pnl.set("allowCallbacks", "Clicks Only");
Anti-patterns:
  - Do NOT call setMouseCallback without setting allowCallbacks first -- defaults
    to "No Callbacks", callback silently never fires
  - Do NOT set allowCallbacks to "All Callbacks" unless you need continuous mouse
    move tracking -- fires on every mouse move, causing unnecessary repaints
Pair with:
  setPaintRoutine -- typically used together for visual feedback on mouse events
  setDraggingBounds -- constrain drag area
Source:
  ScriptingApiContent.cpp  ScriptPanel::setMouseCallback()
    -> stores WeakCallbackHolder(mouseRoutine) with 1 parameter
