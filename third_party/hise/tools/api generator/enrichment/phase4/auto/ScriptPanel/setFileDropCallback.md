Registers a callback for file drag-and-drop events. The callback level controls which events fire:

| Level | Events |
|-------|--------|
| `"No Callbacks"` | Disabled (default) |
| `"Drop Only"` | Fires only when files are dropped |
| `"Drop & Hover"` | Also fires when files enter or exit the panel |
| `"All Callbacks"` | Also fires on mouse movement during drag |

The wildcard filters accepted file types (e.g. `"*.wav;*.aif"`). Use `"{FOLDER}"` as the wildcard to accept only folder drops. Pass an empty string to deactivate the callback.

The callback receives a JSON object with `x`, `y`, `hover`, `drop`, and `fileName` properties. The `fileName` is only present on drop events and contains the absolute path (or an array of paths for multi-file drops). Use `FileSystem.fromAbsolutePath()` to convert a dropped path to a File object.

To persist a dropped filename in a user preset, wrap it in a JSON object before calling `setValue()` - raw String values are not permitted as preset values.

> [!Warning:Non-matching files silently ignored] The wildcard filter is checked before the callback fires. Files that do not match are silently ignored with no callback.
