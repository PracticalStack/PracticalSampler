# Rectangle -- Method Analysis

## constrainedWithin

**Signature:** `Rectangle constrainedWithin(var targetArea)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var clamped = {obj}.constrainedWithin([0, 0, 800, 600]);`

**Description:**
Returns a new Rectangle that is moved to fit within the given target area, keeping its size unchanged if it already fits. If this rectangle is larger than the target area in either dimension, the result is clamped to the target area's bounds on that axis. Delegates to JUCE's `Rectangle::constrainedWithin()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetArea | ScriptObject | no | The bounding rectangle to constrain within. Accepts a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments. | Must be a valid rectangle |

**Pitfalls:**
- If the argument cannot be parsed as a rectangle (wrong type, wrong array length), the method silently returns a copy of the original rectangle unchanged instead of reporting an error.

**Cross References:**
- `$API.Rectangle.contains$`
- `$API.Rectangle.getIntersection$`

## contains

**Signature:** `bool contains(var otherRectOrPoint)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var inside = {obj}.contains([50, 50]);`

**Description:**
Tests whether this rectangle fully contains another rectangle or contains a point. When given a rectangle argument, returns `true` only if the other rectangle is completely inside this one. When given a point argument, returns `true` if the point lies within the bounds of this rectangle. Accepts input in multiple formats: a Rectangle object, a `[x, y, w, h]` array (for rectangle check), four separate numbers (for rectangle check), a `[x, y]` array (for point check), or two separate numbers (for point check).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherRectOrPoint | ScriptObject | no | Another rectangle (Rectangle object, `[x, y, w, h]` array, or 4 numbers) or a point (`[x, y]` array or 2 numbers). | Must be parseable as rectangle or point |

**Pitfalls:**
- Two-argument calls are ambiguous: `contains(50, 100)` is parsed as a point `(50, 100)` because `getRectangleArgs` requires exactly 4 numeric arguments, but `getPointArgs` accepts `numArguments >= 2`. This is consistent and correct, but worth noting since `contains` is the only Rectangle method that accepts both rectangle and point arguments.
- If the argument cannot be parsed as either a rectangle or a point, the method returns `false` silently rather than reporting an error.

**Cross References:**
- `$API.Rectangle.intersects$`
- `$API.Rectangle.constrainedWithin$`
- `$API.Rectangle.isEmpty$`

## expanded

**Signature:** `Rectangle expanded(double x, double optionalY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var bigger = {obj}.expanded(10);`

**Description:**
Returns a new Rectangle that is larger than this one by the given amount. With one argument, the rectangle is expanded uniformly on all sides (each edge moves outward by the given amount, so width and height each increase by twice the amount). With two arguments, the first controls horizontal expansion and the second controls vertical expansion. This is the inverse of `reduced()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Double | no | Horizontal expansion amount (or uniform expansion if only one argument). Each left/right edge moves by this amount. | Any number; negative values shrink |
| optionalY | Double | no | Vertical expansion amount. Each top/bottom edge moves by this amount. If omitted, uses the first argument for both axes. | Any number; negative values shrink |

**Pitfalls:**
- Negative values are allowed and produce the same effect as `reduced()` -- the rectangle shrinks instead of growing.

**Cross References:**
- `$API.Rectangle.reduced$`

## getIntersection

**Signature:** `Rectangle getIntersection(var otherRect)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var overlap = {obj}.getIntersection([50, 50, 200, 200]);`

**Description:**
Returns a new Rectangle representing the intersection of this rectangle and the given rectangle -- the largest area that fits within both. If the rectangles do not overlap, returns an empty rectangle (zero width and/or height). Accepts the argument as a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherRect | ScriptObject | no | The other rectangle to intersect with. Accepts a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments. | Must be a valid rectangle |

**Pitfalls:**
- If the argument cannot be parsed as a rectangle, the method silently returns a copy of the original rectangle unchanged instead of reporting an error or returning an empty rectangle.

**Cross References:**
- `$API.Rectangle.intersects$`
- `$API.Rectangle.getUnion$`
- `$API.Rectangle.constrainedWithin$`

## getUnion

**Signature:** `Rectangle getUnion(var otherRect)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var combined = {obj}.getUnion([200, 200, 100, 100]);`

**Description:**
Returns a new Rectangle that is the smallest rectangle containing both this rectangle and the given rectangle. This is the bounding box of both rectangles combined. Accepts the argument as a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherRect | ScriptObject | no | The other rectangle to combine with. Accepts a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments. | Must be a valid rectangle |

**Pitfalls:**
- If the argument cannot be parsed as a rectangle, the method silently returns a copy of the original rectangle unchanged instead of reporting an error.

**Cross References:**
- `$API.Rectangle.getIntersection$`
- `$API.Rectangle.intersects$`
- `$API.Rectangle.contains$`

## intersects

**Signature:** `bool intersects(var otherRect)`
**Return Type:** `bool`
**Call Scope:** safe
**Minimal Example:** `var overlaps = {obj}.intersects([200, 100, 100, 100]);`

**Description:**
Returns `true` if any part of another rectangle overlaps this one. Returns `false` if the rectangles do not overlap or if the argument cannot be parsed as a valid rectangle. Accepts the argument as a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherRect | ScriptObject | no | The other rectangle to test for overlap. Accepts a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments. | Must be a valid rectangle |

**Pitfalls:**
- If the argument cannot be parsed as a rectangle, the method silently returns `false` instead of reporting an error. This is consistent with `contains()`.

**Cross References:**
- `$API.Rectangle.contains$`
- `$API.Rectangle.getIntersection$`
- `$API.Rectangle.getUnion$`

## reduced

**Signature:** `Rectangle reduced(double x, double optionalY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var smaller = {obj}.reduced(10);`

**Description:**
Returns a new Rectangle that is smaller than this one by the given amount. With one argument, the rectangle is reduced uniformly on all sides (each edge moves inward by the given amount, so width and height each decrease by twice the amount). With two arguments, the first controls horizontal reduction and the second controls vertical reduction. This is the inverse of `expanded()`. Called with zero arguments, returns a copy of the original rectangle unchanged.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Double | no | Horizontal reduction amount (or uniform reduction if only one argument). Each left/right edge moves inward by this amount. | Any number; negative values expand |
| optionalY | Double | no | Vertical reduction amount. Each top/bottom edge moves inward by this amount. If omitted, uses the first argument for both axes. | Any number; negative values expand |

**Pitfalls:**
- Negative values are allowed and produce the same effect as `expanded()` -- the rectangle grows instead of shrinking.

**Cross References:**
- `$API.Rectangle.expanded$`
- `$API.Rectangle.withTrimmedTop$`

## removeFromBottom

**Signature:** `Rectangle removeFromBottom(double numToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var footer = {obj}.removeFromBottom(30);`

**Description:**
Removes a strip from the bottom of this rectangle. This method mutates the source rectangle by shrinking its height from the bottom, and returns a new Rectangle containing the removed strip. The returned strip has the same x position and width as the original, with height equal to `numToRemove` and y position at the new bottom of the source. This is part of the "layout slicing" pattern for dividing an area into sub-regions by iteratively removing strips from each side.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numToRemove | Double | no | The height of the strip to remove from the bottom. | Any number |

**Pitfalls:**
- This method mutates the source rectangle. After calling, the source rectangle is smaller (its height is reduced by `numToRemove`). The returned Rectangle is the removed strip, not the remaining area.

**Cross References:**
- `$API.Rectangle.removeFromTop$`
- `$API.Rectangle.removeFromLeft$`
- `$API.Rectangle.removeFromRight$`
- `$API.Rectangle.withTrimmedBottom$`

## removeFromLeft

**Signature:** `Rectangle removeFromLeft(double numToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var sidebar = {obj}.removeFromLeft(100);`

**Description:**
Removes a strip from the left side of this rectangle. This method mutates the source rectangle by moving its left edge rightward, and returns a new Rectangle containing the removed strip. The returned strip has the same y position and height as the original, with width equal to `numToRemove` and x position at the original left edge. This is part of the "layout slicing" pattern for dividing an area into sub-regions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numToRemove | Double | no | The width of the strip to remove from the left. | Any number |

**Pitfalls:**
- This method mutates the source rectangle. After calling, the source rectangle is narrower (its left edge moves right by `numToRemove`). The returned Rectangle is the removed strip, not the remaining area.

**Cross References:**
- `$API.Rectangle.removeFromRight$`
- `$API.Rectangle.removeFromTop$`
- `$API.Rectangle.removeFromBottom$`
- `$API.Rectangle.withTrimmedLeft$`

## removeFromRight

**Signature:** `Rectangle removeFromRight(double numToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var panel = {obj}.removeFromRight(80);`

**Description:**
Removes a strip from the right side of this rectangle. This method mutates the source rectangle by moving its right edge leftward, and returns a new Rectangle containing the removed strip. The returned strip has the same y position and height as the original, with width equal to `numToRemove` and x position at the new right edge of the source. This is part of the "layout slicing" pattern for dividing an area into sub-regions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numToRemove | Double | no | The width of the strip to remove from the right. | Any number |

**Pitfalls:**
- This method mutates the source rectangle. After calling, the source rectangle is narrower (its right edge moves left by `numToRemove`). The returned Rectangle is the removed strip, not the remaining area.

**Cross References:**
- `$API.Rectangle.removeFromLeft$`
- `$API.Rectangle.removeFromTop$`
- `$API.Rectangle.removeFromBottom$`
- `$API.Rectangle.withTrimmedRight$`

## removeFromTop

**Signature:** `Rectangle removeFromTop(double numToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var header = {obj}.removeFromTop(50);`

**Description:**
Removes a strip from the top of this rectangle. This method mutates the source rectangle by moving its top edge downward, and returns a new Rectangle containing the removed strip. The returned strip has the same x position and width as the original, with height equal to `numToRemove` and y position at the original top edge. This is part of the "layout slicing" pattern for dividing an area into sub-regions by iteratively removing strips from each side.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numToRemove | Double | no | The height of the strip to remove from the top. | Any number |

**Pitfalls:**
- This method mutates the source rectangle. After calling, the source rectangle is smaller (its top edge moves down by `numToRemove`). The returned Rectangle is the removed strip, not the remaining area.

**Cross References:**
- `$API.Rectangle.removeFromBottom$`
- `$API.Rectangle.removeFromLeft$`
- `$API.Rectangle.removeFromRight$`
- `$API.Rectangle.withTrimmedTop$`

## scaled

**Signature:** `Rectangle scaled(double factorX, double optionalFactorY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var half = {obj}.scaled(0.5);`

**Description:**
Returns a new Rectangle with both position and size scaled by the given factor(s). Uses `AffineTransform::scale` internally, which transforms all coordinates (x, y, width, height) by the scale factor. With one argument, both axes are scaled uniformly. With two arguments, the first scales horizontally and the second scales vertically. This differs from simply multiplying width/height because the position is also scaled -- a rectangle at `(100, 50, 200, 100)` scaled by `0.5` becomes `(50, 25, 100, 50)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| factorX | Double | no | Horizontal scale factor (or uniform factor if only one argument). `1.0` = no change, `0.5` = half, `2.0` = double. | Any number |
| optionalFactorY | Double | no | Vertical scale factor. If omitted, uses the first argument for both axes. | Any number |

**Pitfalls:**
- Scaling transforms both position and size. A rectangle not at the origin will move toward or away from `(0, 0)` when scaled. To scale only the size while keeping the centre, use `withSizeKeepingCentre()` instead.

**Cross References:**
- `$API.Rectangle.withSizeKeepingCentre$`
- `$API.Rectangle.withSize$`
- `$API.Rectangle.translated$`
- `$API.Rectangle.expanded$`
- `$API.Rectangle.reduced$`

## isEmpty

**Signature:** `bool isEmpty()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var empty = {obj}.isEmpty();`

**Description:**
Returns `true` if the rectangle's width or height is zero or less. An empty rectangle has no area and cannot contain any points. Useful for checking the result of `getIntersection()` to determine whether two rectangles actually overlap.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Rectangle.getIntersection$`
- `$API.Rectangle.contains$`

## setCentre

**Signature:** `void setCentre(double centerX, double centerY)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setCentre(200, 150);`

**Description:**
Moves this rectangle so that its centre is at the given position, keeping its size unchanged. This is a mutating method -- it modifies the rectangle in place and returns nothing. Delegates to JUCE's `Rectangle::setCentre(double, double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| centerX | Double | no | The x coordinate for the new centre position. | Any number |
| centerY | Double | no | The y coordinate for the new centre position. | Any number |

**Cross References:**
- `$API.Rectangle.withCentre$`
- `$API.Rectangle.setPosition$`

## setPosition

**Signature:** `void setPosition(double x, double y)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setPosition(50, 100);`

**Description:**
Moves this rectangle's top-left corner to the given coordinates, keeping its size unchanged. This is a mutating method -- it modifies the rectangle in place and returns nothing. Delegates to JUCE's `Rectangle::setPosition(double, double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Double | no | The new x coordinate for the top-left corner. | Any number |
| y | Double | no | The new y coordinate for the top-left corner. | Any number |

**Cross References:**
- `$API.Rectangle.setCentre$`
- `$API.Rectangle.withX$`
- `$API.Rectangle.withY$`
- `$API.Rectangle.translated$`

## setSize

**Signature:** `void setSize(double width, double height)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setSize(200, 100);`

**Description:**
Changes this rectangle's width and height, keeping the position of its top-left corner unchanged. This is a mutating method -- it modifies the rectangle in place and returns nothing. Delegates to JUCE's `Rectangle::setSize(double, double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| width | Double | no | The new width of the rectangle. | Any number |
| height | Double | no | The new height of the rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withSize$`
- `$API.Rectangle.withSizeKeepingCentre$`
- `$API.Rectangle.withWidth$`
- `$API.Rectangle.withHeight$`

## toArray

**Signature:** `Array toArray()`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var arr = {obj}.toArray();`

**Description:**
Returns a standard JavaScript array `[x, y, width, height]` representing this rectangle's position and size. Useful for converting a Rectangle object back to the traditional `[x, y, w, h]` array format used by many HISE APIs, or for passing rectangle data to functions that expect arrays.

**Parameters:**

(No parameters.)

## translated

**Signature:** `Rectangle translated(double deltaX, double deltaY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var moved = {obj}.translated(20, -10);`

**Description:**
Returns a new Rectangle that is the same as this one moved by the given offsets. The position is shifted by `deltaX` horizontally and `deltaY` vertically; the size remains unchanged. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::translated(double, double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| deltaX | Double | no | Horizontal offset. Positive values move right, negative values move left. | Any number |
| deltaY | Double | no | Vertical offset. Positive values move down, negative values move up. | Any number |

**Cross References:**
- `$API.Rectangle.setPosition$`
- `$API.Rectangle.scaled$`
- `$API.Rectangle.withX$`
- `$API.Rectangle.withY$`

## withAspectRatioLike

**Signature:** `Rectangle withAspectRatioLike(var otherRect)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var fitted = {obj}.withAspectRatioLike([0, 0, 16, 9]);`

**Description:**
Returns the largest Rectangle that fits within this rectangle while matching the aspect ratio of `otherRect`. The result is centered within this rectangle on the unconstrained axis. For portrait aspect ratios (height > width), the result fills this rectangle's full height and is centered horizontally. For landscape or square aspect ratios, the result fills this rectangle's full width and is centered vertically. Only the aspect ratio of `otherRect` matters -- its position and absolute size are ignored. Accepts the argument as a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherRect | ScriptObject | no | A rectangle whose aspect ratio (height/width) is used as the target ratio. Accepts a Rectangle object, a `[x, y, w, h]` array, or four separate numeric arguments. | Must be a valid rectangle with non-zero width |

**Pitfalls:**
- If the argument cannot be parsed as a rectangle, the method silently returns a copy of the original rectangle unchanged instead of reporting an error.
- [BUG] Passing a rectangle with zero width causes a division by zero when computing the aspect ratio, producing non-finite results.

**Cross References:**
- `$API.Rectangle.withSizeKeepingCentre$`
- `$API.Rectangle.scaled$`

**Example:**
```javascript:fit-aspect-ratio
// Title: Fitting a 16:9 content area inside a square region
var area = Rectangle(0, 0, 300, 300);
var fitted = area.withAspectRatioLike([0, 0, 16, 9]);
// fitted is centered vertically: [0, 65.625, 300, 168.75]
Console.print("x=" + fitted.x + " y=" + fitted.y);
Console.print("w=" + fitted.width + " h=" + fitted.height);
```

```json:testMetadata:fit-aspect-ratio
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "fitted.width", "value": 300},
    {"type": "REPL", "expression": "fitted.height", "value": 168.75}
  ]
}
```

## withBottom

**Signature:** `Rectangle withBottom(double newBottom)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var clipped = {obj}.withBottom(200);`

**Description:**
Returns a new Rectangle with a different bottom edge position but the same top edge as this one. The height is adjusted so that the bottom of the returned rectangle is at `newBottom`. If `newBottom` is less than the current top edge, the resulting rectangle has negative height. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::withBottom(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newBottom | Double | no | The y coordinate for the new bottom edge. | Any number |

**Cross References:**
- `$API.Rectangle.withBottomY$`
- `$API.Rectangle.withRight$`
- `$API.Rectangle.withTrimmedBottom$`

## withBottomY

**Signature:** `Rectangle withBottomY(double newBottomY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var moved = {obj}.withBottomY(400);`

**Description:**
Returns a new Rectangle with the same size and x-position as this one, but moved vertically so that its bottom edge is at `newBottomY`. Unlike `withBottom`, which changes the height while keeping the top edge fixed, this method keeps the size unchanged and repositions the rectangle vertically. The top edge of the returned rectangle is at `newBottomY - height`. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::withBottomY(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newBottomY | Double | no | The y coordinate for the new bottom edge. The rectangle is moved vertically to place its bottom at this position. | Any number |

**Cross References:**
- `$API.Rectangle.withBottom$`
- `$API.Rectangle.withY$`
- `$API.Rectangle.translated$`

## withCentre

**Signature:** `Rectangle withCentre(double newCentreX, double newCentreY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var centered = {obj}.withCentre(200, 150);`

**Description:**
Returns a new Rectangle with the same size as this one, but repositioned so that its centre is at the given coordinates. This is a non-mutating method -- the original rectangle is not modified. Internally constructs a `Point<double>` from the two arguments and delegates to JUCE's `Rectangle::withCentre(Point<double>)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newCentreX | Double | no | The x coordinate for the new centre position. | Any number |
| newCentreY | Double | no | The y coordinate for the new centre position. | Any number |

**Cross References:**
- `$API.Rectangle.setCentre$`
- `$API.Rectangle.withSizeKeepingCentre$`
- `$API.Rectangle.translated$`

## withHeight

**Signature:** `Rectangle withHeight(double newHeight)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var taller = {obj}.withHeight(200);`

**Description:**
Returns a new Rectangle with the same position and width as this one, but with a different height. The top-left corner remains unchanged; only the height changes. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::withHeight(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newHeight | Double | no | The new height for the returned rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withWidth$`
- `$API.Rectangle.withSize$`
- `$API.Rectangle.setSize$`

## withLeft

**Signature:** `Rectangle withLeft(double newLeft)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var shifted = {obj}.withLeft(50);`

**Description:**
Returns a new Rectangle with a different left edge (x position) but the same right edge as this one. The width changes to accommodate the new left position: `newWidth = oldRight - newLeft`. If `newLeft` is greater than the current right edge, the resulting rectangle has negative width. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::withLeft(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newLeft | Double | no | The x coordinate for the new left edge. | Any number |

**Cross References:**
- `$API.Rectangle.withRight$`
- `$API.Rectangle.withX$`
- `$API.Rectangle.withTrimmedLeft$`

## withRight

**Signature:** `Rectangle withRight(double newRight)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var wider = {obj}.withRight(500);`

**Description:**
Returns a new Rectangle with a different right edge position but the same left edge (x position) as this one. The width changes to accommodate the new right position: `newWidth = newRight - oldLeft`. If `newRight` is less than the current left edge, the resulting rectangle has negative width. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::withRight(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newRight | Double | no | The x coordinate for the new right edge. | Any number |

**Cross References:**
- `$API.Rectangle.withLeft$`
- `$API.Rectangle.withWidth$`
- `$API.Rectangle.withTrimmedRight$`

## withSize

**Signature:** `Rectangle withSize(double newWidth, double newHeight)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var resized = {obj}.withSize(300, 200);`

**Description:**
Returns a new Rectangle with the same top-left position as this one, but with a different width and height. The x and y coordinates are preserved; only the dimensions change. This is a non-mutating method -- the original rectangle is not modified. To resize while keeping the centre unchanged, use `withSizeKeepingCentre()` instead. Delegates to JUCE's `Rectangle::withSize(double, double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newWidth | Double | no | The new width for the returned rectangle. | Any number |
| newHeight | Double | no | The new height for the returned rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withSizeKeepingCentre$`
- `$API.Rectangle.setSize$`
- `$API.Rectangle.withWidth$`
- `$API.Rectangle.withHeight$`

## withSizeKeepingCentre

**Signature:** `Rectangle withSizeKeepingCentre(double newWidth, double newHeight)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var centered = {obj}.withSizeKeepingCentre(100, 80);`

**Description:**
Returns a new Rectangle with the same centre position as this one, but with a different width and height. The rectangle is resized symmetrically around its centre point. This is a non-mutating method -- the original rectangle is not modified. This is useful for creating centered sub-regions within a layout area. To resize while keeping the top-left corner unchanged, use `withSize()` instead. Delegates to JUCE's `Rectangle::withSizeKeepingCentre(double, double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newWidth | Double | no | The new width for the returned rectangle. | Any number |
| newHeight | Double | no | The new height for the returned rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withSize$`
- `$API.Rectangle.withCentre$`
- `$API.Rectangle.setCentre$`
- `$API.Rectangle.scaled$`

## withTrimmedBottom

**Signature:** `Rectangle withTrimmedBottom(double amountToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var trimmed = {obj}.withTrimmedBottom(20);`

**Description:**
Returns a new Rectangle with the given amount removed from its bottom edge. The top edge, x position, and width remain unchanged; only the height is reduced by `amountToRemove`. This is a non-mutating method -- the original rectangle is not modified. Unlike `removeFromBottom`, this method does not return the removed strip and does not mutate the source. Delegates to JUCE's `Rectangle::withTrimmedBottom(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| amountToRemove | Double | no | The amount to trim from the bottom edge. Positive values shrink the rectangle; negative values extend it downward. | Any number |

**Cross References:**
- `$API.Rectangle.removeFromBottom$`
- `$API.Rectangle.withTrimmedTop$`
- `$API.Rectangle.withTrimmedLeft$`
- `$API.Rectangle.withTrimmedRight$`
- `$API.Rectangle.reduced$`

## withTrimmedLeft

**Signature:** `Rectangle withTrimmedLeft(double amountToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var trimmed = {obj}.withTrimmedLeft(20);`

**Description:**
Returns a new Rectangle with the given amount removed from its left-hand edge. The right edge, y position, and height remain unchanged; the x position moves rightward by `amountToRemove` and the width decreases by the same amount. This is a non-mutating method -- the original rectangle is not modified. Unlike `removeFromLeft`, this method does not return the removed strip and does not mutate the source. Delegates to JUCE's `Rectangle::withTrimmedLeft(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| amountToRemove | Double | no | The amount to trim from the left edge. Positive values shrink the rectangle from the left; negative values extend it leftward. | Any number |

**Cross References:**
- `$API.Rectangle.removeFromLeft$`
- `$API.Rectangle.withTrimmedRight$`
- `$API.Rectangle.withTrimmedTop$`
- `$API.Rectangle.withTrimmedBottom$`
- `$API.Rectangle.reduced$`

## withTrimmedRight

**Signature:** `Rectangle withTrimmedRight(double amountToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var trimmed = {obj}.withTrimmedRight(20);`

**Description:**
Returns a new Rectangle with the given amount removed from its right-hand edge. The left edge (x position), y position, and height remain unchanged; only the width decreases by `amountToRemove`. This is a non-mutating method -- the original rectangle is not modified. Unlike `removeFromRight`, this method does not return the removed strip and does not mutate the source. Delegates to JUCE's `Rectangle::withTrimmedRight(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| amountToRemove | Double | no | The amount to trim from the right edge. Positive values shrink the rectangle from the right; negative values extend it rightward. | Any number |

**Cross References:**
- `$API.Rectangle.removeFromRight$`
- `$API.Rectangle.withTrimmedLeft$`
- `$API.Rectangle.withTrimmedTop$`
- `$API.Rectangle.withTrimmedBottom$`
- `$API.Rectangle.reduced$`

## withTrimmedTop

**Signature:** `Rectangle withTrimmedTop(double amountToRemove)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var trimmed = {obj}.withTrimmedTop(20);`

**Description:**
Returns a new Rectangle with the given amount removed from its top edge. The bottom edge, x position, and width remain unchanged; the y position moves downward by `amountToRemove` and the height decreases by the same amount. This is a non-mutating method -- the original rectangle is not modified. Unlike `removeFromTop`, this method does not return the removed strip and does not mutate the source. Delegates to JUCE's `Rectangle::withTrimmedTop(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| amountToRemove | Double | no | The amount to trim from the top edge. Positive values shrink the rectangle from the top; negative values extend it upward. | Any number |

**Cross References:**
- `$API.Rectangle.removeFromTop$`
- `$API.Rectangle.withTrimmedBottom$`
- `$API.Rectangle.withTrimmedLeft$`
- `$API.Rectangle.withTrimmedRight$`
- `$API.Rectangle.reduced$`

## withWidth

**Signature:** `Rectangle withWidth(double newWidth)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var wider = {obj}.withWidth(300);`

**Description:**
Returns a new Rectangle with the same position and height as this one, but with a different width. The top-left corner (x, y) and height remain unchanged; only the width changes. This is a non-mutating method -- the original rectangle is not modified. Delegates to JUCE's `Rectangle::withWidth(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newWidth | Double | no | The new width for the returned rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withHeight$`
- `$API.Rectangle.withSize$`
- `$API.Rectangle.setSize$`
- `$API.Rectangle.withRight$`

## withX

**Signature:** `Rectangle withX(double newX)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var moved = {obj}.withX(50);`

**Description:**
Returns a new Rectangle with the same size and y-position as this one, but with a different x-position. The width, height, and y coordinate remain unchanged; only the x coordinate changes. This is a non-mutating method -- the original rectangle is not modified. Unlike `withLeft`, which keeps the right edge fixed and adjusts the width, this method preserves the width and moves the entire rectangle horizontally. Delegates to JUCE's `Rectangle::withX(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newX | Double | no | The new x coordinate for the left edge of the returned rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withY$`
- `$API.Rectangle.withLeft$`
- `$API.Rectangle.setPosition$`
- `$API.Rectangle.translated$`

## withY

**Signature:** `Rectangle withY(double newY)`
**Return Type:** `ScriptObject`
**Call Scope:** safe
**Minimal Example:** `var moved = {obj}.withY(100);`

**Description:**
Returns a new Rectangle with the same size and x-position as this one, but with a different y-position. The width, height, and x coordinate remain unchanged; only the y coordinate changes. This is a non-mutating method -- the original rectangle is not modified. Unlike `withBottomY`, which positions the rectangle by its bottom edge, this method positions it by its top edge. Delegates to JUCE's `Rectangle::withY(double)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newY | Double | no | The new y coordinate for the top edge of the returned rectangle. | Any number |

**Cross References:**
- `$API.Rectangle.withX$`
- `$API.Rectangle.withBottomY$`
- `$API.Rectangle.setPosition$`
- `$API.Rectangle.translated$`
