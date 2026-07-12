# Rectangle -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- no prerequisites for Rectangle
- `enrichment/resources/survey/class_survey_data.json` -- Rectangle entry (domain: ui, role: utility, seeAlso: Path)
- `enrichment/base/Rectangle.json` -- 31 API methods

## Class Location and Inheritance

### Base Class: RectangleDynamicObject
**File:** `HISE/hi_tools/hi_tools/RectangleDynamicObject.h`

```cpp
class RectangleDynamicObject: public DynamicObject,
                              public DebugableObjectBase
```

Key members:
- `Rectangle<double> rectangle` -- the underlying JUCE rectangle (protected)
- `SharedResourcePointer<FunctionMap> functionMap` -- shared function dispatch table (protected)
- Constructor: `RectangleDynamicObject(Rectangle<double> d={})` (protected)
- Overrides from DynamicObject: `hasProperty`, `getProperty`, `setProperty`, `removeProperty` (no-op), `hasMethod`, `invokeMethod`, `clone`, `writeAsJSON`
- `getRectangle() const` returns the underlying `Rectangle<double>`
- `toString()` delegates to `rectangle.toString()`

### Scripting Class: ScriptRectangle
**File:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.h` (line 72)

```cpp
class ScriptRectangle: public RectangleDynamicObject,
                       public AssignableObject,
                       public AssignableDotObject
```

This is the actual scripting API class exposed as `"Rectangle"` to HiseScript. It extends RectangleDynamicObject with:
- `AssignableObject` -- enables indexed assignment (e.g., `rect[0] = 10`)
- `AssignableDotObject` -- enables dot-property assignment (e.g., `rect.x = 10`)

**Object Name:** Returns `"Rectangle"` via `RETURN_STATIC_IDENTIFIER("Rectangle")`

---

## Architecture: Unique DynamicObject-Based Design

Rectangle is architecturally unique among HISE scripting API classes. It does NOT extend `ConstScriptingObject` or `ScriptingObject`. Instead, it extends JUCE's `DynamicObject` directly, which makes it behave like a native JavaScript object in the JUCE scripting engine.

### Why DynamicObject?

The Rectangle class needs to:
1. Act as a drop-in replacement for the `[x, y, w, h]` array convention used throughout HISE
2. Support dot-property access (`rect.x`, `rect.width`) and index access (`rect[0]`)
3. Be passable to any API function that previously accepted `var` arrays for rectangle data
4. Be transparently convertible back and forth with arrays

By extending DynamicObject, Rectangle objects can be stored in `var` and passed to any function expecting a `var`. The `hasProperty`/`getProperty`/`setProperty` overrides make it behave like an object with `x`, `y`, `width`, `height` properties, while `hasMethod`/`invokeMethod` dispatches all method calls through the shared FunctionMap.

### No Constructor Registration / No ADD_API_METHOD Pattern

Unlike standard scripting objects, Rectangle does NOT use `ADD_API_METHOD_N` macros or `addFunction`/`addConstant` patterns. All methods are registered in the `FunctionMap` constructor using native function lambdas. The header file's method declarations (lines 131-231 in ScriptingGraphics.h) are documentation stubs only -- they all contain `jassertfalse;` and are never called. Their sole purpose is to provide Doxygen descriptions for the API generator.

---

## Factory / obtainedVia

### Primary Factory: Global `Rectangle()` Function

The Rectangle constructor is registered as a global function on the JavaScript engine's root object:

**File:** `HISE/hi_scripting/scripting/engine/JavascriptEngineAdditionalMethods.cpp` (line 282)
```cpp
setMethod("Rectangle", ApiHelpers::createRectangle);
```

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` (line 6923)
```cpp
var ApiHelpers::createRectangle(const var::NativeFunctionArgs& a)
{
    if(a.numArguments == 1 && a.arguments[0].isArray() && a.arguments[0].size() == 4)
        return var(new ScriptingObjects::ScriptRectangle(a.arguments[0]));
    else if(a.numArguments == 2)
        return var(new ScriptingObjects::ScriptRectangle(
            Rectangle<double>((double)a.arguments[0], (double)a.arguments[1])));
    else if(a.numArguments == 4)
        return var(new ScriptingObjects::ScriptRectangle(
            Rectangle<double>((double)a.arguments[0], (double)a.arguments[1],
                               (double)a.arguments[2], (double)a.arguments[3])));
    else
        return var(new ScriptingObjects::ScriptRectangle(Rectangle<double>()));
}
```

