# Rectangle -- Project Context

## Project Context

### Real-World Use Cases
- **LAF layout slicing**: The primary use case for Rectangle is dividing a draw area into sub-regions inside Look and Feel callbacks and paint routines. A typical LAF function receives `obj.area` and progressively slices it with `removeFromTop`, `removeFromLeft`, etc. to allocate space for headers, labels, track areas, and controls. The Rectangle class replaces the manual `[x,y,w,h]` array manipulation that previously required a helper namespace.
- **Hit test regions**: Storing Rectangle objects in panel `data` properties during initialization, then calling `.contains([event.x, event.y])` inside mouse callbacks to detect which region was clicked. This replaces manual bounds-checking arithmetic.
- **Dynamic progress visualization**: Creating a partial sub-region of a button or panel area (e.g., `Rectangle(obj.area).removeFromLeft(progress * obj.area[2])`) to draw a progress fill inside a button while it is loading.
- **Proportional time-segment layout**: Dividing an area into proportional time segments (attack, hold, decay, sustain, release) for custom envelope editors. Progressive `removeFromLeft` calls with proportional widths, chained with `removeFromBottom` to set level offsets.

### Complexity Tiers
1. **Basic layout slicing** (most common): `removeFromTop`, `removeFromLeft`, `removeFromBottom`, `removeFromRight`, `reduced`, `withSizeKeepingCentre`. These six method families handle the vast majority of real-world layout work.
2. **Non-mutating transformations**: `translated`, `withX`, `withY`, `withWidth`, `withHeight`, `withCentre`, `withSize`. Used when you need a copy of a region at a different position or size without modifying the source.
3. **Geometry queries and advanced layout**: `contains`, `intersects`, `constrainedWithin`, `withAspectRatioLike`, `getIntersection`, `getUnion`. Used in hit testing, icon fitting, and constraint-based layout.

### Practical Defaults
- Use `Rectangle(this.getLocalBounds(0))` or `Rectangle(obj.area)` as the starting point for all layout slicing inside paint routines and LAF callbacks.
- Use `withSizeKeepingCentre` to center icons, indicators, and knob handles inside their allocated areas. This is more readable and less error-prone than manual centering arithmetic.
- Use `reduced(amount)` for uniform padding and `reduced(xAmount, yAmount)` for asymmetric padding. Negative values to `reduced` expand the rectangle, which is useful for creating hover/focus outlines.
- Store pre-computed Rectangle objects in `panel.data` during setup rather than recomputing them each paint cycle. This improves readability and performance.

### Integration Patterns
- `Rectangle(obj.area)` -> `g.fillRoundedRectangle()` / `g.drawRoundedRectangle()` -- Rectangle objects are accepted directly by all Graphics drawing methods that take area parameters.
- `Rectangle.contains([event.x, event.y])` inside `setMouseCallback` -- hit testing stored regions against mouse coordinates for custom interactive panels.
- `Rectangle.removeFromLeft(width).withSizeKeepingCentre(iconW, iconH)` -> `g.fillPath(path, area)` -- method chaining to position an icon within a sliced column.
- `Rectangle(component.getLocalBounds(0)).translated(component.get("x"), component.get("y"))` -- converting component-local bounds to parent-relative coordinates for overlay rendering.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Implementing a `Rect` helper namespace with manual `[x,y,w,h]` array arithmetic | Using the built-in `Rectangle()` class directly | The Rectangle class provides all layout slicing methods natively with proper method chaining. The helper namespace pattern was necessary before Rectangle existed but is now redundant. |
| `var area = obj.area; area.removeFromTop(30);` expecting `obj.area` unchanged when it is an array | `var area = Rectangle(obj.area); area.removeFromTop(30);` | When `obj.area` is a `[x,y,w,h]` array, `removeFrom*` on the Rect namespace mutates the array in-place, which also mutates `obj.area`. Wrapping in `Rectangle()` creates an independent object. |
