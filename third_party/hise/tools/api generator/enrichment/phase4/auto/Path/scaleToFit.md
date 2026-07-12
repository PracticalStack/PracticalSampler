Transforms the path geometry to fit within a target rectangle. All coordinates are permanently modified. When `preserveProportions` is `true`, the path is uniformly scaled to fit within the area while maintaining its aspect ratio. When `false`, the path stretches to fill the full width and height independently.

This is the method to use when you want to resize a path into a target area. Unlike `setBounds` (which only expands the reported bounding box), `scaleToFit` actually transforms the geometry.

> [!Warning:Transformation is permanent] The transformation is permanent. If you need to query the path later (e.g. with `getYAt`) in pixel space, call `scaleToFit` first, then query. The path cannot be "unscaled" back to its original coordinates - rebuild it if you need the original space.
