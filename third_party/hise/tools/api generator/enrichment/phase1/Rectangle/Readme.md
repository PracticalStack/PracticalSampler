# Rectangle -- Class Analysis

## Brief
Mutable rectangle utility for layout slicing, resizing, containment checks, and geometric operations.

## Purpose
Rectangle is a scripting wrapper around JUCE's `Rectangle<double>` that provides layout calculation methods commonly used in UI code. It supports the "layout slicing" pattern (removeFromTop/Left/Bottom/Right) for dividing areas into sub-regions, geometric queries (containment, intersection, union), and immutable transformation methods (withX, withWidth, translated, scaled, etc.). Unlike most HISE scripting objects, it extends JUCE's DynamicObject directly rather than ConstScriptingObject, making it interchangeable with the traditional `[x, y, w, h]` array convention used throughout HISE.

## Details

### Architecture

Rectangle has a unique design among HISE scripting classes. Its base class `RectangleDynamicObject` extends JUCE's `DynamicObject` directly (not `ConstScriptingObject`), which allows Rectangle objects to be stored in `var` and passed transparently to any API expecting rectangle data. The scripting subclass `ScriptRectangle` adds `AssignableObject` and `AssignableDotObject` interfaces for indexed and dot-property access.

All methods are dispatched through a shared `FunctionMap` (a `SharedResourcePointer<std::map<Identifier, NativeFunction>>`) populated once with lambda implementations. The method declarations in the header are documentation stubs only -- they contain `jassertfalse` and are never called.

### Property Access

Rectangle exposes four mutable properties accessible by name or index:

| Property | Index | Getter | Setter |
|----------|-------|--------|--------|
| `x` | 0 | `rect.x` | `rect.x = 10` |
| `y` | 1 | `rect.y` | `rect.y = 20` |
| `width` | 2 | `rect.width` | `rect.width = 100` |
| `height` | 3 | `rect.height` | `rect.height = 50` |

Both `rect.x` (dot notation) and `rect[0]` (index notation) work for reading and writing.

### Mutability Semantics

Methods fall into three categories:

1. **Mutating + returning strip:** `removeFromTop`, `removeFromLeft`, `removeFromRight`, `removeFromBottom` -- shrink this rectangle and return the removed strip as a new Rectangle.
2. **Mutating, returning void:** `setPosition`, `setSize`, `setCentre` -- modify this rectangle in place.
3. **Non-mutating:** All `with*` methods, `translated`, `scaled`, `reduced`, `expanded`, and query methods -- return a new Rectangle, leaving this one unchanged.

### Argument Flexibility

Methods that accept rectangle arguments (`contains`, `intersects`, `getUnion`, `getIntersection`, `constrainedWithin`, `withAspectRatioLike`) accept input in three formats:
- Another Rectangle object
- A `[x, y, w, h]` array
- Four separate numeric arguments

The `contains` method additionally accepts a point as `[x, y]` or two separate numbers.

Methods with optional second arguments (`scaled`, `reduced`, `expanded`) use the first argument for both axes when called with a single argument.

### Interoperability with [x,y,w,h] Arrays

Rectangle is fully interoperable with HISE's traditional array-based rectangle convention. All Graphics and component APIs that accept `var` rectangle parameters transparently handle both formats. The `toArray()` method converts back to the array format when needed.

### HISE_USE_SCRIPT_RECTANGLE_OBJECT Preprocessor

By default (disabled), APIs like `ScriptComponent.getLocalBounds()` and LAF callback `obj.area` properties return `[x, y, w, h]` arrays. When `HISE_USE_SCRIPT_RECTANGLE_OBJECT=1` is set in the project's extra definitions, these APIs return Rectangle objects instead, enabling method chaining directly on returned values. The global `Rectangle()` constructor is always available regardless of this setting.

## obtainedVia
`Rectangle(x, y, w, h)` -- global constructor function (also accepts `Rectangle(w, h)`, `Rectangle([x,y,w,h])`, or `Rectangle()`)

## minimalObjectToken
rect

## Constants

(No constants -- Rectangle has no addConstant() registrations.)

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var strip = rect.removeFromTop(30); // expects rect unchanged` | `var strip = rect.removeFromTop(30); // rect is now smaller` | removeFrom* methods mutate the source rectangle AND return the removed strip. This is intentional for layout slicing. |

## codeExample
```javascript
// Create a rectangle and slice it into layout regions
var rect = Rectangle(0, 0, 400, 300);
var header = rect.removeFromTop(50);  // rect is now [0, 50, 400, 250]
var footer = rect.removeFromBottom(30);
var sidebar = rect.removeFromLeft(100);
// rect now contains the remaining center area

// Access properties
var w = rect.width;
var h = rect.height;

// Non-mutating transformations (return new rectangles)
var padded = rect.reduced(10);
var moved = rect.translated(20, 0);
var centered = rect.withSizeKeepingCentre(200, 150);
```

## Alternatives
Use Path for complex vector shapes with curves and sub-paths; Rectangle handles axis-aligned rectangular geometry.

## Related Preprocessors
`HISE_USE_SCRIPT_RECTANGLE_OBJECT` -- controls whether APIs return Rectangle objects instead of [x,y,w,h] arrays (default: disabled).

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Rectangle is a pure value-type utility with no timeline dependencies, no preconditions, and no silent failure modes. All methods either return valid rectangles or booleans.