**Constructor overloads:**
1. `Rectangle([x, y, w, h])` -- from a 4-element array
2. `Rectangle(w, h)` -- width and height only (position 0,0)
3. `Rectangle(x, y, w, h)` -- full specification
4. `Rectangle()` -- empty rectangle (0, 0, 0, 0)

### Secondary Source: Return Values from Other APIs

Rectangle objects are returned from various APIs when the `HISE_USE_SCRIPT_RECTANGLE_OBJECT` preprocessor is enabled. The selection logic is in `ApiHelpers::getVarRectangle`:

```cpp
var ApiHelpers::getVarRectangle(bool useRectangleClass, Rectangle<float> floatRectangle, Result* r)
{
    if(useRectangleClass)
        return var(new ScriptingObjects::ScriptRectangle(floatRectangle.toDouble()));
    // else returns [x, y, w, h] array
}
```

This means Rectangle objects appear as return values from:
- `ScriptComponent.getLocalBounds()` and child class equivalents
- All LAF callback `obj.area` properties
- `MidiPlayer.getEventListAsNoteRectangles()` and `getNoteRectangleList()`
- Any other API that constructs area rectangles for scripting

---

## HISE_USE_SCRIPT_RECTANGLE_OBJECT Preprocessor

**File:** `HISE/hi_scripting/hi_scripting.h` (lines 119-127)

```cpp
/** Config: HISE_USE_SCRIPT_RECTANGLE_OBJECT
 *
 *  Enable this to use the custom rectangle object type instead of a JS
 *  array in LAF methods and other scripting callbacks.
 *
 */
#ifndef HISE_USE_SCRIPT_RECTANGLE_OBJECT
#define HISE_USE_SCRIPT_RECTANGLE_OBJECT 0
#endif
```

**Default: disabled (0).** When enabled, `HISE_GET_PREPROCESSOR` reads the runtime value from the project's extra definitions. This is a per-project setting, not a compile-time define -- the macro `HISE_GET_PREPROCESSOR(mc, x)` resolves to `mc->getExtraDefinitionsValue(#x, x)` in backend builds, allowing it to be toggled without recompilation.

The flag is checked in:
- `ScriptingObjects::ScriptedLookAndFeel::Laf` constructor (line 4072 of ScriptingGraphics.cpp)
- `ScriptingObjects::GraphicsObject` constructor (line 1061 of ScriptingGraphics.cpp)
- `ScriptComponent` constructor (line 504 of ScriptingApiContent.cpp)
- `MidiPlayer` event rectangle methods (ScriptingApiObjects.cpp)

---

## FunctionMap: Shared Dispatch Table

**File:** `HISE/hi_tools/hi_tools/RectangleDynamicObject.cpp`

The FunctionMap is a `SharedResourcePointer`, meaning a single instance is shared across ALL Rectangle objects. It stores `std::map<Identifier, var::NativeFunction>` mapping method names to lambda implementations.

### Registration Macros

```cpp
#define ADD_FUNCTION(x, argString, body)  functions[Identifier(#x)] = body;
#define ADD_FUNCTION1(func, argString)    functions[Identifier(#func)] = 
    [](const Args& a) { return create(a, getRectangle(a).func(getDoubleArgs(a, 0))); };
#define ADD_FUNCTION2(func, argString)    functions[Identifier(#func)] = 
    [](const Args& a) { return create(a, getRectangle(a).func(getDoubleArgs(a, 0), 
                                                                getDoubleArgs(a, 1))); };
#define ADD_VOID_FUNCTION2(func, argString) functions[Identifier(#func)] = 
    [](const Args& a) { getRectangle(a).func(getDoubleArgs(a, 0), 
                                              getDoubleArgs(a, 1)); return var(); };
```

- `ADD_FUNCTION1` -- single-arg methods that return a new Rectangle (delegates to JUCE Rectangle method)
- `ADD_FUNCTION2` -- two-arg methods that return a new Rectangle
- `ADD_VOID_FUNCTION2` -- two-arg methods that modify the rectangle in place (return void)
- `ADD_FUNCTION` -- custom lambda body for complex logic

### Method Categories by Implementation Pattern

**Single-arg returning new Rectangle (ADD_FUNCTION1):**
- `removeFromTop`, `removeFromLeft`, `removeFromRight`, `removeFromBottom`
- `withX`, `withY`, `withLeft`, `withRight`, `withBottom`, `withBottomY`
- `withWidth`, `withHeight`
- `withTrimmedBottom`, `withTrimmedLeft`, `withTrimmedRight`, `withTrimmedTop`

