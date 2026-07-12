Broadcaster::addComponentPropertyListener(var object, var propertyList, var metadata, var optionalFunction) -> Integer

Thread safety: UNSAFE -- allocates OwnedArray entries, creates WeakCallbackHolder, resolves component names via string lookup
Adds a target listener that sets properties on specified UI components when the broadcaster fires.
BroadcasterMap renders property values contextually (colours as colours, not hex strings).
Direct mode (pass false): broadcaster must have 3 args (component, propertyId, value); value from args[2] applied to targets, skipping source component.
Callback mode: callback receives (targetIndex, ...broadcastArgs) and must return the property value.
Callback signature: optionalFunction(int targetIndex, var component, var propertyId, var value)
Required setup:
  const var bc = Engine.createBroadcaster({ "id": "X", "args": ["component", "property", "value"] });
Dispatch/mechanics:
  Allocates OwnedArray entry, creates WeakCallbackHolder.
  propertyList can be a single string or array of strings for multiple properties.
  Direct mode: sets property on all targets, skips source component (args[0]) to avoid feedback.
  Callback mode: prepends targetIndex, calls callback, sets returned value as property.
Pair with:
  attachToComponentProperties -- source that produces (component, property, value) events
  removeListener -- remove by metadata
Anti-patterns:
  - Direct mode requires exactly 3 broadcast args -- wrong arg count silently uses incorrect indices.
  - Callback mode must return a value -- implicit undefined triggers error per target component.
  - Feedback skip in direct mode compares args[0] via var::operator==. String name vs reference mismatch defeats the skip.
Source:
  ScriptBroadcaster.cpp  ComponentPropertyItem constructor
