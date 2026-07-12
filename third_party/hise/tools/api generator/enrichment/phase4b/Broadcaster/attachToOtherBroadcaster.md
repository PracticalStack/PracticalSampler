Broadcaster::attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Chains this broadcaster to source broadcaster(s). Messages forwarded to this broadcaster's
listeners. Optional transform function can remap args -- must return an array to replace args,
non-array return forwards original args unchanged.
Callback signature: argTransformFunction(var ...sourceArgs)
Dispatch/mechanics:
  Adds OtherBroadcasterTarget to source's items list.
  Transform function: if returns array, replaces args; if non-array, forwards original.
  Dispatches initial lastValues from source on attachment.
Pair with:
  addListener -- to handle forwarded messages
  sendSyncMessage / sendAsyncMessage -- alternative to chaining
Anti-patterns:
  - Transform must return array to replace args -- non-array return forwards originals.
  - No arg count validation between source and target broadcaster.
Source:
  ScriptBroadcaster.cpp:933  OtherBroadcasterTarget constructor