**Two-arg returning new Rectangle (ADD_FUNCTION2):**
- `withSize`, `withSizeKeepingCentre`, `translated`

**Two-arg modifying in-place (ADD_VOID_FUNCTION2):**
- `setPosition`, `setSize`, `setCentre`

**Custom implementations (ADD_FUNCTION with lambda):**
- `withCentre` -- takes two doubles, constructs Point for JUCE's withCentre(Point)
- `scaled` -- 1 or 2 args, uses AffineTransform::scale
- `constrainedWithin` -- takes rectangle arg via getRectangleArgs
- `contains` -- polymorphic: accepts rectangle OR point
- `intersects` -- takes rectangle arg
- `getUnion` -- takes rectangle arg, returns new Rectangle
- `getIntersection` -- takes rectangle arg, uses intersectRectangle
- `reduced` -- 1 or 2 args
- `expanded` -- 1 or 2 args
- `withAspectRatioLike` -- custom aspect ratio calculation
- `isEmpty` -- returns bool
- `toArray` -- returns [x, y, w, h] array
- `toString` -- returns string representation

---

## Mutability Semantics

Rectangle methods fall into two categories:

### Mutating Methods (modify `this`, return void)
- `removeFromTop`, `removeFromLeft`, `removeFromRight`, `removeFromBottom` -- these call `rectangle.removeFromTop()` etc. which modifies the rectangle AND returns the removed strip as a new Rectangle
- `setPosition`, `setSize`, `setCentre` -- modify in place, return void

### Non-Mutating Methods (return new Rectangle, leave `this` unchanged)
- All `with*` methods, `translated`, `scaled`, `reduced`, `expanded`, `constrainedWithin`, `getUnion`, `getIntersection`, `withAspectRatioLike`

**Important behavioral note for removeFrom* methods:** The JUCE `Rectangle::removeFromTop()` etc. methods are inherently mutating -- they shrink the source rectangle and return the removed strip. The FunctionMap uses `ADD_FUNCTION1` which calls `create(a, getRectangle(a).removeFromTop(arg))`. This both:
1. Modifies the source rectangle (shrinks it)
2. Returns a new ScriptRectangle containing the removed strip

This is the classic "layout slicing" pattern from JUCE.

### The `create` Helper

```cpp
var RectangleDynamicObject::FunctionMap::create(const Args& a, Rectangle<double> s)
{
    auto copy = getObject(a)->clone();
    auto x = dynamic_cast<RectangleDynamicObject*>(copy.get());
    x->rectangle = s;
    return var(x);
}
```

This clones the thisObject (preserving the ScriptRectangle subclass type) and sets its rectangle to the result. This ensures that chained operations like `rect.withX(10).withY(20)` return proper ScriptRectangle instances.

---

## Property Access System

### Direct Properties (x, y, width, height)

The rectangle exposes exactly 4 properties via `hasProperty`/`getProperty`/`setProperty`:

```cpp
static const std::array<Identifier, 4> ids = { "x", "y", "width", "height" };
```

`getProperty` maps to `rectangle.getX()`, `getY()`, `getWidth()`, `getHeight()`.
`setProperty` maps to `rectangle.setX()`, `setY()`, `setWidth()`, `setHeight()`.

**Note:** `getProperty` uses a `static var v` as a return buffer -- this is a pattern to satisfy the `const var&` return type requirement. It means the returned reference is only valid until the next getProperty call.

### Indexed Access (AssignableObject)

ScriptRectangle maps indices 0-3 to `x`, `y`, `width`, `height`:

```cpp
void assign(const int index, var newValue) override
{
    static const std::array<Identifier, 4> ids = { "x", "y", "width", "height" };
    if(isPositiveAndBelow(index, 4))
        assign(ids[index], newValue);
}
```

This enables `rect[0]` for x, `rect[1]` for y, `rect[2]` for width, `rect[3]` for height.

### Dot Property Access (AssignableDotObject)

```cpp
var getDotProperty(const Identifier& id) const override
{
    return getProperty(id);
}
```

This enables `rect.x`, `rect.width` etc. as readable properties.

---

## Argument Parsing Helpers

### getRectangleArgs -- Polymorphic Rectangle Input

