Registers a callback that fires whenever any dynamic child component's value changes. The callback receives two arguments: the component ID (String) and the new value. This monitors child component values, not the container's own value - use `setControlCallback()` for the container itself.

> [!Warning:Call setData before registering callback] Silently does nothing if called before `setData()`. The callback requires the internal value store, which is only created when `setData()` builds the component tree.
