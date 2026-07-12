Registers a callback that fires when a consumed key is pressed while this component has focus. The callback receives an event object describing the key press or a focus change event.

For key press events, the object contains `isFocusChange` (false), `character`, `specialKey`, `keyCode`, `description`, and modifier flags (`shift`, `cmd`, `alt`). For focus change events, it contains `isFocusChange` (true) and `hasFocus`.

> [!Warning:Call setConsumedKeyPresses first] You must call `setConsumedKeyPresses()` before this method. Calling them in the wrong order reports a script error.