```cpp
bool RectangleDynamicObject::FunctionMap::getRectangleArgs(const Args& a, Rectangle<double>& r)
```

Accepts rectangle arguments in three formats:
1. **4 separate doubles:** `(x, y, w, h)` -- constructs Rectangle from 4 args
2. **Single RectangleDynamicObject:** another Rectangle object
3. **Single array of 4 elements:** `[x, y, w, h]` -- the traditional HISE format

This means methods like `contains`, `intersects`, `getUnion`, `constrainedWithin` accept both Rectangle objects and `[x,y,w,h]` arrays as arguments.

### getPointArgs -- Polymorphic Point Input

```cpp
bool RectangleDynamicObject::FunctionMap::getPointArgs(const Args& a, Point<double>& p)
```

Accepts point arguments in two formats:
1. **2 separate doubles:** `(x, y)`
2. **Single array of 2 elements:** `[x, y]`

Used by `contains` to support point containment checks.

### getDoubleArgs -- Safe Double Extraction

```cpp
double getDoubleArgs(const var::NativeFunctionArgs& a, int index, double defaultValue=0.0)
```

Safely extracts a double from the argument list at the given index, returning defaultValue if out of bounds. This enables optional parameters (e.g., `scaled(factorX)` uses factorX for both axes when only 1 arg provided -- though actually `scaled` checks `numArguments` explicitly).

---

## Interoperability with Array-Based Rectangle Convention

Rectangle objects are fully interoperable with HISE's traditional `[x, y, w, h]` array convention:

### Rectangle -> Array
- `ApiHelpers::getRectangleFromVar()` handles both arrays and RectangleDynamicObject:
  ```cpp
  else if(auto ro = dynamic_cast<RectangleDynamicObject*>(data.getDynamicObject()))
      return ro->getRectangle().toFloat();
  ```
- `toArray()` method returns `[x, y, w, h]` as a plain JavaScript array

### Array -> Rectangle
- `ScriptRectangle(const var& rectArray)` constructor converts array to rectangle:
  ```cpp
  ScriptRectangle(const var& rectArray):
    RectangleDynamicObject(ApiHelpers::getRectangleFromVar(rectArray, nullptr).toDouble())
  {}
  ```

### Bidirectional in Graphics/LAF APIs
- `GraphicsObject::getRectangleFromVar()` and `getIntRectangleFromVar()` both check for `ScriptRectangle*` dynamic_cast before falling back to array parsing (ScriptingGraphics.cpp lines 2405, 2423)

---

## JSON Serialization

```cpp
void RectangleDynamicObject::writeAsJSON(OutputStream& mos, int indentLevel, bool cond, int i)
{
    auto s = toString();
    mos.writeText(s, false, false, NewLine::getDefault());
}
```

Rectangle serializes as a plain string representation (JUCE Rectangle::toString format, which is something like `"x y width height"`), not as a JSON object. This is important for console output and debugging.

---

## clone() Behavior

The base class `RectangleDynamicObject::clone()` contains `jassertfalse` and returns `const_cast<RectangleDynamicObject*>(this)` -- it should not be called directly. The ScriptRectangle override properly creates a new instance:

```cpp
Ptr clone() const override
{
    return new ScriptRectangle(this->rectangle);
}
```

The `create` helper in FunctionMap calls `getObject(a)->clone()` which invokes the ScriptRectangle override, ensuring method return values are proper new instances.

---

## withAspectRatioLike -- Custom Implementation Detail

This method has a non-trivial custom implementation:

```cpp
ADD_FUNCTION(withAspectRatioLike, "(var otherRect)", [](const Args& a)
{
    auto r = getRectangle(a);
    Rectangle<double> other;
    if(getRectangleArgs(a, other))
    {
        auto ar = other.getHeight() / other.getWidth();
        auto w = r.getWidth();
        auto h = r.getWidth() * ar;
        float x; float y;
        if(ar > 1.0)  // taller than wide
        {
            w = r.getHeight() / ar;
            h = r.getHeight();
            x = r.getX() + std::abs(w - r.getWidth()) / 2.0;
            y = r.getY();
        }
        else  // wider than tall
        {
            w = r.getWidth();
            h = r.getWidth() * ar;
            x = r.getX();
            y = r.getY() + std::abs(h - r.getHeight()) / 2.0;
        }
        return create(a, {x, y, w, h});
    }
    return create(a, r);
});
```

