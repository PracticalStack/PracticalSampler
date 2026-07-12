Expands the path's reported bounding box to include the specified rectangle without transforming any existing geometry. Internally, this adds two invisible `startNewSubPath` calls at the top-left and bottom-right corners of the given area - equivalent to calling `startNewSubPath(0, 0)` and `startNewSubPath(1, 1)` manually, but shorter.

This is essential when drawing arcs in normalised space for rotary knobs. An arc alone has a bounding box that covers only the arc segment, causing `Graphics.drawPath` to skew the shape when scaling to the component area. Calling `setBounds([0, 0, 1, 1])` before adding the arc anchors the bounding box to the full unit square, ensuring correct proportions.

The parameter also accepts a `Rectangle()` constructor object as an alternative to array literals:

```js
var n = Rectangle(1.0, 1.0);
p.setBounds(n);
p.addArc(n, -2.4, 2.4);
```

> [!Warning:Does not resize or transform geometry] `setBounds` does not resize or transform the path. This is the most common source of confusion. Use `scaleToFit` to actually rescale path geometry into a target area.
