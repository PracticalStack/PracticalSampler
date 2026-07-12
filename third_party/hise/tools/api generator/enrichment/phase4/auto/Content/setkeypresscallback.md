Registers a keyboard shortcut callback at the interface level. The key press can be described as a string (e.g. `"ctrl + s"`, `"shift + F5"`) or a JSON object with `keyCode`, `character`, `shift`, `cmd`/`ctrl`, and `alt` properties. The callback receives a single JSON event object with details about the key press.

Pass a non-function value as the second argument to unregister a shortcut. For per-component key handling, use `ScriptComponent.setKeyPressCallback()` instead.

The callback event object includes these properties:

| Property | Type | Description |
| --- | --- | --- |
| `description` | String | Human-readable key description |
| `keyCode` | int | Raw key code |
| `character` | String | Printable character (empty for special keys) |
| `shift` | bool | Shift modifier active |
| `cmd` | bool | Command/Ctrl modifier active |
| `alt` | bool | Alt modifier active |
| `specialKey` | bool | True if not a printable character |
| `isLetter` | bool | True if the character is a letter |
| `isDigit` | bool | True if the character is a digit |