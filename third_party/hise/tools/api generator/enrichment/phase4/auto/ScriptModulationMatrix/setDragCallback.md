Registers a callback that fires during drag-and-drop interaction with modulation connections. The callback receives the source ID, target ID, and an action string. The action values are:

- `"DragStart"` - a modulation source drag has begun (targetId is empty)
- `"DragEnd"` - the drag was cancelled or dropped on an invalid target (both IDs are empty)
- `"Drop"` - the source was dropped on a valid target, creating a connection
- `"Hover"` - the dragged source is over a valid target knob
- `"DisabledHover"` - the dragged source is over an invalid or already-connected target

The drag can be initiated from a ModulationMatrixController FloatingTile or programmatically via `ScriptPanel.startInternalDrag()` when the drag data is connected to the modulation system.
