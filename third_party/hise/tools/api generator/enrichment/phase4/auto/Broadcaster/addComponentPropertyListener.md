Adds a target that sets properties on the specified UI components whenever the broadcaster fires. Operates in two modes:

- **Direct mode** (pass `false` for the callback): the broadcaster must have exactly 3 arguments `(component, propertyId, value)`. The value from the third argument is applied to the named properties on all target components, skipping the source component to avoid feedback loops.
- **Callback mode**: the callback receives `(targetIndex, ...broadcastArgs)` where `targetIndex` is the index of the current target in the component list. The callback must return the value to set as the property.

The `propertyList` parameter accepts a single string or an array of strings for multiple properties. On registration, target components immediately receive their initial property values. The BroadcasterMap renders property values contextually -- colours as colours, not hex strings -- giving a quick visual check that the correct values are flowing.

> [!Warning:Direct mode requires exactly three arguments] In direct mode, the broadcaster must have exactly 3 arguments. Using a different argument count silently creates the target but produces incorrect property values at dispatch time.
