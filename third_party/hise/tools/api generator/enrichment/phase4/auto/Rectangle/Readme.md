<!-- Diagram triage:
  - No diagrams specified in Phase 1 data. None to render.
-->

# Rectangle

Rectangle is a native HISEScript type for axis-aligned rectangular geometry, designed for UI layout work. It replaces manual `[x, y, w, h]` array arithmetic with a purpose-built object that supports method chaining. The constructor accepts several argument forms:

```javascript
var r0 = Rectangle();                    // empty rectangle
var r1 = Rectangle([x, y, w, h]);        // from array
var r2 = Rectangle(width, height);       // at origin [0, 0]
var r3 = Rectangle(x, y, w, h);          // full specification
var r4 = Rectangle([x1, y1], [x2, y2]);  // from two corner points
var r5 = Rectangle(r4);                  // from a preexisting Rectangle object
```

Rectangle provides three categories of operations:

1. **Layout slicing** (`removeFromTop`, `removeFromLeft`, `removeFromBottom`, `removeFromRight`) - mutate this rectangle and return the removed strip as a new Rectangle. This is the primary use case for dividing a draw area into sub-regions inside paint routines and Look and Feel callbacks.
2. **Non-mutating transformations** (`with*`, `translated`, `scaled`, `reduced`, `expanded`) - return a new Rectangle without modifying the original.
3. **Geometry queries** (`contains`, `intersects`, `getIntersection`, `getUnion`, `constrainedWithin`, `isEmpty`) - test spatial relationships between rectangles and points.

Properties are accessible by name (`rect.x`, `rect.width`) or by index (`rect[0]`, `rect[2]`), and both forms support reading and writing. Rectangle objects are fully interoperable with the traditional `[x, y, w, h]` array convention - all `Graphics` drawing methods and component APIs accept either format.

You can inspect rectangles visually using a sampling session: enable sampling for a scope with `.sample("id")`, add rectangles via `Console.sample()`, then click the inspect icon to open the rectangle viewer.

> [!Tip:Returns new objects, use in UI code only] Most methods return new Rectangle objects, making Rectangle unsuitable for realtime thread operations. In practice this is not a limitation, as the vast majority of use cases are within paint routines and other UI functions.

By default, APIs like `ScriptComponent.getLocalBounds()` and LAF callback `obj.area` properties return plain `[x, y, w, h]` arrays. Existing projects depend on this array format, so it cannot be changed globally without breaking code. The recommended pattern is to wrap the return value in `Rectangle()` at the top of each paint routine or LAF callback:

```javascript
// Works regardless of project settings - recommended best practice
var rect = Rectangle(this.getLocalBounds(0));
var area = Rectangle(obj.area);
```

When the source already returns a Rectangle object, the wrapping constructor creates a lightweight copy - negligible overhead since Rectangle is a very small class. This pattern is portable across all projects and avoids reliance on preprocessor settings.

Alternatively, new projects that have no existing code depending on array return values can set `HISE_USE_SCRIPT_RECTANGLE_OBJECT=1` in the project's preprocessor definitions (Extra Definitions field). This makes all APIs return Rectangle objects directly, removing the need for wrapping. Do not enable this on existing projects without auditing all code that accesses area properties by array index.

## Common Mistakes

- **removeFrom mutates the source rectangle**
  **Wrong:** `var strip = rect.removeFromTop(30);` and expecting `rect` to be unchanged.
  **Right:** `var strip = rect.removeFromTop(30);` - `rect` is now smaller and `strip` holds the removed portion.
  *The `removeFrom*` methods mutate the source rectangle and return the removed strip. This dual behaviour is intentional for the layout slicing pattern.*

- **Wrap plain arrays in Rectangle first**
  **Wrong:** `var area = obj.area; area.removeFromTop(30);` when `obj.area` is still a `[x,y,w,h]` array.
  **Right:** `var area = Rectangle(obj.area); area.removeFromTop(30);`
  *When `obj.area` is a plain array, slicing it mutates the array in place, which corrupts the original `obj.area` for subsequent repaints. Wrapping in `Rectangle()` creates an independent copy.*

- **Use built-in Rectangle class instead**
  **Wrong:** Writing a custom `Rect` helper namespace with manual array arithmetic for layout calculations.
  **Right:** Using the built-in `Rectangle()` class, which provides all layout slicing, transformation, and query methods natively.
  *The helper namespace pattern was necessary before Rectangle existed but is now redundant.*
