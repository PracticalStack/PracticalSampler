# Path (PathObject) -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (Path entry)
- `enrichment/resources/survey/class_survey.md` (prerequisite table -- Path has no prerequisites)
- No prerequisite Readme files needed
- No base class explorations needed (PathObject inherits ConstScriptingObject directly)

## Class Declaration

**Header:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.h` (lines 416-553)

```cpp
class PathObject : public ConstScriptingObject
{
public:
    PathObject(ProcessorWithScriptingContent* p);
    ~PathObject();

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Path"); }
    String getDebugName() const override { return "Path"; }
    String getDebugValue() const override { return p.getBounds().toString(); }
    Component* createPopupComponent(const MouseEvent& e, Component *c) override;

    // ... 31 API methods ...

    struct Wrapper;

    Path& getPath() { return p; }
    const Path& getPath() const { return p; }

private:
    bool useRectangleClass = false;
    Path p;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PathObject);
};
```

### Inheritance
- `ConstScriptingObject` -- standard HISE scripting API base class for immutable API objects (no property get/set interface)
- No inner types, no nested classes, no listener patterns
- The class wraps a single `juce::Path` member `p`

### Key Members
- `Path p` -- the underlying JUCE Path object that all methods operate on
- `bool useRectangleClass` -- determines whether `getBounds()` returns a `[x,y,w,h]` array or a `ScriptRectangle` object. Initialized from `HISE_USE_SCRIPT_RECTANGLE_OBJECT` preprocessor define at construction time.

## Constructor Analysis

**Implementation:** `ScriptingGraphics.cpp` (lines 1025-1062)

```cpp
ScriptingObjects::PathObject::PathObject(ProcessorWithScriptingContent* p) :
    ConstScriptingObject(p, 0)  // 0 constants
{
    ADD_API_METHOD_1(loadFromData);
    ADD_API_METHOD_0(closeSubPath);
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_2(startNewSubPath);
    ADD_API_METHOD_2(lineTo);
    ADD_API_METHOD_4(quadraticTo);
    ADD_API_METHOD_4(cubicTo);
    ADD_API_METHOD_4(addQuadrilateral);
    ADD_API_METHOD_3(addArc);
    ADD_API_METHOD_4(addPieSegment);
    ADD_API_METHOD_1(addEllipse);
    ADD_API_METHOD_1(addRectangle);
    ADD_API_METHOD_2(addRoundedRectangle);
    ADD_API_METHOD_3(addRoundedRectangleCustomisable);
    ADD_API_METHOD_3(addTriangle);
    ADD_API_METHOD_4(addPolygon);
    ADD_API_METHOD_5(addArrow);
    ADD_API_METHOD_5(addStar);
    ADD_API_METHOD_5(scaleToFit);
    ADD_API_METHOD_1(roundCorners);
    ADD_API_METHOD_1(getPointOnPath);
    ADD_API_METHOD_3(getIntersection);
    ADD_API_METHOD_1(contains);
    ADD_API_METHOD_1(getBounds);
    ADD_API_METHOD_1(setBounds);
    ADD_API_METHOD_0(getLength);
    ADD_API_METHOD_0(getRatio);
    ADD_API_METHOD_2(createStrokedPath);
    ADD_API_METHOD_0(toString);
    ADD_API_METHOD_0(toBase64);
    ADD_API_METHOD_1(fromString);
    ADD_API_METHOD_1(getYAt);

    useRectangleClass = HISE_GET_PREPROCESSOR(
        getScriptProcessor()->getMainController_(), HISE_USE_SCRIPT_RECTANGLE_OBJECT);
}
```

### Key observations:
- **0 constants** -- no `addConstant()` calls
- **ALL methods use `ADD_API_METHOD_N`** (plain, untyped) -- no `ADD_TYPED_API_METHOD_N` usage at all
- **31 API methods** registered
- The `useRectangleClass` flag is read from the project's preprocessor definitions at construction time via `HISE_GET_PREPROCESSOR`

## Wrapper Struct (Method Registration)

All methods use the standard `API_VOID_METHOD_WRAPPER_N` or `API_METHOD_WRAPPER_N` macros. No typed wrappers.

```cpp
struct ScriptingObjects::PathObject::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(PathObject, loadFromData);
    API_VOID_METHOD_WRAPPER_0(PathObject, closeSubPath);
    API_VOID_METHOD_WRAPPER_2(PathObject, startNewSubPath);
    API_VOID_METHOD_WRAPPER_2(PathObject, lineTo);
    API_VOID_METHOD_WRAPPER_0(PathObject, clear);
    API_VOID_METHOD_WRAPPER_4(PathObject, quadraticTo);
    API_VOID_METHOD_WRAPPER_4(PathObject, cubicTo);
    API_VOID_METHOD_WRAPPER_4(PathObject, addQuadrilateral);
    API_VOID_METHOD_WRAPPER_3(PathObject, addArc);
    API_VOID_METHOD_WRAPPER_4(PathObject, addPieSegment);
    API_VOID_METHOD_WRAPPER_1(PathObject, addEllipse);
    API_VOID_METHOD_WRAPPER_1(PathObject, addRectangle);
    API_VOID_METHOD_WRAPPER_2(PathObject, addRoundedRectangle);
    API_VOID_METHOD_WRAPPER_3(PathObject, addRoundedRectangleCustomisable);
    API_VOID_METHOD_WRAPPER_3(PathObject, addTriangle);
    API_VOID_METHOD_WRAPPER_4(PathObject, addPolygon);
    API_VOID_METHOD_WRAPPER_5(PathObject, addArrow);
    API_VOID_METHOD_WRAPPER_5(PathObject, addStar);
    API_VOID_METHOD_WRAPPER_5(PathObject, scaleToFit);
    API_VOID_METHOD_WRAPPER_1(PathObject, roundCorners);
    API_METHOD_WRAPPER_2(PathObject, createStrokedPath);
    API_METHOD_WRAPPER_3(PathObject, getIntersection);
    API_METHOD_WRAPPER_1(PathObject, getPointOnPath);
    API_METHOD_WRAPPER_1(PathObject, contains);
    API_METHOD_WRAPPER_1(PathObject, getBounds);
    API_VOID_METHOD_WRAPPER_1(PathObject, setBounds);
    API_METHOD_WRAPPER_0(PathObject, getLength);
    API_METHOD_WRAPPER_0(PathObject, getRatio);
    API_METHOD_WRAPPER_0(PathObject, toString);
    API_METHOD_WRAPPER_0(PathObject, toBase64);
    API_METHOD_WRAPPER_1(PathObject, getYAt);
    API_VOID_METHOD_WRAPPER_1(PathObject, fromString);
};
```

## Factory / obtainedVia

Path objects are created via `Content.createPath()`:

```cpp
// ScriptingApiContent.cpp line 8460
var ScriptingApi::Content::createPath()
{
    ScriptingObjects::PathObject* obj = new ScriptingObjects::PathObject(getScriptProcessor());
    return var(obj);
}
```

Path objects are also created internally in many other contexts:
- **LAF callback objects**: LAF functions for filter graphs, knob paths, waveform paths etc. create PathObject instances and attach them as properties on the callback DynamicObject (e.g., `obj->setProperty("path", keeper)`)
- **DisplayBuffer.createPath()**: Creates a PathObject from ring buffer data for visualization
- **LorisManager**: Creates PathObject instances from envelope data
- **createStrokedPath()**: Returns a new PathObject (the stroked version of the current path)

## Helper Functions Used by PathObject

### `ApiHelpers::getRectangleFromVar(const var& data, Result* r)`
Converts a `var` to `Rectangle<float>`. Accepts either:
- A 4-element array `[x, y, width, height]`
- A `RectangleDynamicObject` (the new Rectangle scripting object)

All float values are sanitized via `SANITIZED()`.

### `ApiHelpers::getPointFromVar(const var& data, Result* r)`
Converts a `var` to `Point<float>`. Expects a 2-element array `[x, y]`.

### `ApiHelpers::loadPathFromData(Path& p, var data)`
Loads path data from three possible formats:
1. **String** -- treated as base64 encoded path data. Decoded via `MemoryBlock::fromBase64Encoding()` then loaded via `Path::loadPathFromData()`
2. **Array** -- treated as raw unsigned char data. Each element is cast to `unsigned char` and loaded via `Path::loadPathFromData()`
3. **PathObject** -- extracts the internal `juce::Path` directly via `sp->getPath()`

### `ApiHelpers::createPathStrokeType(var strokeType)`
Creates a `PathStrokeType` from either:
1. **DynamicObject** with properties:
   - `"Thickness"` (float)
   - `"EndCapStyle"` -- one of: `"butt"`, `"square"`, `"rounded"` (mapped to enum index 0, 1, 2)
   - `"JointStyle"` -- one of: `"mitered"`, `"curved"`, `"beveled"` (mapped to enum index 0, 1, 2)
2. **Numeric value** -- used directly as thickness (simple stroke)

Default thickness is 1.0f if not specified.

### `ApiHelpers::getVarRectangle(bool useRectangleClass, Rectangle<float> r, Result*)`
Converts a `Rectangle<float>` back to a `var`:
- If `useRectangleClass` is true, returns a `ScriptRectangle` object
- Otherwise, returns a 4-element array `[x, y, width, height]`

### `ApiHelpers::convertStyleSheetProperty(value, "path")`
When a Path object is used as a CSS/StyleSheet property, it is automatically converted to its base64 representation via `p->toBase64()`.

## SANITIZED Macro

```cpp
// ScriptMacroDefinitions.h line 122
#define SANITIZED(x) FloatSanitizers::sanitizeFloatNumber(x)
```

Applied to float coordinates in `startNewSubPath()`, `lineTo()`, `addArc()`, `addPieSegment()`, and `createPathStrokeType()`. This guards against NaN/Inf values being passed to the JUCE Path.

Note: NOT applied consistently -- `quadraticTo()` and `cubicTo()` pass values directly to JUCE Path without sanitization.

## HISE_USE_SCRIPT_RECTANGLE_OBJECT Preprocessor

```cpp
// hi_scripting.h lines 119-127
/** Config: HISE_USE_SCRIPT_RECTANGLE_OBJECT
 *
 *  Enable this to use the custom rectangle object type instead of a JS
 *  array in LAF methods and other scripting callbacks.
 */
