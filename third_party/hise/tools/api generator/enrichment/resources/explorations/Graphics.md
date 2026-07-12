# Graphics -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite check (ScriptPanel listed as prerequisite)
- `enrichment/resources/survey/class_survey_data.json` -- Graphics entry
- No prerequisite Readme loaded (ScriptPanel enrichment not yet completed; Graphics can stand alone as a drawing context API)
- No base class explorations needed (Graphics is not a component class)

## Source Files

- **Header:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.h` (lines 596-791)
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.cpp` (lines 1605-2495)
- **Draw action types:** `HISE/hi_scripting/scripting/api/ScriptDrawActions.cpp`
- **DrawActions infrastructure:** `HISE/hi_core/hi_core/MiscComponents.h` (lines 306-582)
- **PostGraphicsRenderer:** `HISE/hi_tools/hi_tools/PostGraphicsRenderer.h`

## Class Declaration

```cpp
class GraphicsObject : public ConstScriptingObject
```

- C++ class name: `ScriptingObjects::GraphicsObject`
- Scripting API name: `Graphics` (via `RETURN_STATIC_IDENTIFIER("Graphics")`)
- Inherits from `ConstScriptingObject` (0 constants in constructor call)
- No `ApiClass` base -- this is a pure object, not a namespace
- Constructor takes `ProcessorWithScriptingContent* p` and `ConstScriptingObject* parent`
- The `parent` is critical: it determines the context (ScriptPanel, ScriptedLookAndFeel, or DynamicContainer child)

### Private Members

```cpp
Point<float> getPointFromVar(const var& data);
Rectangle<float> getRectangleFromVar(const var &data);
Rectangle<int> getIntRectangleFromVar(const var &data);

Font currentFont;
String currentFontName = "";
float currentKerningFactor = 0.0f;
float currentFontHeight = 13.0f;

Result rectangleResult;

ConstScriptingObject* parent = nullptr;

DrawActions::Handler drawActionHandler;
```

Key observations:
- `currentFont`, `currentFontName`, `currentFontHeight`, `currentKerningFactor` -- font state is cached locally in the GraphicsObject for `getStringWidth()` to work without a Graphics context
- `drawActionHandler` -- The central infrastructure. All drawing operations create action objects and add them to this handler. Drawing is deferred, not immediate.
- `parent` -- raw pointer to the parent object (ScriptPanel, ScriptedLookAndFeel, etc.)

## Constructor -- API Method Registration

Constructor at line 1721. Registers with `ConstScriptingObject(p, 0)` -- **zero constants**.

### Diagnostics Infrastructure (USE_BACKEND only)

The constructor sets up diagnostic checks for common mistakes:

```cpp
registerDiagnosticPrototype(*this, parent);

#if USE_BACKEND
#define CHECK_AREA_AND_COLOUR(x) addDiagnostic(#x, DiagnosticResult::combine(GraphicsDiagnostics::checkColourSet, GraphicsDiagnostics::checkAreaAsArray));
#define CHECK_FONT_AND_COLOUR(x) addDiagnostic(#x, DiagnosticResult::combine(GraphicsDiagnostics::checkColourSet, GraphicsDiagnostics::checkFontSet));
```

Three diagnostic checks defined in `GraphicsDiagnostics` struct (lines 1661-1716):

1. **checkColourSet** -- Warns if `setColour` or `setGradientFill` hasn't been called before a drawing method
2. **checkInLayer** -- Fails if `beginLayer` hasn't been called before layer operations
3. **checkFontSet** -- Warns if `setFont` or `setFontWithSpacing` hasn't been called before text methods
4. **checkAreaAsArray** -- Fails if too many arguments passed (catches the common mistake of passing x,y,w,h as separate arguments instead of as an array)

### Diagnostic Assignments

