ScriptPanel::setMouseCursor(var pathOrName, Integer colour, Array hitPoint) -> undefined

Thread safety: UNSAFE -- updates cursor via LambdaBroadcaster
Sets the mouse cursor for this panel. Accepts a Path object for a custom cursor
(with colour tint and hitPoint as normalized [x, y] 0-1 coordinates) or a string
for a standard JUCE cursor type.

| Cursor Name | Description |
|-------------|-------------|
| `ParentCursor` | Inherit the cursor from the parent component |
| `NoCursor` | Hide the cursor |
| `NormalCursor` | Standard arrow cursor |
| `WaitCursor` | Busy / wait cursor |
| `IBeamCursor` | Text-editing cursor |
| `CrosshairCursor` | Crosshair cursor |
| `CopyingCursor` | Copy / duplicate cursor |
| `PointingHandCursor` | Link / button hand cursor |
| `DraggingHandCursor` | Dragging hand cursor |
| `LeftRightResizeCursor` | Horizontal resize cursor |
| `UpDownResizeCursor` | Vertical resize cursor |
| `UpDownLeftRightResizeCursor` | Multi-direction resize cursor |
| `TopEdgeResizeCursor` | Top-edge resize cursor |
| `BottomEdgeResizeCursor` | Bottom-edge resize cursor |
| `LeftEdgeResizeCursor` | Left-edge resize cursor |
| `RightEdgeResizeCursor` | Right-edge resize cursor |
| `TopLeftCornerResizeCursor` | Top-left corner resize cursor |
| `TopRightCornerResizeCursor` | Top-right corner resize cursor |
| `BottomLeftCornerResizeCursor` | Bottom-left corner resize cursor |
| `BottomRightCornerResizeCursor` | Bottom-right corner resize cursor |

Source:
  ScriptingApiContent.cpp  ScriptPanel::setMouseCursor()
    -> ApiHelpers::getMouseCursorNames() for string lookup
