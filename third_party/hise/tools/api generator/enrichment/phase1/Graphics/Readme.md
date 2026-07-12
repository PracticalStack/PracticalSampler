# Graphics -- Class Analysis

## Brief
2D drawing context for paint callbacks -- shapes, text, images, paths, layers, and post-processing effects.

## Purpose
Graphics is the drawing context object passed as the first argument to ScriptPanel paint callbacks and ScriptedLookAndFeel drawing functions. It provides a deferred rendering API where all drawing operations are recorded as action objects on the scripting thread, then replayed on the UI thread via a command list architecture. The class supports basic shape drawing, text rendering, image compositing, path operations, gradient fills, layer-based post-processing (blur, HSL grading, masks, shaders), and blend mode compositing. Graphics objects are never created by user code -- they are provided by the framework.

## Details

### Deferred Rendering Architecture

Graphics methods do not draw directly onto pixels. Each method call creates a `DrawActions::ActionBase` subclass and adds it to an internal `DrawActions::Handler`. When the paint callback returns, `flush()` swaps the accumulated action list into the renderer's current set. The UI thread then iterates the action list and calls `perform(Graphics& g)` on each action against the real JUCE Graphics context. A SpinLock protects the swap between the next-actions buffer and the current-actions buffer.

### Layer System

`beginLayer(drawOnParent)` creates an `ActionLayer` that renders its contents to a separate offscreen image. Post-processing effects (gaussianBlur, boxBlur, desaturate, applyHSL, applyGamma, applyGradientMap, applyMask, applySharpness, applySepia, applyVignette) operate on this offscreen image via `PostGraphicsRenderer`. `endLayer()` composites the processed layer back. Layers can be nested. All post-processing methods require an active layer -- calling them without one triggers a script error.

`beginBlendLayer(blendMode, alpha)` creates a `BlendingLayer` that composites using one of 25 gin-library blend modes (Normal, Multiply, Screen, Overlay, etc.).

### Colour Format

Colours use ARGB format `0xAARRGGBB` and can be specified as:
- Integer/int64 literals: `0xFFFF0000` (red)
- Hex strings: `"0xFFFF0000"`
- Large integer strings parsed via `getLargeIntValue()`

### Area Parameters

Most drawing methods accept an `area` parameter as a `[x, y, width, height]` array. Passing x, y, w, h as separate arguments (instead of a single array) is a common mistake caught by the backend diagnostic system.

### Gradient Data Format

See `setGradientFill` for the full gradient array format (linear, radial, and multi-stop variants).

### Rounded Rectangle cornerData Format

See `fillRoundedRectangle` and `drawRoundedRectangle` for the full cornerData format (uniform radius or per-corner JSON object).

### Stroke Style Format (drawPath)

See `drawPath` for the full strokeStyle format (number or JSON object with Thickness, EndCapStyle, JointStyle).

### Shadow Data Format (drawAlignedTextShadow)

See `drawAlignedTextShadow` for the full shadowData JSON format (Colour, Offset, Radius, Spread, Inner).

### Noise Parameters (addNoise)

See `addNoise` for the full noise parameter format (simple float or JSON object with alpha, monochromatic, scaleFactor, area).

### Text Alignment Strings

See `drawAlignedText` for the full list of 11 supported alignment strings (uses British spelling `"centred"`).

### Blend Mode Strings

See `beginBlendLayer` for the full list of 25 supported blend mode strings.

### Image Drawing

See `drawImage` for the full image rendering API (preloading, filmstrip frames, context requirements).

### Font Resolution

See `setFont` and `setFontWithSpacing` for font name resolution details (embedded, system, and global HISE fonts).

### drawLine Parameter Order

See `drawLine` for the unusual `(x1, x2, y1, y2, thickness)` parameter order (groups x-coordinates then y-coordinates).

### Backend Diagnostics (USE_BACKEND only)

