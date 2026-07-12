Copies the ring buffer's read data into a preallocated target buffer, providing a thread-safe independent copy. Pass a single Buffer to copy channel 0, or an Array of Buffers for multi-channel data.

> [!Warning:Target buffer size must match exactly] There is no automatic resizing - if the target buffer size does not match the ring buffer's sample count, the copy fails. Use `getResizedBuffer()` if you need a different number of samples.
