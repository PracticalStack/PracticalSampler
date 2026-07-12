Broadcaster::addComponentRefreshListener(var componentIds, String refreshType, var metadata) -> Integer

Thread safety: UNSAFE -- allocates OwnedArray entries, resolves component names, creates RefCountedTime slots
Adds a target that triggers a refresh action on specified components when the broadcaster fires.
Ignores broadcast arguments entirely. Valid refreshType values: "repaint", "changed",
"updateValueFromProcessorConnection", "loseFocus", "resetValueToDefault".
  "repaint" = triggers paint routine. "changed" = re-fires control callback.
  "updateValueFromProcessorConnection" = syncs from processorId/parameterId.
  "loseFocus" = removes keyboard focus. "resetValueToDefault" = resets to defaultValue property.
Dispatch/mechanics:
  Parses refreshType string to RefreshType enum.
  callSync() ignores args entirely, triggers the specified refresh action on all targets.
Pair with:
  addComponentPropertyListener -- for setting specific properties instead of generic refresh
  removeListener -- remove by metadata
Anti-patterns:
  - Invalid refreshType string throws error only after the item object is constructed.
  - Empty componentIds list throws "Can't find components for the given componentId object".
Source:
  ScriptBroadcaster.cpp:3546  ComponentRefreshItem constructor
