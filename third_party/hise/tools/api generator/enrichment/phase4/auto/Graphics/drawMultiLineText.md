Draws text that automatically wraps to new lines when it exceeds the specified `maxWidth`. The `xy` parameter is a 2-element `[x, y]` array specifying the starting point, where `y` is the text baseline. The `leading` parameter adds extra vertical spacing between lines (0.0 uses the default font-height spacing).

> [!Warning:Takes point array, not area array] The `xy` parameter is a `[x, y]` point (2 elements), not an `[x, y, w, h]` area like most other Graphics methods. The `y` value is the baseline position, so the first line of text appears slightly above the y coordinate.
