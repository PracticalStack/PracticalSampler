Returns a DisplayBuffer reference for the buffer at the given index on the source processor. The returned object provides methods for reading buffer contents, creating visualisation paths, and configuring ring buffer properties.

When a processor exposes multiple display buffers (common for dynamics processors), each buffer is accessed by its zero-based index. Dynamics modules typically expose gain reduction at index 0 and peak level at index 1, but the exact layout depends on the processor type.

> [!Warning:No API to query buffer count] There is no method to discover how many display buffers a processor exposes or what each index represents. You must know the processor's buffer layout from its documentation. An out-of-range index causes a script error.
