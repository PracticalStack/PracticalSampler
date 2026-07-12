Applies a rotation transform around the specified centre point. All subsequent drawing operations are rotated by the given angle in radians (positive = clockwise). The `center` parameter is a 2-element `[x, y]` array. The rotation persists for all subsequent draw calls in the same paint callback.

> [!Warning:Always undo rotation after drawing] Always undo the rotation after drawing the rotated element by calling `g.rotate(-angle, centre)`. Forgetting this causes all subsequent drawing in the same callback to be rotated, producing misaligned text and shapes.
