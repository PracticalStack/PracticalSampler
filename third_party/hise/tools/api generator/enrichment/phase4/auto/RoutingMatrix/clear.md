Removes all primary and send connections. Use this before rebuilding a routing configuration with `addConnection()` calls to ensure no stale connections remain from a previous setup.

> [!Warning:Auto-correction may re-add passthrough] Under the default stereo constraint, `clear()` may not leave the matrix truly empty - auto-correction can re-add a default passthrough connection. Call `setNumChannels()` first to relax the constraint if you need full control over the cleared state.
