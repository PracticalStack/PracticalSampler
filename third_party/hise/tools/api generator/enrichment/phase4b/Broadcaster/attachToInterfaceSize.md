Broadcaster::attachToInterfaceSize(var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires on interface size changes. Broadcaster must have 2 args
(width, height). Provides 1 initial call with current dimensions.
Useful for responsive layouts.
Dispatch/mechanics:
  Subscribes to ScriptingContent::interfaceSizeBroadcaster (LambdaBroadcaster<int,int>).
  Sends asynchronously. Provides 1 initial call with current dimensions.
Pair with:
  addListener -- to handle (width, height) callbacks
Anti-patterns:
  - Error message incorrectly says "visibility events" instead of "interface size events" (copy-paste bug).
  - Size changes dispatched asynchronously.
Source:
  ScriptBroadcaster.cpp  InterfaceSizeListener constructor
