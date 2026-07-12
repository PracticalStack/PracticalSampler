Applies a path-based alpha mask to the current layer. Pixels inside the path are kept and pixels outside are made transparent. Set `invert` to `true` to reverse this behaviour (keep pixels outside the path instead).

> [!Warning:Non-uniform scaling distorts mask shape] The path is scaled to fit the area with non-uniform scaling. If the path's aspect ratio does not match the area's, the mask shape will be distorted.
