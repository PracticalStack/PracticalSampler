Registers a popup context menu on the specified components. When the user right-clicks (or double-taps on trackpad, or Ctrl+clicks on macOS), a popup menu appears using the component's LookAndFeel customisation. When the user selects an item, the broadcaster fires with `(componentReference, selectedItemIndex)` where the index is zero-based. Set `useLeftClick` to `true` to open the menu on left-click instead.

The `itemList` supports pseudo-markdown formatting: `**Bold Text**` for section headers, `___` for horizontal separators, and `~~Strikethrough~~` for always-disabled items. Use the `{DYNAMIC}` wildcard in an item's text to request dynamic text from the state function at display time.

The optional `stateFunction` controls menu item appearance. It receives `(type, index)` where `this` points to the clicked component, and must return a value:

| Type | Return | Purpose |
|---|---|---|
| `"active"` | Boolean | Whether the item shows a tick mark |
| `"enabled"` | Boolean | Whether the item is interactable (not greyed out) |
| `"text"` | String | Dynamic text override (empty string uses the static label) |

Pass `false` for the state function to skip state management.

> [!Warning:State function results are cached] State function results are cached and only refreshed after a menu selection or an explicit `refreshContextMenuState()` call. If external state changes between menu openings, call `refreshContextMenuState()` to update tick marks and enabled states.

> [!Warning:Does not override right-click behaviour] This attachment does not override existing right-click behaviour. Disable `enableMidiLearn` on attached components to prevent the MIDI learn popup from conflicting with the context menu.
