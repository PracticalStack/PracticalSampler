Registers a callback that fires when a consumed key is pressed while this component has focus. You must call `setConsumedKeyPresses()` before this method.

The callback receives an event object with `isFocusChange`, `character`, `keyCode`, `description`, `shift`, `cmd`, `alt`, and other properties. Focus change events have `isFocusChange: true` and `hasFocus` instead of key properties.