In the HISE IDE, the backend diagnostic system warns at parse time if:
- A drawing method is called before `setColour`/`setGradientFill`
- A text method is called before `setFont`/`setFontWithSpacing`
- An area parameter receives too many arguments (separate args instead of array)
- A layer operation is called without `beginLayer`

### Float Sanitization

All float parameters are wrapped in `SANITIZED()` which filters NaN/Inf values to prevent corruption of the JUCE Graphics state.

## obtainedVia
Passed as the first argument to `ScriptPanel` paint callbacks and `ScriptLookAndFeel` drawing functions. Not user-created.

## minimalObjectToken
g

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `g.fillRect(10, 10, 100, 50)` | `g.fillRect([10, 10, 100, 50])` | Area must be a single `[x, y, w, h]` array, not four separate arguments. Passing separate arguments is caught by the backend diagnostic system. |
| `g.fillRect([10, 10, 100, 50])` (no setColour) | `g.setColour(0xFFFF0000); g.fillRect([10, 10, 100, 50])` | Drawing methods require `setColour` or `setGradientFill` to be called first, otherwise the colour is undefined. |
| `g.gaussianBlur(5)` (no layer) | `g.beginLayer(false); ...; g.gaussianBlur(5); g.endLayer()` | Post-processing effects (blur, desaturate, HSL, mask, etc.) require an active layer created with `beginLayer`. |
| `g.drawAlignedText("Hi", [0,0,100,30], "center")` | `g.drawAlignedText("Hi", [0,0,100,30], "centred")` | Alignment uses British spelling `"centred"`, not American `"center"`. Invalid strings cause a script error. |

## codeExample
```javascript
// Graphics is not user-created -- it arrives as a paint callback parameter
Panel1.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setColour(0xFFFFFFFF);
    g.setFont("Arial", 16.0);
    g.drawAlignedText("Hello", [10, 10, 200, 30], "left");
});
```

## Alternatives
- `Path` -- defines reusable vector shape geometry that Graphics draws or fills.
- `ScriptLookAndFeel` -- determines when and where drawing methods are called for component rendering.

## Related Preprocessors
`USE_BACKEND` -- backend diagnostic checks for colour/font/layer/area errors.
`HISE_INCLUDE_PROFILING_TOOLKIT` -- per-action profiling data.
`PERFETTO` -- repaint flow tracing in drawRepaintMarker.

## Diagrams

### graphics-deferred-rendering
- **Brief:** Deferred Draw Action Pipeline
- **Type:** topology
- **Description:** The Graphics deferred rendering pipeline has three stages across two threads. On the Scripting Thread: (1) Paint callback calls Graphics methods (setColour, fillRect, etc.) which create ActionBase objects and append them to a nextActions list in the DrawActions::Handler. (2) After the callback returns, flush() swaps nextActions into currentActions and triggers an AsyncUpdater. On the UI Thread: (3) The BorderPanel's paint() method creates an Iterator over currentActions and calls perform(Graphics& g) on each action, replaying them onto the real JUCE Graphics context. A SpinLock protects the nextActions/currentActions swap.

### graphics-layer-stack
- **Brief:** Layer and Post-Processing Stack
- **Type:** topology
- **Description:** The layer system works as a stack. beginLayer() pushes an ActionLayer onto the layerStack. Subsequent draw calls (fillRect, drawPath, etc.) add ActionBase objects to the layer's internalActions list. Post-processing calls (gaussianBlur, applyHSL, applyMask, etc.) add PostActionBase objects to the layer's postActions list. endLayer() pops the layer and composites it: first all internalActions render to an offscreen image, then all postActions process that image via PostGraphicsRenderer, then the result is drawn onto the parent layer or main canvas. beginBlendLayer() works similarly but uses gin::BlendMode compositing for the final draw.

## Diagnostic Ideas
Reviewed: Yes
Count: 2
- Graphics.drawMarkdownText -- timeline dependency (logged)
- Graphics.beginBlendLayer -- value check (logged)