| Method | Diagnostics Applied |
|--------|-------------------|
| drawRect | CHECK_AREA_AND_COLOUR |
| fillRect | CHECK_AREA_AND_COLOUR |
| drawRoundedRectangle | CHECK_AREA_AND_COLOUR |
| fillRoundedRectangle | CHECK_AREA_AND_COLOUR |
| drawAlignedText | CHECK_FONT_AND_COLOUR |
| drawAlignedTextShadow | CHECK_FONT_AND_COLOUR |
| drawMultiLineText | CHECK_FONT_AND_COLOUR |
| drawEllipse | CHECK_AREA_AND_COLOUR |
| fillEllipse | CHECK_AREA_AND_COLOUR |
| drawTriangle | CHECK_AREA_AND_COLOUR |

### Method Registration

All methods use `ADD_API_METHOD_N` (untyped). No `ADD_TYPED_API_METHOD_N` registrations.

```
ADD_API_METHOD_1(fillAll)
ADD_API_METHOD_1(setColour)
ADD_API_METHOD_1(setOpacity)
ADD_API_METHOD_2(drawRect)
ADD_API_METHOD_1(fillRect)
ADD_API_METHOD_3(drawRoundedRectangle)
ADD_API_METHOD_2(fillRoundedRectangle)
ADD_API_METHOD_5(drawLine)
ADD_API_METHOD_3(drawHorizontalLine)
ADD_API_METHOD_3(drawVerticalLine)
ADD_API_METHOD_2(setFont)
ADD_API_METHOD_3(setFontWithSpacing)
ADD_API_METHOD_2_DEPRECATED(drawText, "use drawAlignedText for better placement")
ADD_API_METHOD_3(drawAlignedText)
ADD_API_METHOD_4(drawAlignedTextShadow)
ADD_API_METHOD_5(drawMultiLineText)
ADD_API_METHOD_1(drawMarkdownText)
ADD_API_METHOD_3(drawSVG)
ADD_API_METHOD_1(setGradientFill)
ADD_API_METHOD_2(drawEllipse)
ADD_API_METHOD_1(fillEllipse)
ADD_API_METHOD_4(drawImage)
ADD_API_METHOD_3(drawDropShadow)
ADD_API_METHOD_5(drawDropShadowFromPath)
ADD_API_METHOD_5(drawInnerShadowFromPath)
ADD_API_METHOD_2(addDropShadowFromAlpha)
ADD_API_METHOD_3(drawTriangle)
ADD_API_METHOD_2(fillTriangle)
ADD_API_METHOD_2(fillPath)
ADD_API_METHOD_3(drawPath)
ADD_API_METHOD_2(rotate)
ADD_API_METHOD_2(drawFFTSpectrum)
ADD_API_METHOD_1(beginLayer)
ADD_API_METHOD_1(gaussianBlur)
ADD_API_METHOD_1(boxBlur)
ADD_API_METHOD_0(desaturate)
ADD_API_METHOD_1(addNoise)
ADD_API_METHOD_3(applyMask)
ADD_API_METHOD_2(flip)
ADD_API_METHOD_3(applyHSL)
ADD_API_METHOD_1(applyGamma)
ADD_API_METHOD_2(applyGradientMap)
ADD_API_METHOD_1(applySharpness)
ADD_API_METHOD_0(applySepia)
ADD_API_METHOD_3(applyVignette)
ADD_API_METHOD_2(applyShader)
ADD_API_METHOD_0(endLayer)
ADD_API_METHOD_2(beginBlendLayer)
ADD_API_METHOD_1(getStringWidth)
ADD_API_METHOD_1(drawRepaintMarker)
```

**Notable:** `drawText` is registered with `ADD_API_METHOD_2_DEPRECATED` with message "use drawAlignedText for better placement".

**Missing registration:** `drawFittedText` has a wrapper (`API_VOID_METHOD_WRAPPER_5`) and full implementation but does NOT appear in the constructor's `ADD_API_METHOD` calls. It is present in the base JSON, meaning the Doxygen-based generator picks it up from the header declaration. Whether it is actually accessible at runtime is uncertain -- it may be silently unavailable if the ADD_API_METHOD is required for the scripting engine to find the method.

