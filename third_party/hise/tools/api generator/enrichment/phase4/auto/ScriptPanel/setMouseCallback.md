Registers a callback for mouse events. The `allowCallbacks` property must be set to an appropriate level before the callback fires.

The callback receives an event object with these properties:

| Property | Description |
|----------|-------------|
| `x`, `y` | Current mouse position inside the panel |
| `mouseDownX`, `mouseDownY` | Mouse position where the current click or drag started |
| `clicked`, `doubleClick`, `rightClick`, `mouseUp`, `drag`, `hover` | Event-state flags for the current mouse action |
| `dragX`, `dragY` | Drag distance from the original mouse-down position |
| `shiftDown`, `cmdDown`, `altDown`, `ctrlDown` | Modifier-key state during the event |
| `result`, `itemText` | Popup menu result data after a popup menu selection |

> [!Warning:Enable Clicks and Hover for hover events] The `hover` flag requires `allowCallbacks` set to `"Clicks & Hover"` or above. Setting `"Clicks Only"` silently suppresses hover events without error.
