Returns a Buffer view into a subrange of the source buffer, so you can work on sections without copying data. Use `Buffer.create(...)` plus copy operators if you need an independent clone instead.

> [!Warning:Slice aliases source memory] The returned slice aliases the source memory. Writing to the slice writes to the original region.

> [!Warning:Negative offsets not safely clamped] Negative offsets are not clamped safely. Use `0` or higher offsets.
