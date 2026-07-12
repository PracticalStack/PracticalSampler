Tests whether a line segment from `start` to `end` intersects the path. If an intersection exists, returns the intersection point as `[x, y]`. The `keepSectionOutsidePath` flag controls which endpoint of the clipped line is returned: `true` for the outer point, `false` for the entry point into the path.

> [!Warning:Returns false when no intersection found] Returns `false` (not an array) when no intersection is found. Always check the return type before indexing the result.
