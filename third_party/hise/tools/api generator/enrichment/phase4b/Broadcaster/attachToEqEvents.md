Broadcaster::attachToEqEvents(var moduleIds, var eventTypes, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source for EQ events on specified module(s). Broadcaster must have 2 args
(eventType, value). Valid types: "BandAdded", "BandRemoved", "BandSelected", "FFTEnabled".
Empty string/array subscribes to all types. No initial values.
Dispatch/mechanics:
  Registers with ProcessorFilterStatistics eventBroadcaster.
  Valid types: BandAdded, BandRemoved, BandSelected, FFTEnabled.
  Empty event string/array subscribes to all types.
Pair with:
  addListener -- to handle EQ event callbacks
Notes:
  EQ band attributes use formula: attributeIndex = attributeType + bandIndex * bandOffset.
Anti-patterns:
  - Module must implement ProcessorFilterStatistics::Holder (EQ modules only).
  - Empty eventTypes subscribes to all -- intentional convenience, not an error.
Source:
  ScriptBroadcaster.cpp:4414  EqListener constructor
