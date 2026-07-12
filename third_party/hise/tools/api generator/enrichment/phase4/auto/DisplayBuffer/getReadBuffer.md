Returns a Buffer wrapping channel 0 of the ring buffer's read data. The returned Buffer shares memory with the ring buffer - it is a direct reference, not a copy. This is useful for read-only inspection or computing values like peak magnitude with `Buffer.getMagnitude()`. For a safe independent copy, use `copyReadBuffer()` instead.

> [!Warning:Shared memory reference] Writing to the returned Buffer corrupts the ring buffer data visible to all consumers. Cache the reference once at init time and treat it as read-only in timer callbacks.
