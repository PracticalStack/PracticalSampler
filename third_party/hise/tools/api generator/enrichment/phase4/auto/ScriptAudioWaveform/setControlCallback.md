Assigns a custom inline function as the control callback for this component, replacing the default `onControl` handler. The function receives two parameters: the component reference and the new value. Pass `false` to revert to the default `onControl` callback.

> [!Warning:Requires inline function with two parameters] The function must be declared with `inline function` and must have exactly two parameters. Regular function references are rejected with a script error.
