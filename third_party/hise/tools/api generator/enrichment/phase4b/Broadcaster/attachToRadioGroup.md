Broadcaster::attachToRadioGroup(int radioGroupIndex, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source for radio button group selection. Broadcaster must have 1 arg (selectedIndex).
Scans components for matching radioGroup property. Overrides broadcaster's call() for
radio button click handling. Provides 1 initial call with currentIndex.
Dispatch/mechanics:
  Scans all components for matching radioGroup property.
  Overrides broadcaster's call() for radio button click handling.
  Provides 1 initial call with currentIndex.
Pair with:
  addListener -- to handle selected index changes
Notes:
  Bidirectional: dot-assignment or sendSyncMessage also changes the active radio button.
  Index order matches the component list order.
  Common pattern: pair with addComponentPropertyListener for page handling (set visible property based on index).
Anti-patterns:
  - radioGroupIndex of 0 throws error -- must be positive.
  - No components with matching radioGroup throws error.
Source:
  ScriptBroadcaster.cpp  RadioGroupListener constructor
