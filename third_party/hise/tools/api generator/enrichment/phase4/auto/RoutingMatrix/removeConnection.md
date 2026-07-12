Removes the primary connection between `sourceIndex` and `destinationIndex`. Returns `true` if the connection existed and was removed.

> [!Warning:Auto-restores passthrough below minimum] Under the default stereo constraint, removing a connection that drops the count below 2 auto-restores a default passthrough. Call `setNumChannels()` first if you need to remove connections without auto-correction.
