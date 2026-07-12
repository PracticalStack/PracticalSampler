Registers a script callback that fires when an incoming OSC message matches the given sub-address. The sub-address is combined with the domain set in `connectToOSC` to form the full OSC address pattern. Registration order does not matter - you can register callbacks before or after calling `connectToOSC`.

The callback receives two parameters: the matched sub-address (String) and the value. For single-argument OSC messages, the value is a float, integer, or String. For multi-argument messages, the value is an Array. This gives broader type support than cable-based OSC routing, which is limited to single normalised numbers.

The sub-address supports OSC pattern wildcards (e.g., `"/*"` to catch all messages under the domain).

> [!Warning:Callback runs on the OSC receiver thread] The callback executes on the OSC receiver thread with high priority, not on the scripting UI thread. Avoid heavy processing or direct UI state changes inside the callback.
