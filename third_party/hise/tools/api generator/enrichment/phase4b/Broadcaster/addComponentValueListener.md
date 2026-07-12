Broadcaster::addComponentValueListener(var object, var metadata, var optionalFunction) -> Integer

Thread safety: UNSAFE -- allocates OwnedArray entries, creates WeakCallbackHolder, resolves component names
Adds a target listener that sets the value of specified UI components when the broadcaster fires.
Direct mode (pass false): uses args.getLast() as value for all targets.
Callback mode: callback receives (targetIndex, ...broadcastArgs) and must return the value to set.
Callback signature: optionalFunction(int targetIndex, var ...broadcastArgs)
Dispatch/mechanics:
  Direct mode: sets args.getLast() as value on all targets via setValue().
  Callback mode: prepends targetIndex, calls callback, sets returned value.
Pair with:
  attachToComponentValue -- source that produces (component, value) events
  removeListener -- remove by metadata
Anti-patterns:
  - Direct mode uses args.getLast() regardless of arg count -- position-dependent, may confuse.
  - Callback mode must return a value -- implicit undefined triggers error per target.
Source:
  ScriptBroadcaster.cpp:3097  ComponentValueItem constructor
