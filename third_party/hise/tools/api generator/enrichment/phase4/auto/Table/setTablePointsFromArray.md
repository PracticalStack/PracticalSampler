Replaces all control points from a nested array of `[x, y, curve]` sub-arrays. Requires at least 2 points (script error otherwise) and each sub-array must have exactly 3 elements. All values are clamped to 0.0-1.0. This is the preferred method for bulk point setup since it triggers only a single lookup table re-render.

> [!Warning:Edge x values forced to 0 and 1] The first point's x is silently forced to 0.0 and the last point's x to 1.0, regardless of the values you pass.
