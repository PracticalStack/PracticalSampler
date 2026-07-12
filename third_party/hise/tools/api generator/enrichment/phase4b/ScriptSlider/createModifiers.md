ScriptSlider::createModifiers() -> ScriptObject

Thread safety: UNSAFE
Creates a Modifiers script object containing action keys and modifier flag constants for setModifiers.

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

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  setModifiers -- consumes Modifiers constants for action and flag mapping

Source:
  ScriptingApiContent.cpp:2054  ScriptSlider::ModifierObject creation and constant registration
