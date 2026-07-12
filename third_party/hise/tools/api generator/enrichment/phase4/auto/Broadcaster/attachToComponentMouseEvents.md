Registers the broadcaster as a source that fires whenever mouse events occur on the specified components. The `callbackLevel` string controls which events are captured:

| Level | Events |
|---|---|
| `"No Callbacks"` | Disabled |
| `"Context Menu"` | Right-click only |
| `"Clicks Only"` | Mouse clicks |
| `"Clicks & Hover"` | Clicks and mouse enter/exit |
| `"Clicks, Hover & Dragging"` | Clicks, hover, and drag |
| `"All Callbacks"` | All mouse events including movement |

The `event` argument is a JSON object identical to the one from `ScriptPanel.setMouseCallback()`. This attachment does not override the component's default mouse behaviour -- it is an additive callback, so existing click, drag, and hover handling continues to work.

Change detection is automatically disabled for mouse events, so every event is dispatched even if its properties match a previous event. Existing listeners are not called on attachment since mouse events have no initial state.

> [!Warning:Callback level must match exactly] The callback level string must match exactly, including spaces and the ampersand character. An invalid string produces the error "illegal callback level".
