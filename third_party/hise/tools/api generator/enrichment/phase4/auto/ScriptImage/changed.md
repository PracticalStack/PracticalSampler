Triggers the control callback with the component's current value, as if the user had interacted with it. Fires the custom callback set via `setControlCallback()`, or the default `onControl` handler if none is set.

> [!Warning:Ignored during onInit] Cannot be called during `onInit` - it returns without executing and logs a message to the console.
