Extracts a single named property from every element and writes the values into a target Buffer or Array. Returns 1 on success. When copying to a Buffer, the Buffer must have the same size as the array's `length` constant. When copying to a regular Array, it is resized automatically.

This is useful for feeding flat data to downstream systems that consume numeric arrays rather than structured objects - for example, extracting a column of gain values into a Buffer for DSP processing or shader upload. Pre-allocate all target Buffers at init time to preserve the allocation-free design.

> [!Warning:Float cast loses integer precision] When copying to a Buffer, all values are cast to float. Integer properties lose precision for values beyond +/-16,777,216.