#ifndef HISE_USE_SCRIPT_RECTANGLE_OBJECT
#define HISE_USE_SCRIPT_RECTANGLE_OBJECT 0
```

Default is OFF (0). When enabled, `getBounds()` returns a `Rectangle` scripting object instead of a `[x,y,w,h]` array. This is a project-level configuration.

The value is read at PathObject construction time via:
```cpp
useRectangleClass = HISE_GET_PREPROCESSOR(
    getScriptProcessor()->getMainController_(), HISE_USE_SCRIPT_RECTANGLE_OBJECT);
```

`HISE_GET_PREPROCESSOR` is:
- In backend: `mc->getExtraDefinitionsValue(#x, x)` -- reads from project's extra definitions
- In frontend: just the compile-time `x` value

## Backend Debug Support

### PathPreviewComponent (USE_BACKEND only)

```cpp
Component* ScriptingObjects::PathObject::createPopupComponent(const MouseEvent &e, Component* componentToNotify)
{
#if USE_BACKEND
    return new PathPreviewComponent(this);
#else
    ignoreUnused(e, componentToNotify);
    return nullptr;
#endif
}
```

The `PathPreviewComponent` (ScriptingGraphics.cpp lines 921-976) renders a visual preview of the path in the HISE IDE. It:
- Shows the path filled (white 50% alpha) and stroked (white 80% alpha, 2px)
- Displays the bounding box corner coordinates
- Is resizable
- Inherits from `ComponentForDebugInformation` for live-update capability

The `getDebugValue()` override returns the bounding box as a string: `p.getBounds().toString()`.

## How Graphics Consumes Path

The Path object is consumed by `GraphicsObject` methods:

### `Graphics.fillPath(path, area)`
```cpp
void ScriptingObjects::GraphicsObject::fillPath(var path, var area)
{
    if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
    {
        Path p = pathObject->getPath();  // copies the path
        if (p.getBounds().isEmpty()) return;

        if (area.isArray() || dynamic_cast<ScriptRectangle*>(area.getDynamicObject()))
        {
            Rectangle<float> r = getRectangleFromVar(area);
            p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
        }

        drawActionHandler.addDrawAction(new ScriptedDrawActions::fillPath(p));
    }
}
```

Key behavior: if `area` is provided (as array or Rectangle), the path is scaled to fit that area (without preserving proportions). If `area` is not an array/rectangle, the path is drawn at its original coordinates.

### `Graphics.drawPath(path, area, strokeType)`
Similar to `fillPath` but also accepts a `strokeType` parameter (via `ApiHelpers::createPathStrokeType`).

### `Graphics.applyMask(path, area, invert)`
Uses the path as a mask on the current layer. Requires a layer to be active.

### `Graphics.drawDropShadowFromPath(path, area, colour, radius, offset)`
### `Graphics.drawInnerShadowFromPath(path, area, colour, radius, offset)`
Both use melatonin shadow library with the path geometry.

## Method Implementation Patterns

### Array-based coordinate methods
Most shape-adding methods that accept coordinates as arrays use `ApiHelpers::getPointFromVar()` for `[x, y]` points and `ApiHelpers::getRectangleFromVar()` for `[x, y, w, h]` areas.

**Point-based methods:** `addPolygon`, `addArrow`, `addStar`, `getIntersection`, `contains`, `getPointOnPath`
**Rectangle-based methods:** `addArc`, `addPieSegment`, `addEllipse`, `addRectangle`, `addRoundedRectangle`, `addRoundedRectangleCustomisable`
**Direct-indexed array methods:** `cubicTo` (uses `cxy1[0]`, `cxy1[1]`), `addQuadrilateral`, `addTriangle`

### cubicTo parameter convention
The `cubicTo` method in the header declares `(var cxy1, var cxy2, var x, var y)` where cxy1 and cxy2 are arrays, but x and y are scalars:
```cpp
p.cubicTo(cxy1[0], cxy1[1], cxy2[0], cxy2[1], x, y);
```
This is a mixed convention: first two params are `[x,y]` arrays, last two are separate x,y scalars.

### quadraticTo parameter convention
Takes 4 separate scalar values `(cx, cy, x, y)`:
```cpp
p.quadraticTo(cx, cy, x, y);
```

### getYAt implementation detail
Uses `flex_ahdsr_base::Helpers::getYAt()` -- a utility from the envelope node library that uses `PathFlatteningIterator` to walk the path and find the Y value at a given X position. Returns -1.0f if no match, which PathObject converts to `var()` (undefined). Returns `var(x)` if found.

### getIntersection subtle behavior
```cpp
Line<float> l(p1.getX(), p1.getY() - 0.001f, p2.getX(), p2.getY());
```
Note the `-0.001f` offset on the start Y coordinate, with the comment: "ensures the line starts inside the path boundaries... Hmmm..." -- this is a deliberate workaround for edge cases.

Returns `[x, y]` array of the intersection point, or `false` if no intersection. When `keepSectionOutsidePath` is true, returns the start point of the clipped line; when false, returns the end point.

### createStrokedPath
Returns a NEW PathObject (allocated with `new PathObject(getScriptProcessor())`). The new path's bounds are explicitly set by adding invisible subpaths at the original path's top-left and bottom-right corners:
```cpp
np->p.startNewSubPath(p.getBounds().getTopLeft());
np->p.startNewSubPath(p.getBounds().getBottomRight());
```
This ensures the stroked path inherits the original path's bounding box.

Supports optional dash pattern via `dotData` parameter (array of floats).

### setBounds implementation
Does NOT resize the path to fit the bounding box. Instead, it adds invisible start points at the corners of the bounding box to expand the path's bounds:
```cpp
void ScriptingObjects::PathObject::setBounds(var boundingBox)
{
    auto r = Result::ok();
    auto tb = ApiHelpers::getRectangleFromVar(boundingBox, &r);
    if(r.failed())
        reportScriptError(r.getErrorMessage());
    p.startNewSubPath(tb.getTopLeft());
    p.startNewSubPath(tb.getBottomRight());
}
```
This is a "minimal" bounding box setter -- it sets the path's reported bounds without affecting visible geometry.

### scaleToFit implementation
Applies a transform rather than modifying the path data directly:
```cpp
p.applyTransform(p.getTransformToScaleToFit(x, y, width, height, preserveProportions));
```

### roundCorners implementation
Replaces the internal path entirely:
```cpp
p = p.createPathWithRoundedCorners(radius);
```

### toString / fromString / toBase64
- `toString()` -- delegates to `juce::Path::toString()` which produces a human-readable string representation
- `fromString()` -- delegates to `juce::Path::restoreFromString()`
- `toBase64()` -- writes path to a `MemoryOutputStream` via `writePathToStream()`, then base64-encodes the memory block

## Threading and Lifecycle

- No threading constraints. Path is a data-only object with no audio thread interaction.
- Can be created any time (not restricted to onInit).
- Used primarily in paint callbacks (UI thread), but the object itself can be constructed and manipulated anywhere.
- No locks, no atomic operations, no thread synchronization.
- The Path is typically created once in `onInit` and reused across paint calls for performance.

## Consumers of PathObject (Beyond Graphics)

1. **LAF callback objects** -- Many LAF functions create PathObject instances and pass them as properties:
   - `drawFilterPath` -- `obj.path` contains the filter response curve
   - `drawFilterGridLines` -- `obj.grid` contains the grid lines path
   - Knob paths, waveform paths, etc.
   
2. **DisplayBuffer.createPath()** -- Creates a PathObject from ring buffer audio data for visualization

3. **LorisManager** -- Creates PathObject instances from envelope analysis data

4. **CSS/StyleSheet integration** -- `ApiHelpers::convertStyleSheetProperty` converts Path objects to base64 for the `"path"` CSS type

5. **ScriptComponent pathIcon property** -- Components like ScriptButton can display a path icon. Checked via:
   ```cpp
   if (auto po = dynamic_cast<ScriptingObjects::PathObject*>(pathIcon.getObject()))
   ```

## Related Preprocessors
- `USE_BACKEND` -- controls `PathPreviewComponent` debug popup availability
- `HISE_USE_SCRIPT_RECTANGLE_OBJECT` -- affects return type of `getBounds()` (array vs Rectangle object)

## Survey Data Context

From `class_survey_data.json`:
- **domain:** `ui`
- **role:** `utility`
- **createdBy:** `Content`
- **fanIn:** 0.3 (used by many classes)
- **references:** `Graphics`, `Rectangle`
- **seeAlso:**
  - `Rectangle` -- "Use Path for complex vector shapes with curves and multiple sub-paths; use Rectangle for simple axis-aligned rectangular geometry."
  - `Graphics` -- "Path defines shape geometry; Graphics renders those shapes onto a surface."
