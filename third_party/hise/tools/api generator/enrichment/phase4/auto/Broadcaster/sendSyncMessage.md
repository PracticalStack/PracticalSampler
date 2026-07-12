Sends a synchronous message to all registered listeners on the calling thread. For multi-argument broadcasters, pass an array with one element per argument. For single-argument broadcasters, pass the value directly.

This is functionally equivalent to dot-assignment syntax (`bc.argName = value`) but sets all arguments in a single call. Identical consecutive values are suppressed by change detection unless queue mode is enabled.
