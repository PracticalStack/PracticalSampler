Modifies an existing control point at the given index. All values (x, y, curve) are clamped to the 0.0-1.0 range. For non-edge points, all three values are updated. Use `getTablePointsAsArray()` to find the correct index for a given point.

> [!Warning:Edge points ignore x parameter] Edge points (first and last) silently ignore the x parameter - only y and curve are applied. No error is thrown when passing a different x value for an edge point.
