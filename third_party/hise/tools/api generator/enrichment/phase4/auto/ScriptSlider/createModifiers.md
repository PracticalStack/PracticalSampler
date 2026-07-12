Creates a modifiers object containing action IDs and modifier flags for `setModifiers()`. In practice you usually create one object from a representative slider, then reuse it to configure all sliders in a group consistently.

Available action IDs:

| Action | Default gesture | Description |
|--------|-----------------|-------------|
| `TextInput` | Shift | Opens text input for direct value entry |
| `ResetToDefault` | Double click | Resets the slider to its default value |
| `ContextMenu` | Right click | Opens the slider context menu |
| `FineTune` | Cmd / Ctrl / Alt | Uses finer drag sensitivity |

Available modifier flags:

| Modifier | Meaning |
|----------|---------|
| `shiftDown`, `altDown`, `ctrlDown`, `cmdDown` | Keyboard modifiers |
| `rightClick`, `doubleClick` | Mouse gesture modifiers |
| `disabled` | Disables the action |
| `noKeyModifier` | Only matches when no keyboard modifier is held |
