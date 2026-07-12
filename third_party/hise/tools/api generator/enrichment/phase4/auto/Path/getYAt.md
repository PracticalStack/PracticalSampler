Returns the Y coordinate where the path first crosses the given X position. This is useful for interactive envelope editors that need to place draggable control points along a curve - after building the envelope path and calling `scaleToFit` to map it to pixel coordinates, `getYAt` samples the curve height at any X position.

> [!Warning:Returns undefined outside path segments] Returns `undefined` when no path segment contains the given X position. Always check with `isDefined()` before using the result in arithmetic.

> [!Warning:Operates in current coordinate space] Operates in whatever coordinate space the path currently occupies. If the path was built in normalised `[0, 1]` space, call `scaleToFit` first to transform it to pixel coordinates before querying with pixel X values.
