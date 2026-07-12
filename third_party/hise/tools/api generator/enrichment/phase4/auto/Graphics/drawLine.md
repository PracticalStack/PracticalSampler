Draws a straight line from point (x1, y1) to point (x2, y2) with the specified stroke thickness, using the current colour.

> [!Warning:Parameter order is x1, x2, y1, y2] The parameter order is `(x1, x2, y1, y2, thickness)`, grouping x-coordinates before y-coordinates. This is unusual - most drawing APIs use point-by-point order `(x1, y1, x2, y2)`. Despite the unusual order, the line draws correctly from (x1, y1) to (x2, y2).
