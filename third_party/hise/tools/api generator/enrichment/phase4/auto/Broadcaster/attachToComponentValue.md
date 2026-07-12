Registers the broadcaster as a source that fires whenever the value of any of the specified components changes. Pass a single component name or an array of component names. On attachment, existing listeners immediately receive the current value of each watched component.

The listener callback signature `function(component, value)` is identical to `setControlCallback()`, which makes it straightforward to migrate from control callbacks to a broadcaster-based system. The key difference is that `attachToComponentValue` is non-exclusive -- it does not replace any existing control callback on the component.

> [!Warning:Only one broadcaster per component] If two broadcasters both call `attachToComponentValue` on the same component, only the last one receives updates. Each component supports a single broadcaster value listener at a time.