### Error Logger Setup

```cpp
WeakReference<Processor> safeP(dynamic_cast<Processor*>(p));
drawActionHandler.errorLogger = [safeP](const String& m)
{
    if (safeP.get() != nullptr)
        debugError(safeP.get(), m);
};
```

The draw handler routes errors back to the script processor's debug console.

## Deferred Draw Action Architecture

This is the most important architectural pattern for the Graphics class.

### Pattern

None of the Graphics methods draw directly. Instead, each method:
1. Creates a `DrawActions::ActionBase` (or `PostActionBase`) subclass instance
2. Passes it to `drawActionHandler.addDrawAction()` (or to the current layer's `addPostAction()`)
3. The actions are accumulated in a list
4. When `flush()` is called, the accumulated actions become the "current" set
5. The rendering component (BorderPanel) iterates through actions and calls `perform(Graphics& g)` on each

This means the scripting thread builds a command list, and the UI thread replays it. The two never share a JUCE Graphics context directly.

### DrawActions::Handler (hi_core/MiscComponents.h)

```cpp
struct Handler: private AsyncUpdater
{
    void beginDrawing();
    bool beginBlendLayer(const Identifier& blendMode, float alpha);
    void beginLayer(bool drawOnParent);
    ActionLayer::Ptr getCurrentLayer();
    void endLayer();
    void addDrawAction(ActionBase* newDrawAction);
    void flush(uint64_t perfettoTrackId, uint32 profileTrackId);
    // ...
private:
    ReferenceCountedArray<ActionLayer> layerStack;
    ReferenceCountedArray<ActionBase> nextActions;
    ReferenceCountedArray<ActionBase> currentActions;
};
```

Key fields:
- `nextActions` -- accumulates actions during the paint callback
- `currentActions` -- the last flushed set, used by the renderer
- `layerStack` -- stack of active layers for nested layer operations
- `scaleFactor` -- scale factor for retina/HiDPI rendering
- `globalBounds` -- used for shader coordinate mapping

### ActionBase

```cpp
class ActionBase: public ReferenceCountedObject
{
    virtual void perform(Graphics& g) = 0;
    virtual bool wantsCachedImage() const;
    virtual bool wantsToDrawOnParent() const;
    virtual void setCachedImage(Image& actionImage_, Image& mainImage_);
    virtual void setScaleFactor(float sf);
};
```

### ActionLayer

```cpp
class ActionLayer : public ActionBase
{
    void addDrawAction(ActionBase* a);
    void addPostAction(PostActionBase* a);
    OwnedArray<ActionBase> internalActions;
    OwnedArray<PostActionBase> postActions;
    PostGraphicsRenderer::DataStack stack;
};
```

A layer renders its internal actions to a separate image, then applies post-processing effects (blur, desaturate, etc.) using `PostGraphicsRenderer`, then composites the result.

### BlendingLayer

```cpp
class BlendingLayer : public ActionLayer
{
    gin::BlendMode blendMode;
    float alpha;
    Image blendSource;
};
```

Extends ActionLayer with blend mode compositing using gin library blend functions.

### PostActionBase

```cpp
class PostActionBase : public ReferenceCountedObject
{
    virtual void perform(PostGraphicsRenderer& r) = 0;
    virtual bool needsStackData() const;
};
```

Post-actions are pixel-level operations applied after all draw actions in a layer are completed.

## ObtainedVia / Factory Pattern

Graphics objects are **never created by user script code**. They are created internally by HISE and passed as the first argument to paint callbacks. There are four creation contexts:

### 1. ScriptPanel Paint Routine (primary use case)

In `ScriptingApiContent.cpp`, `ScriptPanel` owns a persistent `graphics` member:
```cpp
ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> graphics;
// Created in ScriptPanel constructor:
graphics(new ScriptingObjects::GraphicsObject(base, this))
```

The paint callback passes it as `var`:
```cpp
var arguments = var(graphics.get());
engine->callExternalFunction(paintRoutine, args, &r);
graphics->getDrawHandler().flush(lastId, idx);
```

### 2. ScriptedLookAndFeel Functions

In `ScriptingGraphics.cpp`, the LAF creates GraphicsObjects per-component-per-function:
```cpp
gr.g = new GraphicsObject(getScriptProcessor(), this);
// ...
args[0] = var(g.get());
args[1] = argsObject;
```

The parent here is the `ScriptedLookAndFeel` itself, not the component being drawn.

### 3. DynamicContainer ChildReference

Each child in a ScriptDynamicContainer gets its own GraphicsObject:
```cpp
n->second = new ScriptingObjects::GraphicsObject(obj->getScriptProcessor(), obj);
```

### 4. ComponentDragInfo

For drag-and-drop rendering:
```cpp
graphicsObject = var(new ScriptingObjects::GraphicsObject(sc->getScriptProcessor(), sc));
```

## Colour Handling

All colour parameters go through `ScriptingApi::Content::Helpers::getCleanedObjectColour()` which delegates to `ApiHelpers::getColourFromVar()`:

```cpp
Colour ApiHelpers::getColourFromVar(const var& value)
{
    int64 colourValue = 0;
    if (value.isInt64() || value.isInt())
        colourValue = (int64)value;
    else if (value.isString())
    {
        auto string = value.toString();
        if (string.startsWith("0x"))
            colourValue = string.getHexValue64();
        else
            colourValue = string.getLargeIntValue();
    }
    return Colour((uint32)colourValue);
}
```

Colours can be:
- Integer/int64: e.g., `0xFF000000`
- String starting with "0x": hex parsing via `getHexValue64()`
- Other string: parsed as large integer via `getLargeIntValue()`

Standard ARGB format: `0xAARRGGBB`

## Justification/Alignment Strings

Used by `drawAlignedText`, `drawAlignedTextShadow`, `drawFittedText`, `drawMultiLineText`.

Valid values from `ApiHelpers::getJustificationNames()`:

| String | JUCE Justification |
|--------|--------------------|
| `"left"` | Justification::left |
| `"right"` | Justification::right |
| `"top"` | Justification::top |
| `"bottom"` | Justification::bottom |
| `"centred"` | Justification::centred |
| `"centredTop"` | Justification::centredTop |
| `"centredBottom"` | Justification::centredBottom |
| `"topLeft"` | Justification::topLeft |
| `"topRight"` | Justification::topRight |
| `"bottomLeft"` | Justification::bottomLeft |
| `"bottomRight"` | Justification::bottomRight |

Invalid alignment strings cause `reportScriptError()`.

## Blend Mode Strings

Used by `beginBlendLayer`. Valid values from `DrawActions::Handler::beginBlendLayer()`:

```
"Normal", "Lighten", "Darken", "Multiply", "Average",
"Add", "Subtract", "Difference", "Negation", "Screen",
"Exclusion", "Overlay", "SoftLight", "HardLight", "ColorDodge",
"ColorBurn", "LinearDodge", "LinearBurn", "LinearLight",
"VividLight", "PinLight", "HardMix", "Reflect", "Glow", "Phoenix"
```

These are `gin::BlendMode` enum values (25 modes). Invalid blend mode strings cause `beginBlendLayer` to return false (no error reported, just silently fails).

## Gradient Fill Format

`setGradientFill` accepts an array with multiple formats:

### Linear gradient (6 elements)
```
[Colour1, x1, y1, Colour2, x2, y2]
```

### Linear/radial gradient with isRadial flag (7 elements)
```
[Colour1, x1, y1, Colour2, x2, y2, isRadial]
```

### Multi-stop gradient (7+ elements, pairs of colour+position after element 6)
```
[Colour1, x1, y1, Colour2, x2, y2, isRadial, StopColour1, position1, StopColour2, position2, ...]
```

Where additional stops after index 7 come in pairs: `[colour, position]` where position is 0.0-1.0.

If the array has fewer than 6 elements, `reportScriptError("Gradient Data is not sufficient")` is called.

## Rounded Rectangle cornerData Format

Used by `fillRoundedRectangle` and `drawRoundedRectangle`. The `cornerData` parameter can be:

### Simple float
A single number for uniform corner radius:
```javascript
g.fillRoundedRectangle([10, 10, 100, 50], 5.0);
```

### JSON object for per-corner control
```javascript
g.fillRoundedRectangle([10, 10, 100, 50], {
    "CornerSize": 10.0,
    "Rounded": [true, true, false, false]  // [topLeft, topRight, bottomLeft, bottomRight]
});
```

When `Rounded` array has all false values, falls back to `fillRect`/`drawRect`. When some corners are rounded, uses `Path::addRoundedRectangle` with per-corner booleans.

## Stroke Style Format

Used by `drawPath`. The `strokeStyle` parameter can be:

### Simple number
Just the stroke thickness:
```javascript
g.drawPath(p, area, 2.0);
```

### JSON object
```javascript
g.drawPath(p, area, {
    "Thickness": 2.0,
    "EndCapStyle": "rounded",   // "butt", "square", "rounded"
    "JointStyle": "curved"      // "mitered", "curved", "beveled"
});
```

Parsed by `ApiHelpers::createPathStrokeType()`.

## Shadow Data Format (drawAlignedTextShadow)

The `shadowData` parameter is a JSON object parsed by `ApiHelpers::getShadowParameters()`:

```javascript
{
    "Colour": 0xFF000000,    // Shadow colour (defaults to black)
    "Offset": [2, 2],        // [x, y] offset (defaults to [0, 0])
    "Radius": 5,             // Blur radius (defaults to 0)
    "Spread": 0,             // Shadow spread (defaults to 0)
    "Inner": false           // true for inner shadow, false for drop shadow (defaults to false)
}
```

Uses melatonin blur library for rendering (both `melatonin::DropShadow` and `melatonin::InnerShadow`).

## addNoise Parameter Format

The `noiseAmount` parameter can be:

### Simple float (0.0 to 1.0)
Opacity of noise overlay. Area is derived from parent component dimensions.

### JSON object for advanced noise control
```javascript
{
    "alpha": 0.5,           // Noise opacity (0.0-1.0)
    "monochromatic": true,  // true for grayscale noise, false for colour
    "scaleFactor": 1.0,     // Scale factor (-1.0 uses the handler's scale factor)
    "area": [0, 0, 200, 200] // Custom area (optional, defaults to component bounds)
}
```

Noise maps are managed by `DrawActions::NoiseMapManager` (a `SharedResourcePointer`), which caches noise images to avoid regenerating them every frame.

## Layer Operations -- Require Active Layer

The following methods require a layer created via `beginLayer()` and will call `reportScriptError()` if no layer exists:

- `gaussianBlur` -- "You need to create a layer for gaussian blur"
- `boxBlur` -- "You need to create a layer for box blur"
- `desaturate` -- "You need to create a layer for desaturating"
- `applyMask` -- "You need to create a layer for applying a mask"
- `applyHSL` -- "You need to create a layer for applying HSL"
- `applyGamma` -- "You need to create a layer for applying gamma"
- `applyGradientMap` -- "You need to create a layer for applyGradientMap"
- `applySharpness` -- "You need to create a layer for applySharpness"
- `applySepia` -- "You need to create a layer for applySepia"
- `applyVignette` -- "You need to create a layer for applySepia" (copy-paste error in message)

These operations use `PostGraphicsRenderer` which operates on pixel data of the layer's rendered image.

### PostGraphicsRenderer

Located in `hi_tools/PostGraphicsRenderer.h`. Provides pixel-level image processing:
- `gaussianBlur(int blur)` -- Gaussian blur with kernel
- `boxBlur(int blur)` -- Box blur
- `desaturate()` -- Remove all saturation
- `applyMask(Path, invert, scale)` -- Path-based alpha masking
- `applyHSL(h, s, l)` -- HSL colour grading
- `applyGamma(g)` -- Gamma curve correction
- `applyGradientMap(ColourGradient)` -- Map brightness to gradient
- `applySharpness(delta)` -- Sharpen or soften
- `applySepia()` -- Sepia tone effect
- `applyVignette(amount, radius, falloff)` -- Corner darkening

Uses an internal `DataStack` of pre-allocated buffers to avoid allocations during rendering.

## Methods NOT Requiring Layers

These methods add draw actions directly and can be used without a layer:
- All shape drawing (fillRect, drawRect, fillRoundedRectangle, etc.)
- All text drawing (drawText, drawAlignedText, etc.)
- All state setting (setColour, setFont, setGradientFill, setOpacity)
- fillAll, drawImage, drawPath, fillPath, rotate, flip
- drawDropShadow, drawDropShadowFromPath, drawInnerShadowFromPath
- addDropShadowFromAlpha (uses cached image but NOT a layer)
- addNoise (adds directly as a draw action, not a post action)
- drawRepaintMarker

## drawImage Behavior

Only works when the parent is a ScriptPanel or ScriptedLookAndFeel:

```cpp
if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(parent))
    img = sc->getLoadedImage(imageName);
else if (auto laf = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel*>(parent))
    img = laf->getLoadedImage(imageName);
else
    reportScriptError("drawImage is only allowed in a panel's paint routine");
```

Images must be loaded via `ScriptPanel.loadImage()` or `ScriptLookAndFeel.loadImage()` first. The `imageName` is a string key.

If the image is not found, a grey placeholder rectangle with "XXX" text is drawn and a debug error is logged.

The `yOffset` parameter is used for filmstrip-style images: it selects which vertical frame to display. The `xOffset` parameter is declared but explicitly ignored (`int /*xOffset*/`).

## drawLine Parameter Quirk

The signature is `drawLine(float x1, float x2, float y1, float y2, float lineThickness)` -- note that x2 comes before y1. This is atypical (most APIs use x1,y1,x2,y2). The implementation passes them to the JUCE drawLine call reordered:

```cpp
drawActionHandler.addDrawAction(new ScriptedDrawActions::drawLine(
    SANITIZED(x1), SANITIZED(y1), SANITIZED(x2), SANITIZED(y2), SANITIZED(lineThickness)));
```

The action then calls `g.drawLine(x1, x2, y1, y2, lineThickness)` with the first-arg-order (x1, y1=x2_param, x2=y1_param, y2, thickness). This means the actual JUCE call receives (x1, y1, x2, y2, thickness) because the drawLine action stores its params as x1,x2,y1,y2 matching the ScriptedDrawActions::drawLine constructor: `drawLine(float x1_, float x2_, float y1_, float y2_, float lineThickness_)`. But `perform()` calls `g.drawLine(x1, x2, y1, y2, lineThickness)` -- which maps to JUCE's `drawLine(float startX, float startY, float endX, float endY, float lineThickness)`.

So in the scripting API: `g.drawLine(x1, x2, y1, y2, thickness)` maps to JUCE's `drawLine(startX=x1, startY=x2, endX=y1, endY=y2, thickness)`. This means the parameter NAMES in the scripting API are misleading -- x2 is actually startY, and y1 is actually endX.

Wait, let me re-trace this more carefully:

1. Script calls: `g.drawLine(x1, x2, y1, y2, lineThickness)`
2. GraphicsObject::drawLine receives: `float x1, float x2, float y1, float y2, float lineThickness`
3. Creates: `new ScriptedDrawActions::drawLine(SANITIZED(x1), SANITIZED(y1), SANITIZED(x2), SANITIZED(y2), SANITIZED(lineThickness))` -- note the reorder to x1,y1,x2,y2
4. ScriptedDrawActions::drawLine constructor stores: `x1(x1_), x2(x2_), y1(y1_), y2(y2_)` -- so x1=x1, x2=y1_from_script, y1=x2_from_script, y2=y2
5. perform() calls: `g.drawLine(x1, x2, y1, y2, lineThickness)`

Wait, this is confusing. Let me trace the actual values:

- Script arg 1 (called "x1") -> GraphicsObject x1 -> passes as arg1 to action (x1_) -> stored as x1 -> JUCE startX. OK.
- Script arg 2 (called "x2") -> GraphicsObject x2 -> **not passed here yet**
- But the create call is: `drawLine(SANITIZED(x1), SANITIZED(y1), SANITIZED(x2), SANITIZED(y2), ...)`
  
Hmm, actually the GraphicsObject::drawLine takes `(float x1, float x2, float y1, float y2, ...)` and creates the action with `(x1, y1, x2, y2, ...)`. So:
- Arg1 (x1 from script) -> action arg1 (x1_) -> stored as x1 -> JUCE g.drawLine arg1 (startX). Correct.
- Arg2 (x2 from script) -> NOT passed as action arg2. Instead, `y1` is passed as action arg2.
  
Wait, the script passes args in order: x1, x2, y1, y2. The GraphicsObject receives them as x1, x2, y1, y2. Then it passes to the action constructor as: `(x1, y1, x2, y2)`. So:
- action x1_ = script's x1
- action x2_ = script's y1  (!)
- action y1_ = script's x2  (!)  
- action y2_ = script's y2

Then action::perform calls `g.drawLine(x1, x2, y1, y2, lineThickness)` = `g.drawLine(script_x1, script_y1, script_x2, script_y2, thickness)`.

JUCE's drawLine is: `drawLine(float startX, float startY, float endX, float endY, float lineThickness)`.

So the FINAL mapping is: script's (x1, x2, y1, y2) -> JUCE's (startX=x1, startY=y1, endX=x2, endY=y2). The internal reordering means that despite the confusing parameter names `x1, x2, y1, y2` in the API, the actual drawing is `(x1, y1) to (x2, y2)` which is the correct line from point1 to point2. The parameter ordering in the API signature is just unusual (grouping x's then y's instead of point1 then point2).

## Font Resolution

`setFont` and `setFontWithSpacing` use `MainController::getFontFromString()`:

```cpp
auto f = mc->getFontFromString(fontName, SANITIZED(fontSize));
```

This resolves embedded fonts, system fonts, and the special name for the global HISE font. The font is stored locally for `getStringWidth()`:

```cpp
float ScriptingObjects::GraphicsObject::getStringWidth(String text)
{
    auto mc = getScriptProcessor()->getMainController_();
    return mc->getStringWidthFromEmbeddedFont(text, currentFontName, currentFontHeight, currentKerningFactor);
}
```

## Threading Context

Graphics objects are created and used on the **scripting thread** (JavascriptThreadPool's LowPriorityCallbackExecution). The draw action list is then flushed and consumed by the **UI thread** through the AsyncUpdater mechanism.

Key sequence:
1. [Scripting Thread] Paint callback executes, calling Graphics methods that build action list
2. [Scripting Thread] `flush()` swaps nextActions into currentActions and triggers AsyncUpdater
3. [UI Thread] `handleAsyncUpdate()` notifies listeners (BorderPanel)
4. [UI Thread] BorderPanel's paint() creates an Iterator and replays all actions on the real JUCE Graphics

The SpinLock on the Handler protects the swap between nextActions and currentActions.

## Profiling Support (HISE_INCLUDE_PROFILING_TOOLKIT)

Each draw action has a `profileData` member that can track per-action timing. The profiling system uses Perfetto traces and dispatch counters. When enabled:
- Each action reports its time in the "g.methodName()" format
- Profile data source type is `SourceType::Paint`
- Preferred time domain is `FPS60` (16.67ms budget)

## Preprocessor Guards

- `USE_BACKEND` -- The `GraphicsDiagnostics` struct and all diagnostic checks are backend-only
- `HISE_INCLUDE_PROFILING_TOOLKIT` -- Per-action profiling data
- `PERFETTO` -- Flow tracing for repaint tracking (in drawRepaintMarker)
- No guards on any API method itself -- all methods available in all build targets

## Related Helper Classes

### SVGObject (ScriptingGraphics.h line 392)

```cpp
class SVGObject: public ConstScriptingObject
```

Internal class for SVG rendering. Created from base64-encoded SVG data. Has a `draw(Graphics& g, Rectangle<float> r, float opacity)` method called by `drawSVG`.

### PathObject (ScriptingGraphics.h line 416)

Wraps a JUCE `Path`. Used by `fillPath`, `drawPath`, `applyMask`, `drawDropShadowFromPath`, `drawInnerShadowFromPath`.

### MarkdownObject (ScriptingGraphics.h line 555)

Wraps a `MarkdownRenderer`. Used by `drawMarkdownText`. Requires `setTextBounds()` to be called first.

### ScriptShader (ScriptingGraphics.h line 234)

OpenGL shader support. Used by `applyShader`. The shader writes to a cached image buffer that is then composited.

## melatonin Blur Library Integration

Several methods use the melatonin blur library (a modern JUCE shadow/blur library):

- `drawDropShadow` -- Uses `melatonin::DropShadow` (replaced the old JUCE DropShadow)
- `drawDropShadowFromPath` -- Template `drawDropShadowFromPath<melatonin::DropShadow>`
- `drawInnerShadowFromPath` -- Template `drawDropShadowFromPath<melatonin::InnerShadow>`
- `drawAlignedTextShadow` -- Uses both `melatonin::DropShadow` and `melatonin::InnerShadow`

## SANITIZED Macro

Many float parameters are wrapped in `SANITIZED()` which calls `FloatSanitizers::sanitizeFloatNumber()` to guard against NaN/Inf values that would corrupt the JUCE Graphics state.

## drawMultiLineText xy Parameter

Unlike other area-based methods that take `[x, y, w, h]`, `drawMultiLineText` takes `xy` as `[x, y]` (just a 2-element point array for the starting position):

```cpp
int startX = (int)xy[0];
int baseLineY = (int)xy[1];
```

## fillPath / drawPath Area Behavior

The `area` parameter for path methods is optional:
```cpp
if (area.isArray() || dynamic_cast<ScriptingObjects::ScriptRectangle*>(area.getDynamicObject()) != nullptr)
{
    Rectangle<float> r = getRectangleFromVar(area);
    p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
}
```

If `area` is not an array or Rectangle, the path is drawn at its original coordinates. This is checked for both `fillPath` and `drawPath`.

## addDropShadowFromAlpha

This is the only method that uses the `wantsCachedImage()` mechanism:
```cpp
bool wantsCachedImage() const override { return true; };
```

This means it receives the parent's main image data via `setCachedImage()` and uses the alpha channel of the existing rendering to generate a shadow. It applies inverse scaling for retina rendering.

## drawRepaintMarker Debug Utility

Fills the entire area with a random HSL colour (30% opacity) and optionally creates Perfetto trace counters:
```cpp
g.fillAll(Colour::fromHSL(Random::getSystemRandom().nextFloat(), 0.33f, 0.5, 0.3f));
```

Used for debugging excessive repaints -- each call shows a different colour overlay.

## Method Count Summary

Total methods in base JSON: 53
All use `ADD_API_METHOD_N` (untyped).
One method is deprecated: `drawText`.
One method may be unregistered: `drawFittedText`.
