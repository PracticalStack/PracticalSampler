Registers a callback that fires when the transport state changes (play/stop). The callback receives a boolean indicating whether the transport is playing. Fires immediately upon registration with the current play state.

A common pattern is to pass a Broadcaster as the callback function instead of a plain function. The TransportHandler calls the Broadcaster with the isPlaying argument, which propagates to all listeners. This enables loose coupling when multiple script files need to react to transport changes.
