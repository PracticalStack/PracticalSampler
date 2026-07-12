Adds a cubic Bezier curve from the current position to the endpoint `(x, y)`, shaped by two control points. The first control point pulls the curve away from the start, and the second pulls it toward the end.

> [!Warning:Mixed parameter convention for points] Uses a mixed parameter convention: control points are `[x, y]` arrays, but the endpoint uses two separate scalar values. This differs from `quadraticTo` (which uses four scalars) and from array-based methods like `addTriangle`.