It fits a rectangle with the aspect ratio of `otherRect` into `this` rectangle, centering it. The aspect ratio is determined by comparing height/width ratio to 1.0 (portrait vs landscape).

---

## Threading / Lifecycle

Rectangle objects have no threading constraints. They are pure value objects -- no references to MainController, no audio thread interactions, no lifecycle requirements. They can be created and used anywhere at any time.

The underlying `FunctionMap` is a `SharedResourcePointer`, which is thread-safe for read-only access (the map is populated once in the constructor and never modified).

---

## No Constants

Rectangle has no `addConstant()` calls and no enum definitions. It is a pure utility class with no configuration constants.

---

## No Preprocessor Guards

The Rectangle class itself has no conditional compilation. The only related preprocessor is `HISE_USE_SCRIPT_RECTANGLE_OBJECT` which controls whether OTHER APIs return Rectangle objects instead of arrays -- it does not affect the Rectangle class itself. The global `Rectangle()` function is always available regardless of this setting.

---

## Summary of All 31 API Methods

| Method | Args | Pattern | Returns | Mutates this |
|--------|------|---------|---------|-------------|
| removeFromTop | 1 | ADD_FUNCTION1 | Rectangle (removed strip) | Yes (shrinks) |
| removeFromLeft | 1 | ADD_FUNCTION1 | Rectangle (removed strip) | Yes (shrinks) |
| removeFromRight | 1 | ADD_FUNCTION1 | Rectangle (removed strip) | Yes (shrinks) |
| removeFromBottom | 1 | ADD_FUNCTION1 | Rectangle (removed strip) | Yes (shrinks) |
| withX | 1 | ADD_FUNCTION1 | Rectangle | No |
| withY | 1 | ADD_FUNCTION1 | Rectangle | No |
| withLeft | 1 | ADD_FUNCTION1 | Rectangle | No |
| withRight | 1 | ADD_FUNCTION1 | Rectangle | No |
| withBottom | 1 | ADD_FUNCTION1 | Rectangle | No |
| withBottomY | 1 | ADD_FUNCTION1 | Rectangle | No |
| withWidth | 1 | ADD_FUNCTION1 | Rectangle | No |
| withHeight | 1 | ADD_FUNCTION1 | Rectangle | No |
| withCentre | 2 | Custom | Rectangle | No |
| withSize | 2 | ADD_FUNCTION2 | Rectangle | No |
| withSizeKeepingCentre | 2 | ADD_FUNCTION2 | Rectangle | No |
| translated | 2 | ADD_FUNCTION2 | Rectangle | No |
| withTrimmedBottom | 1 | ADD_FUNCTION1 | Rectangle | No |
| withTrimmedLeft | 1 | ADD_FUNCTION1 | Rectangle | No |
| withTrimmedRight | 1 | ADD_FUNCTION1 | Rectangle | No |
| withTrimmedTop | 1 | ADD_FUNCTION1 | Rectangle | No |
| scaled | 1-2 | Custom | Rectangle | No |
| setPosition | 2 | ADD_VOID_FUNCTION2 | void | Yes |
| setSize | 2 | ADD_VOID_FUNCTION2 | void | Yes |
| setCentre | 2 | ADD_VOID_FUNCTION2 | void | Yes |
| constrainedWithin | 1 (rect) | Custom | Rectangle | No |
| contains | 1 (rect or point) | Custom | bool | No |
| intersects | 1 (rect) | Custom | bool | No |
| getUnion | 1 (rect) | Custom | Rectangle | No |
| getIntersection | 1 (rect) | Custom | Rectangle | No |
| isEmpty | 0 | Custom | bool | No |
| reduced | 1-2 | Custom | Rectangle | No |
| expanded | 1-2 | Custom | Rectangle | No |
| withAspectRatioLike | 1 (rect) | Custom | Rectangle | No |
| toArray | 0 | Custom | Array | No |

Note: `toString` is also registered in the FunctionMap but is not listed in the base JSON as a public API method. Total registered functions: 32 (31 in JSON + toString).

---

## Note on setSize Duplicate Registration

In the FunctionMap constructor, `setSize` is registered twice:
1. First via `ADD_VOID_FUNCTION2(setSize, ...)` (line 49)
2. Then via a custom `ADD_FUNCTION(setSize, ...)` lambda (line 52)

The second registration overwrites the first in the `std::map`. Both implementations are functionally identical -- they both call `getRectangle(a).setSize(arg0, arg1)` and return void. This appears to be an oversight but has no behavioral impact.
