Broadcaster::addListener(var object, var metadata, var function) -> Integer

Thread safety: UNSAFE -- allocates OwnedArray entries, creates WeakCallbackHolder, performs realtime safety check
Adds a general-purpose callback listener. The primary method for receiving broadcast messages.
The object parameter serves as the this reference in the callback (when setReplaceThisReference
is true, the default). Duplicates rejected. Sorted by metadata priority.
Callback signature: function(var ...broadcastArgs)
Dispatch/mechanics:
  Wraps callback in WeakCallbackHolder with broadcaster's numArgs.
  Object parameter replaces `this` in callback by default. Can be string, JSON object, or component reference.
  Function called synchronously at registration time with current values (init dispatch).
  Priority via metadata JSON: { "id": "X", "priority": 10 } -- higher values execute first.
  On realtime-safe broadcaster: validates callback for audio-thread safety.
  callSync() invokes callback with obj as this reference.
Pair with:
  removeListener -- remove by metadata
  setReplaceThisReference -- controls this binding in callback
Anti-patterns:
  - On realtime-safe broadcaster, non-inline function in exported plugin throws error.
  - Duplicate metadata rejected with error "already registered".
Source:
  ScriptBroadcaster.cpp:849  ScriptTarget constructor
