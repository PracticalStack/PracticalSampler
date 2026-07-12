# ScriptPanel -- Class Analysis

## Brief
Scriptable panel with custom paint routines, mouse/timer callbacks, drag-and-drop, popups, child panels, and Lottie animation.

## Purpose
ScriptPanel is the most versatile UI component in HISE, providing a blank canvas for custom drawing via paint routines that receive a Graphics object. It supports mouse interaction at configurable callback levels, periodic timer callbacks, file drop handling, sample preload notifications, popup menus, popup panel overlays, parent-child panel hierarchies, image loading, dragging (both internal UI drag and external file drag), undo-aware value changes, and Lottie animation playback. The `data` property provides a persistent DynamicObject for storing arbitrary per-panel state.

## Details

### Paint System

ScriptPanel uses a deferred paint architecture. Call `setPaintRoutine(function(g){...})` to register a paint function that receives a Graphics object. Call `repaint()` to schedule an asynchronous repaint -- the paint routine executes on the scripting thread via a low-priority job, not on the calling thread. The `repaint()` method is safe to call from any thread. Canvas resolution accounts for high-DPI settings (`Content.setUseHighResolutionForPanels`) and the global scale factor, capped at 2x.

Alternatively, `setImage()` bypasses the paint routine entirely and clips a region from a previously loaded image for filmstrip-style rendering.

### Mouse Callback System

The `allowCallbacks` property controls which mouse events are delivered to the callback set via `setMouseCallback`:

| Level | Events |
|-------|--------|
| No Callbacks | None |
| Context Menu | Popup menu only |
| Clicks Only | mouseDown, mouseUp, doubleClick |
| Clicks & Hover | Clicks + mouseEnter/Exit |
| Clicks, Hover & Dragging | Clicks + hover + drag |
| All Callbacks | All above + mouseMove |

The callback receives a JSON event object with positional data (`x`, `y`, `mouseDownX`, `mouseDownY`), state flags (`clicked`, `doubleClick`, `rightClick`, `mouseUp`, `drag`, `hover`), drag data (`dragX`, `dragY`, `insideDrag`, `isDragOnly`), modifier keys (`shiftDown`, `cmdDown`, `altDown`, `ctrlDown`), and popup results (`result`, `itemText`).

### Timer System

ScriptPanel inherits from `SuspendableTimer`. Call `setTimerCallback(function(){...})` to register a zero-parameter callback, then `startTimer(ms)` to begin. The callback fires on the message thread. Call `stopTimer()` to stop. The timer is automatically stopped on recompilation.

### Child Panel Hierarchy

`addChildPanel()` creates a new ScriptPanel as a child of this panel. Child panels are full ScriptPanel instances with their own paint routines, mouse callbacks, and timers. Use `getChildPanelList()`, `getParentPanel()`, and `removeFromParent()` to manage the hierarchy.

### Popup System

Two popup modes exist:
1. **Popup menu:** Set `popupMenuItems` (newline-separated) and the mouse callback receives `result`/`itemText` on selection.
2. **Panel popup:** Set `isPopupPanel` to true, configure with `setPopupData()`, and show/hide with `showAsPopup()`/`closeAsPopup()`. Use `setIsModalPopup(true)` for a modal overlay with dark background.

### Drag and Drop

- **Panel dragging:** Enable `allowDragging` and optionally constrain with `setDraggingBounds([x, y, w, h])`.
- **Internal drag:** `startInternalDrag(data)` initiates a drag within the HISE UI.
- **External file drag:** `startExternalFileDrag(files, moveOriginal, callback)` initiates an OS-level file drag.
- **File drop:** `setFileDropCallback(level, wildcard, callback)` handles files dropped onto the panel.

### Lottie Animation

Requires `HISE_INCLUDE_RLOTTIE`. Load a base64-encoded Lottie JSON with `setAnimation()`, control frames with `setAnimationFrame()`, and query state with `getAnimationData()` (returns `{active, currentFrame, numFrames, frameRate}`).

## obtainedVia
`Content.addPanel(name, x, y)`

## minimalObjectToken
pnl

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| data | {} | JSON | Persistent DynamicObject for storing arbitrary per-panel state | -- |

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `pnl.setMouseCallback(fn)` without setting allowCallbacks | Set `pnl.set("allowCallbacks", "Clicks Only")` before `setMouseCallback` | The allowCallbacks property defaults to "No Callbacks", so the mouse callback will never fire unless explicitly set to a callback level |
| Calling `pnl.startTimer(50)` without `setTimerCallback` | Call `setTimerCallback(fn)` first, then `startTimer(50)` | startTimer starts the timer but without a registered callback function, nothing happens |

## codeExample
```javascript
const var pnl = Content.addPanel("Panel1", 0, 0);
pnl.set("width", 200);
pnl.set("height", 100);
pnl.set("allowCallbacks", "Clicks & Hover");

pnl.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setColour(0xFFFFFFFF);
    g.drawRect([0, 0, this.getWidth(), this.getHeight()], 1.0);
});

pnl.setMouseCallback(function(event)
{
    if (event.clicked)
        this.repaint();
});
```

## Alternatives
- ScriptImage: for simple static image display without custom drawing
- ScriptFloatingTile: for embedding pre-built HISE widgets (keyboard, waveform, etc.)
- ScriptButton: for standard toggle/momentary button behavior
- Timer: for standalone periodic tasks not tied to a panel
- DisplayBuffer: data source for real-time audio visualization in a panel paint callback

## Related Preprocessors
`HISE_INCLUDE_RLOTTIE`, `HISE_SEND_PANEL_CHANGED_TO_PLUGIN_PARAMETER`

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ScriptPanel's callback setup methods (setMouseCallback, setTimerCallback, etc.) already use ADD_CALLBACK_DIAGNOSTIC for compile-time validation. The allowCallbacks/setMouseCallback mismatch is a runtime configuration issue, not a parse-time detectable error.
