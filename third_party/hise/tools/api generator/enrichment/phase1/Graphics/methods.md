# Graphics -- Method Entries

## addDropShadowFromAlpha

**Signature:** `undefined addDropShadowFromAlpha(Colour colour, Number radius)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a JUCE DropShadow object and a new heap-allocated draw action.
**Minimal Example:** `{obj}.addDropShadowFromAlpha(0x88000000, 10);`

**Description:**
Adds a drop shadow behind the current rendering based on the alpha values of the previously drawn content. Unlike `drawDropShadow` which draws a shadow around a rectangular area, this method reads the actual alpha channel of the current image buffer (via the cached image mechanism) and generates a shadow that follows the shape of whatever has already been drawn. The shadow colour and blur radius are configurable. This method does NOT require an active layer -- it adds a regular draw action, not a post-processing effect.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | Shadow colour in 0xAARRGGBB format | Must include alpha channel |
| radius | Number | no | Blur radius for the shadow in pixels | 0 disables the shadow |

**Pitfalls:**
- This method uses the `wantsCachedImage()` mechanism, meaning it receives a snapshot of the entire parent component's rendered image. It must be placed in the draw sequence AFTER the shapes whose alpha channel should generate the shadow. Drawing it first produces no shadow because the cached image is empty.

**Cross References:**
- `$API.Graphics.drawDropShadow$`
- `$API.Graphics.drawDropShadowFromPath$`
- `$API.Graphics.drawInnerShadowFromPath$`
- `$API.Graphics.drawAlignedTextShadow$`

## addNoise

**Signature:** `undefined addNoise(NotUndefined noiseAmount)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action and accesses a shared NoiseMapManager.
**Minimal Example:** `{obj}.addNoise(0.1);`

**Description:**
Adds a noise texture overlay to the current rendering. This is a regular draw action (not a post-processing effect), so it does NOT require an active layer. The noise map is cached by a shared `NoiseMapManager` to avoid regenerating the noise image every paint call.

The method accepts two input formats:

1. **Simple float** (0.0-1.0): Uses the float as the noise opacity. The area is derived from the parent ScriptComponent's width and height.

2. **JSON object** for advanced control:
   - `alpha` (float, 0.0-1.0): Noise opacity
   - `monochromatic` (bool): `true` for grayscale noise, `false` for colour noise
   - `scaleFactor` (float, 0.125-2.0): Scale of the noise texture. Pass `-1.0` to use the draw handler's current scale factor (retina/HiDPI).
   - `area` (array, optional): Custom `[x, y, width, height]` area. Defaults to the parent component's bounds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noiseAmount | NotUndefined | no | Float (0.0-1.0) for simple noise, or JSON object for advanced control | Alpha is clamped to 0.0-1.0; scaleFactor clamped to 0.125-2.0 |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| alpha | Double | Noise opacity (0.0-1.0) |
| monochromatic | Integer | `true` for grayscale noise, `false` for colour noise |
| scaleFactor | Double | Scale factor for noise texture (0.125-2.0, or -1.0 for auto) |
| area | Array | Custom `[x, y, w, h]` area (optional, defaults to component bounds) |

**Pitfalls:**
- When using the simple float form outside a ScriptPanel context (e.g., in a ScriptedLookAndFeel callback), the parent may not be a ScriptComponent, so the area defaults to an empty rectangle and the method reports "No valid area for noise map specified". Use the JSON object form with an explicit `area` property in LAF callbacks.
- The `scaleFactor` value of `-1.0` is a special sentinel that reads the draw handler's current scale factor. Other negative values are clamped to 0.125.

**Cross References:**
- `$API.Graphics.applyGamma$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.gaussianBlur$`

**Example:**
```javascript:addnoise-advanced
// Title: Adding monochromatic noise with custom scale
Panel1.setPaintRoutine(function(g)
{
    g.fillAll(0xFF333333);
    g.addNoise({"alpha": 0.15, "monochromatic": true, "scaleFactor": -1.0});
});
```
```json:testMetadata:addnoise-advanced
{
  "testable": false,
  "skipReason": "Visual output only -- noise overlay cannot be verified programmatically"
}
```

## applyGamma

**Signature:** `undefined applyGamma(Double gamma)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.applyGamma(1.5);`

**Description:**
Applies a gamma correction curve to the current layer's pixel data. This is a post-processing effect that operates on the offscreen image of an active layer via `PostGraphicsRenderer`. Values less than 1.0 brighten the image (lift shadows), values greater than 1.0 darken the image (crush shadows), and 1.0 produces no change. Requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for applying gamma".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| gamma | Double | no | Gamma correction exponent | 1.0 = no change; < 1.0 brightens; > 1.0 darkens |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.desaturate$`
- `$API.Graphics.applyGradientMap$`

## applyGradientMap

**Signature:** `undefined applyGradientMap(Colour darkColour, Colour brightColour)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object and converts colour vars.
**Minimal Example:** `{obj}.applyGradientMap(0xFF001133, 0xFFFFCC00);`

**Description:**
Maps the brightness values of the current layer's pixels to a two-colour gradient. Dark pixels are remapped toward `darkColour` and bright pixels toward `brightColour`, producing a duotone colour grading effect. The mapping uses a `ColourGradient` constructed from the two colours. This is a post-processing effect that requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for applyGradientMap".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| darkColour | Colour | no | Colour for dark/shadow regions | 0xAARRGGBB format |
| brightColour | Colour | no | Colour for bright/highlight regions | 0xAARRGGBB format |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.desaturate$`

## applyHSL

**Signature:** `undefined applyHSL(Double hue, Double saturation, Double lightness)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.applyHSL(0.1, -0.3, 0.05);`

**Description:**
Applies HSL (Hue, Saturation, Lightness) colour grading adjustments to the current layer's pixel data. Each parameter is an additive offset applied to the corresponding HSL channel of every pixel. A hue offset of 0.5 shifts the hue by 180 degrees (opposite side of the colour wheel). Saturation and lightness offsets add to or subtract from the existing values. This is a post-processing effect that requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for applying HSL".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| hue | Double | no | Hue offset (0.0 = no shift, 0.5 = 180-degree shift, 1.0 = full rotation) | Wraps cyclically |
| saturation | Double | no | Saturation offset (positive increases, negative decreases) | -- |
| lightness | Double | no | Lightness offset (positive brightens, negative darkens) | -- |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.
- The parameters are additive offsets, not absolute values. Passing `(0.0, 0.0, 0.0)` produces no change.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.desaturate$`
- `$API.Graphics.applyGamma$`
- `$API.Graphics.applyGradientMap$`

## applyMask

**Signature:** `undefined applyMask(ScriptObject path, Array area, Integer invert)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object, copies the Path, and scales it to the area.
**Minimal Example:** `{obj}.applyMask(myPath, [0, 0, 200, 200], false);`

**Description:**
Applies a path-based alpha mask to the current layer. Pixels inside the path are kept; pixels outside are made transparent (or vice versa when `invert` is true). The path is scaled to fit the specified area using `scaleToFit` (non-uniform scaling). The mask is applied via `PostGraphicsRenderer::applyMask()` which renders the path to an internal alpha image and composites with the layer pixels. This is a post-processing effect that requires an active layer created with `beginLayer()` -- calling without one triggers "You need to create a layer for applying a mask". The `path` parameter must be a valid `Path` object or the method reports "No valid path object supplied".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | ScriptObject | no | A Path object defining the mask shape | Must be a valid Path object |
| area | Array | no | Bounding rectangle `[x, y, w, h]` to scale the path into | -- |
| invert | Integer | no | If true, areas outside the path are kept instead of inside | Boolean value |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.
- The path is scaled to fit the area with non-uniform scaling (`scaleToFit` with `preserveProportions=false`). The mask shape may be distorted if the path's aspect ratio does not match the area's aspect ratio.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.fillPath$`

**Example:**
```javascript:applymask-circular
// Title: Circular mask on a layer
Panel1.setPaintRoutine(function(g)
{
    var area = [0, 0, this.getWidth(), this.getHeight()];

    g.beginLayer(false);
    g.fillAll(0xFFFF0000);
    g.setColour(0xFF0000FF);
    g.fillRect([20, 20, 60, 60]);

    var p = Content.createPath();
    p.addEllipse(area);
    g.applyMask(p, area, false);
    g.endLayer();
});
```
```json:testMetadata:applymask-circular
{
  "testable": false,
  "skipReason": "Visual output only -- mask effect cannot be verified programmatically"
}
```

## applySepia

**Signature:** `undefined applySepia()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.applySepia();`

**Description:**
Applies an old-school sepia tone filter to the current layer's pixel data, converting colours to warm brownish tones. Takes no parameters -- the sepia tone curve is fixed. This is a post-processing effect that requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for applySepia".

**Parameters:**

(None)

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.desaturate$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.applyGradientMap$`
- `$API.Graphics.applyGamma$`

## applyShader

**Signature:** `Integer applyShader(ScriptObject shader, Array area)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action and accesses OpenGL context state.
**Minimal Example:** `var ok = {obj}.applyShader(myShader, [0, 0, 200, 200]);`

**Description:**
Applies an OpenGL shader to the specified rectangular area within the current rendering. The shader must be a `ScriptShader` object. The method adds a draw action that, during the UI-thread render pass, compiles the shader (if dirty), sets up global/local coordinate bounds, configures OpenGL blending, and renders the shader output into the specified area. Returns `true` (1) if the shader object was valid and the action was queued, or `false` (0) if the shader parameter was not a valid `ScriptShader` object. Note that a return of `true` only means the action was enqueued -- actual shader compilation errors are logged to the debug console on the UI thread during rendering (backend builds only). This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shader | ScriptObject | no | A ScriptShader object containing the GLSL shader code | Must be a valid ScriptShader |
| area | Array | no | Rendering area `[x, y, w, h]` for the shader output | Converted to nearest integer rectangle |

**Pitfalls:**
- Returns `false` silently when the `shader` parameter is not a `ScriptShader` object -- no error message is produced.
- Shader compilation errors are deferred to the render pass and only logged in backend (HISE IDE) builds. In exported plugins, a failed shader silently renders nothing or uses a cached previous frame.
- Requires OpenGL to be enabled. If the OpenGL context is not active, the backend logs "Open GL is not enabled" during rendering.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.drawImage$`

## applySharpness

**Signature:** `undefined applySharpness(Number delta)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.applySharpness(50);`

**Description:**
Applies a sharpness or softening filter to the current layer's pixel data. Positive values sharpen the image (enhance edges), negative values soften it, and zero produces no change. The effect is implemented via `PostGraphicsRenderer::applySharpness()` operating on the offscreen layer image. This is a post-processing effect that requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for applySharpness".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| delta | Number | no | Sharpness amount. Positive sharpens, negative softens, 0 = no change. | Cast to int internally |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.gaussianBlur$`
- `$API.Graphics.boxBlur$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.desaturate$`

## applyVignette

**Signature:** `undefined applyVignette(Double amount, Double radius, Double falloff)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.applyVignette(0.5, 0.8, 0.5);`

**Description:**
Applies a vignette effect (darkened corners) to the current layer's pixel data. The three parameters control the intensity, size, and transition curve of the corner darkening. This is a post-processing effect implemented via `PostGraphicsRenderer::applyVignette()` that requires an active layer created with `beginLayer()` -- calling without one triggers a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| amount | Double | no | Intensity of the darkening effect | -- |
| radius | Double | no | Radius of the unaffected center area (larger = smaller vignette) | -- |
| falloff | Double | no | Transition curve from clear center to darkened edges | -- |

**Pitfalls:**
- [BUG] Requires an active layer, but the error message when no layer is present says "You need to create a layer for applySepia" instead of "applyVignette" (copy-paste error from the `applySepia` method).
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.applyGamma$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.gaussianBlur$`
- `$API.Graphics.desaturate$`

## beginBlendLayer

**Signature:** `undefined beginBlendLayer(String blendMode, Double alpha)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new BlendingLayer object and pushes it onto the layer stack.
**Minimal Example:** `{obj}.beginBlendLayer("Multiply", 0.8);`

**Description:**
Begins a new layer that composites using the specified blend mode. All subsequent draw calls are rendered to an offscreen image, and when `endLayer()` is called, the offscreen image is composited onto the parent canvas using the specified blend mode and alpha. This is an alternative to `beginLayer()` that provides Photoshop-style blend mode compositing via the gin library (25 modes). Post-processing effects (blur, desaturate, HSL, etc.) can also be applied to blend layers before ending them.

The blend mode must be one of 25 supported string values (case-sensitive). An invalid blend mode string causes the method to silently fail -- the blend layer is not created and no error is reported.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| blendMode | String | no | Blend mode name (case-sensitive) | Must be one of the 25 supported blend modes |
| alpha | Double | no | Opacity of the composited layer (0.0 = fully transparent, 1.0 = fully opaque) | -- |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Normal" | Standard alpha compositing with no blending effect |
| "Lighten" | Keeps the lighter pixel of the two layers |
| "Darken" | Keeps the darker pixel of the two layers |
| "Multiply" | Multiplies the pixel values, darkening the result |
| "Average" | Averages the pixel values of both layers |
| "Add" | Adds the pixel values, brightening the result |
| "Subtract" | Subtracts the top layer from the bottom, darkening |
| "Difference" | Absolute difference between the two layers |
| "Negation" | Inverted difference, producing a softer contrast effect |
| "Screen" | Inverse of Multiply, lightening the result |
| "Exclusion" | Similar to Difference but with lower contrast |
| "Overlay" | Combines Multiply and Screen based on base brightness |
| "SoftLight" | Subtle version of Overlay with gentler transitions |
| "HardLight" | Combines Multiply and Screen based on top layer brightness |
| "ColorDodge" | Brightens the base colour to reflect the blend layer |
| "ColorBurn" | Darkens the base colour to reflect the blend layer |
| "LinearDodge" | Same as Add -- adds the pixel values |
| "LinearBurn" | Adds the pixel values then subtracts white |
| "LinearLight" | Combines LinearDodge and LinearBurn based on blend brightness |
| "VividLight" | Combines ColorDodge and ColorBurn based on blend brightness |
| "PinLight" | Replaces pixels based on brightness comparison |
| "HardMix" | Reduces colours to pure black or white based on threshold |
| "Reflect" | Produces a bright reflective effect |
| "Glow" | Inverse of Reflect, applied to the base |
| "Phoenix" | Adds the minimum and subtracts the maximum of each channel |

**Pitfalls:**
- [BUG] Invalid blend mode strings silently fail. The method returns without creating a layer or reporting an error. Subsequent draw calls go to the parent canvas instead of the intended blend layer.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`

**Example:**
```javascript:beginblendlayer-multiply
// Title: Multiply-blending a coloured overlay onto content
Panel1.setPaintRoutine(function(g)
{
    g.fillAll(0xFFFFFFFF);
    g.setColour(0xFF000000);
    g.setFont("Arial", 20.0);
    g.drawAlignedText("Base Text", [10, 10, 200, 40], "left");

    g.beginBlendLayer("Multiply", 1.0);
    g.fillAll(0xFFFF6600);
    g.endLayer();
});
```
```json:testMetadata:beginblendlayer-multiply
{
  "testable": false,
  "skipReason": "Visual output only -- blend mode effect cannot be verified programmatically"
}
```

## beginLayer

**Signature:** `undefined beginLayer(Integer drawOnParent)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ActionLayer object and pushes it onto the layer stack.
**Minimal Example:** `{obj}.beginLayer(false);`

**Description:**
Starts a new offscreen layer. All subsequent draw calls are recorded into this layer's internal action list instead of the main canvas. Post-processing effects (gaussianBlur, boxBlur, desaturate, applyHSL, applyGamma, applyGradientMap, applyMask, applySharpness, applySepia, applyVignette) can then be applied to the layer before calling `endLayer()`, which renders the layer's contents to an offscreen image, applies the post-processing effects via `PostGraphicsRenderer`, and composites the result back onto the parent canvas. Layers can be nested.

The `drawOnParent` parameter controls whether the layer renders its own contents to a fresh transparent image (`false`) or captures the parent's existing content as the starting point (`true`). When `true`, the layer's cached image mechanism copies the parent canvas, allowing post-processing effects to operate on the combined result. When `false`, the layer starts blank and its rendered content is composited on top of the parent.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| drawOnParent | Integer | no | If true, the layer captures the parent's existing content as its starting image | Boolean value |

**Pitfalls:**
- Every `beginLayer()` must be matched with a corresponding `endLayer()`. Forgetting `endLayer()` leaves the layer on the stack and subsequent draw calls continue targeting the layer instead of the main canvas.

**Cross References:**
- `$API.Graphics.endLayer$`
- `$API.Graphics.beginBlendLayer$`
- `$API.Graphics.gaussianBlur$`
- `$API.Graphics.boxBlur$`
- `$API.Graphics.desaturate$`
- `$API.Graphics.applyMask$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.applyGamma$`
- `$API.Graphics.applyGradientMap$`
- `$API.Graphics.applySepia$`
- `$API.Graphics.applySharpness$`
- `$API.Graphics.applyVignette$`

**DiagramRef:** graphics-layer-stack

**Example:**
```javascript:beginlayer-blur-effect
// Title: Applying a gaussian blur to a layer
Panel1.setPaintRoutine(function(g)
{
    g.beginLayer(false);
    g.fillAll(0xFF333333);
    g.setColour(0xFFFFFFFF);
    g.fillRect([40, 40, 80, 80]);
    g.gaussianBlur(10);
    g.endLayer();
});
```
```json:testMetadata:beginlayer-blur-effect
{
  "testable": false,
  "skipReason": "Visual output only -- blur effect cannot be verified programmatically"
}
```

## boxBlur

**Signature:** `undefined boxBlur(Number blurAmount)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.boxBlur(5);`

**Description:**
Applies a box blur (uniform average filter) to the current layer's pixel data. Box blur is computationally cheaper than gaussian blur but produces a less natural-looking blur with visible block artifacts at large radii. The `blurAmount` parameter controls the blur kernel size -- larger values produce a stronger blur. The value is clamped to the range 0-100. This is a post-processing effect that requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for box blur".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| blurAmount | Number | no | Blur radius in pixels | Clamped to 0-100; cast to int |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.gaussianBlur$`

## desaturate

**Signature:** `undefined desaturate()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.desaturate();`

**Description:**
Removes all colour saturation from the current layer's pixel data, converting it to grayscale. Takes no parameters. This is a post-processing effect implemented via `PostGraphicsRenderer::desaturate()` that requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for desaturating".

**Parameters:**

(None)

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.applyHSL$`
- `$API.Graphics.applySepia$`
- `$API.Graphics.applyGradientMap$`

## drawAlignedText

**Signature:** `undefined drawAlignedText(String text, Array area, String alignment)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action containing a String copy.
**Minimal Example:** `{obj}.drawAlignedText("Hello", [10, 10, 200, 30], "left");`

**Description:**
Draws a single line of text within the specified rectangular area using the specified alignment. The text is rendered using the current font (set via `setFont` or `setFontWithSpacing`) and the current colour (set via `setColour` or `setGradientFill`). The alignment string controls both horizontal and vertical placement within the area.

This method is the preferred replacement for the deprecated `drawText` method, which only supports centred alignment. It does NOT require an active layer -- it adds a regular draw action.

The alignment must be one of 11 supported string values (see Value Descriptions). An invalid alignment string triggers a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | The text to render | -- |
| area | Array | no | Bounding rectangle `[x, y, w, h]` for text placement | -- |
| alignment | String | no | Text alignment within the area | Must be a valid alignment string |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "left" | Left-aligned, vertically centred |
| "right" | Right-aligned, vertically centred |
| "top" | Horizontally centred, top-aligned |
| "bottom" | Horizontally centred, bottom-aligned |
| "centred" | Centred both horizontally and vertically |
| "centredTop" | Horizontally centred, top-aligned |
| "centredBottom" | Horizontally centred, bottom-aligned |
| "topLeft" | Left-aligned, top-aligned |
| "topRight" | Right-aligned, top-aligned |
| "bottomLeft" | Left-aligned, bottom-aligned |
| "bottomRight" | Right-aligned, bottom-aligned |

**Pitfalls:**
- Uses British spelling "centred" not American "center". Passing "center", "centered", or "Centre" triggers a script error.

**Cross References:**
- `$API.Graphics.drawAlignedTextShadow$`
- `$API.Graphics.drawMultiLineText$`
- `$API.Graphics.drawMarkdownText$`
- `$API.Graphics.drawText$`
- `$API.Graphics.setFont$`
- `$API.Graphics.setColour$`

## drawAlignedTextShadow

**Signature:** `undefined drawAlignedTextShadow(String text, Array area, String alignment, JSON shadowData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, constructs a melatonin shadow object.
**Minimal Example:** `{obj}.drawAlignedTextShadow("Title", [10, 10, 200, 40], "left", {"Colour": 0x88000000, "Radius": 4, "Offset": [2, 2]});`

**Description:**
Renders a blurred shadow behind text at the specified alignment. The shadow is rendered using the melatonin blur library -- either as a `melatonin::DropShadow` or `melatonin::InnerShadow` depending on the `Inner` property in `shadowData`. The text itself is NOT drawn -- only the shadow. To display both text and its shadow, call `drawAlignedTextShadow` first, then `drawAlignedText` with the same text, area, and alignment.

The text uses the current font (set via `setFont` or `setFontWithSpacing`) and the shadow colour is specified in the `shadowData` object. The alignment parameter uses the same string values as `drawAlignedText`. Invalid alignment strings or non-object `shadowData` trigger a script error.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | The text whose shadow to render | -- |
| area | Array | no | Bounding rectangle `[x, y, w, h]` for text placement | -- |
| alignment | String | no | Text alignment within the area | Must be a valid alignment string |
| shadowData | JSON | no | Shadow configuration object | Must be a JSON object |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Colour | Colour | Shadow colour in 0xAARRGGBB format (defaults to 0xFF000000) |
| Offset | Array | Shadow offset as `[x, y]` pixel values (defaults to [0, 0]) |
| Radius | Integer | Blur radius for the shadow in pixels (defaults to 0) |
| Spread | Integer | Shadow spread in pixels (defaults to 0) |
| Inner | Integer | `true` for inner shadow, `false` for drop shadow (defaults to false) |

**Pitfalls:**
- This method only renders the shadow, not the text. Call `drawAlignedText` separately with the same parameters to draw the visible text on top of the shadow.
- The `shadowData` parameter must be a JSON object. Passing a non-object value (e.g., a number or string) triggers the script error "shadowData needs to be a JSON object with the shadow parameters".
- Uses British spelling "centred" for alignment, same as `drawAlignedText`.

**Cross References:**
- `$API.Graphics.drawAlignedText$`
- `$API.Graphics.drawDropShadow$`
- `$API.Graphics.drawDropShadowFromPath$`
- `$API.Graphics.setFont$`
- `$API.Graphics.setColour$`

**Example:**
```javascript:drawalignedtextshadow-usage
// Title: Text with a drop shadow
Panel1.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setFont("Arial", 24.0);

    var area = [20, 20, 200, 40];
    var shadow = {"Colour": 0x88000000, "Radius": 5, "Offset": [2, 3]};

    g.drawAlignedTextShadow("Hello World", area, "left", shadow);
    g.setColour(0xFFFFFFFF);
    g.drawAlignedText("Hello World", area, "left");
});
```
```json:testMetadata:drawalignedtextshadow-usage
{
  "testable": false,
  "skipReason": "Visual output only -- text shadow rendering cannot be verified programmatically"
}
```

## drawDropShadow

**Signature:** `undefined drawDropShadow(Array area, Colour colour, Number radius)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, creates a JUCE DropShadow object and a melatonin::DropShadow, copies a Path.
**Minimal Example:** `{obj}.drawDropShadow([10, 10, 100, 50], 0x88000000, 10);`

**Description:**
Draws a rectangular drop shadow around the specified area using the melatonin blur library. The shadow is rendered outside the rectangle defined by `area`, simulating a light source casting a shadow behind a rectangular element. The colour controls both the shadow tint and opacity (use the alpha channel of the colour for transparency). The `radius` parameter controls the blur spread -- larger values produce a softer, more diffused shadow.

Internally, the rectangle is converted to a Path and rendered via `melatonin::DropShadow::render()`. The shadow offset defaults to (0, 0). For shadows with custom offsets, use `drawDropShadowFromPath` instead. This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Rectangle `[x, y, w, h]` defining the shape that casts the shadow | Converted to integer rectangle via `getIntRectangleFromVar` |
| colour | Colour | no | Shadow colour in 0xAARRGGBB format | Alpha channel controls shadow opacity |
| radius | Number | no | Blur radius in pixels | 0 produces no visible shadow |

**Pitfalls:**
- The area is converted to integer coordinates via `getIntRectangleFromVar`, so sub-pixel positioning is lost. For float-precision shadows, use `drawDropShadowFromPath` with a rectangular path.
- The shadow offset is fixed at (0, 0). The shadow extends equally in all directions. Use `drawDropShadowFromPath` if you need a directional shadow with an offset.

**Cross References:**
- `$API.Graphics.drawDropShadowFromPath$`
- `$API.Graphics.drawInnerShadowFromPath$`
- `$API.Graphics.addDropShadowFromAlpha$`
- `$API.Graphics.drawAlignedTextShadow$`

## drawDropShadowFromPath

**Signature:** `undefined drawDropShadowFromPath(ScriptObject path, Array area, Colour colour, Number radius, Array offset)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, copies the Path, creates a melatonin::DropShadow object.
**Minimal Example:** `{obj}.drawDropShadowFromPath(myPath, [0, 0, 200, 200], 0x88000000, 8, [2, 3]);`

**Description:**
Draws a drop shadow that follows the outline of a Path object, using the melatonin blur library. The path is scaled to fit the specified area (non-uniform scaling via `scaleToFit`), then the shadow is rendered at the specified blur radius and offset. The shadow extends outward from the path boundary.

The `offset` parameter is a `[x, y]` point that shifts the shadow position relative to the path. Positive x shifts right, positive y shifts down. The offset is converted to integer coordinates.

If the path is empty (has no bounds), the method silently does nothing -- no draw action is added and no error is reported.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | ScriptObject | no | A Path object defining the shadow shape | Must be a valid Path object |
| area | Array | no | Bounding rectangle `[x, y, w, h]` to scale the path into | Float precision |
| colour | Colour | no | Shadow colour in 0xAARRGGBB format | Alpha channel controls shadow opacity |
| radius | Number | no | Blur radius in pixels | -- |
| offset | Array | no | Shadow offset as `[x, y]` pixel values | Converted to integer point |

**Pitfalls:**
- If the `path` parameter is not a valid Path object, the method silently does nothing -- no error is reported.
- The path is scaled to fit the area with non-uniform scaling (`scaleToFit` with `preserveProportions=false`). If the path's aspect ratio does not match the area's, the shadow shape may be distorted.
- The offset is converted to integer coordinates, so sub-pixel shadow offsets are not supported.

**Cross References:**
- `$API.Graphics.drawInnerShadowFromPath$`
- `$API.Graphics.drawDropShadow$`
- `$API.Graphics.addDropShadowFromAlpha$`

**Example:**
```javascript:drawdropshadowfrompath-rounded
// Title: Drop shadow behind a rounded rectangle path
Panel1.setPaintRoutine(function(g)
{
    var area = [20, 20, 160, 80];

    var p = Content.createPath();
    p.addRoundedRectangle(area, 10.0);

    g.drawDropShadowFromPath(p, area, 0x88000000, 12, [3, 4]);
    g.setColour(0xFFEEEEEE);
    g.fillPath(p, area);
});
```
```json:testMetadata:drawdropshadowfrompath-rounded
{
  "testable": false,
  "skipReason": "Visual output only -- shadow rendering cannot be verified programmatically"
}
```

## drawEllipse

**Signature:** `undefined drawEllipse(Array area, Double lineThickness)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.drawEllipse([10, 10, 80, 80], 2.0);`

**Description:**
Draws an ellipse outline (stroke only, not filled) within the specified rectangular area using the current colour. The ellipse is inscribed in the bounding rectangle -- a square area produces a circle. The `lineThickness` parameter controls the width of the outline stroke. To draw a filled ellipse, use `fillEllipse` instead.

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if `setColour`/`setGradientFill` has not been called before drawing, or if the area is passed as separate arguments instead of an array.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` for the ellipse | Float precision |
| lineThickness | Double | no | Width of the outline stroke in pixels | -- |

**Cross References:**
- `$API.Graphics.fillEllipse$`
- `$API.Graphics.drawRect$`
- `$API.Graphics.setColour$`

## drawFFTSpectrum

**Signature:** `undefined drawFFTSpectrum(ScriptObject fftObject, Array area)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, copies a spectrum Image from the FFT object.
**Minimal Example:** `{obj}.drawFFTSpectrum(myFFT, [0, 0, 200, 100]);`

**Description:**
Draws the 2D spectrum image of an FFT object into the specified rectangular area. The FFT object must be a valid `FFT` scripting object (created via `Engine.createFFT()`). The spectrum image is rendered using `Spectrum2D::draw()` with the quality setting configured on the FFT object via `setSpectrum2DParameters`.

The method retrieves the current spectrum image from the FFT object using `getSpectrum(false)` (non-blocking) and queues a draw action that renders this snapshot into the specified bounds.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fftObject | ScriptObject | no | An FFT object containing the spectrum data | Must be a valid FFT object |
| area | Array | no | Rendering bounds `[x, y, w, h]` for the spectrum display | Converted to nearest integer rectangle |

**Pitfalls:**
- [BUG] If the `fftObject` parameter is not a valid FFT object, the error message says "not a SVG object" instead of "not a FFT object". This is a copy-paste error from the `drawSVG` method.

**Cross References:**
- `$API.Graphics.drawImage$`
- `$API.Graphics.drawSVG$`

## drawFittedText

**Disabled:** no-op
**Disabled Reason:** The method has a wrapper and full implementation but is NOT registered via `ADD_API_METHOD_5` in the constructor. It is inaccessible from HISEScript. Use `drawAlignedText` or `drawMultiLineText` instead.

**Cross References:**
- `$API.Graphics.drawAlignedText$`
- `$API.Graphics.drawMultiLineText$`

## drawHorizontalLine

**Signature:** `undefined drawHorizontalLine(Number y, Double x1, Double x2)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.drawHorizontalLine(50, 10.0, 190.0);`

**Description:**
Draws a horizontal line at the specified integer y-coordinate from `x1` to `x2` using the current colour. The line is drawn without sub-pixel interpolation (pixel-snapped), making it suitable for crisp single-pixel separators and grid lines. The y-coordinate is an integer for pixel-perfect alignment; the x start and end positions are floats.

The line width is always 1 pixel. For thicker horizontal lines, use `drawLine` instead.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| y | Number | no | Vertical position of the line in pixels | Integer; no sub-pixel positioning |
| x1 | Double | no | Start x-coordinate | SANITIZED against NaN/Inf |
| x2 | Double | no | End x-coordinate | SANITIZED against NaN/Inf |

**Cross References:**
- `$API.Graphics.drawVerticalLine$`
- `$API.Graphics.drawLine$`
- `$API.Graphics.setColour$`

## drawImage

**Signature:** `undefined drawImage(String imageName, Array area, Number xOffset, Number yOffset)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new draw actions, resolves image by name from the parent panel or LAF.
**Minimal Example:** `{obj}.drawImage("myImage.png", [0, 0, 200, 100], 0, 0);`

**Description:**
Draws a previously loaded image into the specified rectangular area. Images must be loaded in advance using `ScriptPanel.loadImage()` or `ScriptLookAndFeel.loadImage()` -- the `imageName` string references the loaded image by its filename or path key. This method only works within ScriptPanel paint routines or ScriptedLookAndFeel drawing functions. Calling it in any other context triggers the script error "drawImage is only allowed in a panel's paint routine".

The image is scaled to fit the target area's width. The `yOffset` parameter selects a vertical offset into the source image in pixels, enabling filmstrip-style animations where multiple frames are stacked vertically in a single image. The `xOffset` parameter is declared but explicitly ignored in the implementation.

If the image name does not match any loaded image, a grey placeholder rectangle with "XXX" text is drawn and a debug error "Image [name] not found" is logged (backend builds only).

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| imageName | String | no | Name/path key of a previously loaded image | Must match a loaded image |
| area | Array | no | Target rectangle `[x, y, w, h]` to draw the image into | Zero width produces no output |
| xOffset | Number | no | Horizontal offset into the source image | Ignored in current implementation |
| yOffset | Number | no | Vertical offset into the source image in pixels | Used for filmstrip frame selection |

**Pitfalls:**
- The `xOffset` parameter is silently ignored. The C++ signature declares it as `int /*xOffset*/` -- only `yOffset` is used. If you need horizontal offset behavior, crop the image externally or use path-based clipping.
- Only works in ScriptPanel paint routines and ScriptedLookAndFeel drawing functions. Calling from DynamicContainer child paint routines or other contexts produces a script error.
- [BUG] When the image is not found, the placeholder rendering overwrites the current colour state (sets it to grey then black). Drawing operations after a failed `drawImage` call will use black as the current colour unless `setColour` is called again.

**Cross References:**
- `$API.Graphics.drawSVG$`
- `$API.Graphics.fillPath$`

**Example:**
```javascript:drawimage-filmstrip
// Title: Drawing an image with filmstrip frame selection
Panel1.loadImage("filmstrip.png", "filmstrip.png");
Panel1.setPaintRoutine(function(g)
{
    var frameIndex = parseInt(this.getValue() * 10);
    var frameHeight = 50;
    g.drawImage("filmstrip.png", [0, 0, 100, 50], 0, frameIndex * frameHeight);
});
```
```json:testMetadata:drawimage-filmstrip
{
  "testable": false,
  "skipReason": "Requires a preloaded image resource that cannot be created via API"
}
```

## drawInnerShadowFromPath

**Signature:** `undefined drawInnerShadowFromPath(ScriptObject path, Array area, Colour colour, Number radius, Array offset)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, copies the Path, creates a melatonin::InnerShadow object.
**Minimal Example:** `{obj}.drawInnerShadowFromPath(myPath, [0, 0, 200, 200], 0x88000000, 6, [2, 2]);`

**Description:**
Draws an inner shadow inside the outline of a Path object, using the melatonin blur library. Unlike `drawDropShadowFromPath` which renders a shadow extending outward from the path, this method renders the shadow extending inward, creating an inset/recessed visual effect. The shadow is rendered via `melatonin::InnerShadow`.

The path is scaled to fit the specified area (non-uniform scaling via `scaleToFit`). The `offset` parameter shifts the light source direction -- positive x shifts the shadow to the right side (light from left), positive y shifts it downward (light from above). The offset is converted to integer coordinates.

If the path is empty (has no bounds), the method silently does nothing. If the `path` parameter is not a valid Path object, the method silently does nothing -- no error is reported.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | ScriptObject | no | A Path object defining the shape for the inner shadow | Must be a valid Path object |
| area | Array | no | Bounding rectangle `[x, y, w, h]` to scale the path into | Float precision |
| colour | Colour | no | Shadow colour in 0xAARRGGBB format | Alpha channel controls shadow intensity |
| radius | Number | no | Blur radius in pixels | -- |
| offset | Array | no | Shadow offset as `[x, y]` pixel values, controls light direction | Converted to integer point |

**Pitfalls:**
- If the `path` parameter is not a valid Path object, the method silently does nothing -- no error is reported.
- The path is scaled to fit the area with non-uniform scaling. The shadow shape may be distorted if the path's aspect ratio does not match the area's.
- The offset is converted to integer coordinates, so sub-pixel shadow offsets are not supported.

**Cross References:**
- `$API.Graphics.drawDropShadowFromPath$`
- `$API.Graphics.drawDropShadow$`
- `$API.Graphics.drawAlignedTextShadow$`

**Example:**
```javascript:drawinnershadowfrompath-inset
// Title: Inner shadow for an inset panel effect
Panel1.setPaintRoutine(function(g)
{
    var area = [10, 10, 180, 80];

    var p = Content.createPath();
    p.addRoundedRectangle(area, 8.0);

    g.setColour(0xFF555555);
    g.fillPath(p, area);
    g.drawInnerShadowFromPath(p, area, 0xAA000000, 8, [2, 3]);
});
```
```json:testMetadata:drawinnershadowfrompath-inset
{
  "testable": false,
  "skipReason": "Visual output only -- inner shadow rendering cannot be verified programmatically"
}
```

## drawLine

**Signature:** `undefined drawLine(Double x1, Double x2, Double y1, Double y2, Double lineThickness)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.drawLine(10.0, 190.0, 20.0, 80.0, 2.0);`

**Description:**
Draws a straight line from point (x1, y1) to point (x2, y2) with the specified stroke thickness, using the current colour. The parameters group x-coordinates before y-coordinates rather than using a point-by-point order: the signature is `(x1, x2, y1, y2, thickness)`, not `(x1, y1, x2, y2, thickness)`. Internally, the coordinates are reordered so the line draws correctly from (x1, y1) to (x2, y2) as expected. All float parameters are SANITIZED against NaN/Inf values.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x1 | Double | no | Start x-coordinate of the line | SANITIZED |
| x2 | Double | no | End x-coordinate of the line | SANITIZED |
| y1 | Double | no | Start y-coordinate of the line | SANITIZED |
| y2 | Double | no | End y-coordinate of the line | SANITIZED |
| lineThickness | Double | no | Stroke width in pixels | SANITIZED |

**Pitfalls:**
- The parameter order is `(x1, x2, y1, y2)`, grouping x-coordinates then y-coordinates. This is unusual -- most drawing APIs use `(x1, y1, x2, y2)` point-by-point order. Despite the unusual order, the line draws correctly from (x1, y1) to (x2, y2).

**Cross References:**
- `$API.Graphics.drawHorizontalLine$`
- `$API.Graphics.drawVerticalLine$`
- `$API.Graphics.setColour$`

## drawMarkdownText

**Signature:** `undefined drawMarkdownText(ScriptObject markdownRenderer)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Accesses the MarkdownRenderer's internal draw action and adds it to the handler.
**Minimal Example:** `{obj}.drawMarkdownText(myMarkdown);`

**Description:**
Draws the text of a `MarkdownRenderer` object to its previously specified area. The `markdownRenderer` parameter must be a `MarkdownRenderer` object (created via `Content.createMarkdownRenderer()`). Before calling this method, `setTextBounds()` must be called on the MarkdownRenderer to define the rendering area -- calling `drawMarkdownText` without setting text bounds triggers the script error "You have to call setTextBounds() before using this method".

The MarkdownRenderer handles its own text layout, formatting, and rendering. The Graphics object simply incorporates the renderer's internal draw action into its action list. The current colour and font set on the Graphics object do NOT affect the markdown rendering -- the MarkdownRenderer uses its own styling.

If the parameter is not a valid MarkdownRenderer object, the script error "not a markdown renderer" is reported.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| markdownRenderer | ScriptObject | no | A MarkdownRenderer object with text bounds set | Must have setTextBounds() called first |

**Pitfalls:**
- The MarkdownRenderer must have `setTextBounds()` called before this method. Without it, the render area is empty and the method reports a script error.

**Cross References:**
- `$API.Graphics.drawAlignedText$`
- `$API.Graphics.drawMultiLineText$`
- `$API.Graphics.drawAlignedTextShadow$`

## drawMultiLineText

**Signature:** `undefined drawMultiLineText(String text, Array xy, Number maxWidth, String alignment, Double leading)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action containing a String copy.
**Minimal Example:** `{obj}.drawMultiLineText("Line one and more text", [10, 20], 180, "left", 0.0);`

**Description:**
Draws text that automatically wraps to new lines when it exceeds the specified `maxWidth`. The text starts at the position defined by the `xy` parameter (a 2-element `[x, y]` array representing the starting point). The `y` coordinate is the baseline position of the first line of text. The `alignment` parameter controls the horizontal alignment of each wrapped line relative to the `maxWidth` boundary. The `leading` parameter controls the extra vertical spacing between lines (in addition to the default font-height-based line spacing).

Unlike `drawAlignedText`, which positions text within a bounding rectangle, `drawMultiLineText` uses a starting point and maximum width, making it better suited for flowing paragraph text that may span multiple lines.

The alignment must be a valid alignment string. An invalid alignment string triggers a script error.

This method has the `CHECK_FONT_AND_COLOUR` diagnostic in backend builds, which warns at parse time if `setFont`/`setFontWithSpacing` or `setColour`/`setGradientFill` have not been called before drawing.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | The text to render, wrapping at maxWidth | -- |
| xy | Array | no | Starting position as `[x, y]` where y is the baseline | 2-element array; values cast to int |
| maxWidth | Number | no | Maximum width in pixels before text wraps to next line | Cast to int |
| alignment | String | no | Horizontal alignment of wrapped lines | Must be a valid alignment string |
| leading | Double | no | Extra vertical spacing between lines | 0.0 = default spacing |

**Pitfalls:**
- The `xy` parameter is a `[x, y]` point (2 elements), NOT an `[x, y, w, h]` area like most other Graphics methods. Passing a 4-element area array will use only the first two elements and ignore the rest.
- The `y` value is the text baseline position, not the top of the text. The first line of text will appear slightly above the y coordinate.
- Both `xy` values and `maxWidth` are cast to `int`, so sub-pixel positioning is not available.

**Cross References:**
- `$API.Graphics.drawAlignedText$`
- `$API.Graphics.drawAlignedTextShadow$`
- `$API.Graphics.drawMarkdownText$`
- `$API.Graphics.setFont$`
- `$API.Graphics.setColour$`

## drawPath

**Signature:** `undefined drawPath(ScriptObject path, Array area, NotUndefined strokeStyle)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, copies the Path, and creates a PathStrokeType.
**Minimal Example:** `{obj}.drawPath(myPath, [0, 0, 200, 100], 2.0);`

**Description:**
Draws the outline (stroke) of a Path object using the current colour. The `path` must be a valid `Path` scripting object. The `area` parameter is optional -- if it is an array (or ScriptRectangle), the path is scaled to fit the specified `[x, y, w, h]` rectangle using non-uniform scaling (`scaleToFit` with `preserveProportions=false`). If `area` is not an array or ScriptRectangle, the path is drawn at its original coordinates without scaling.

The `strokeStyle` parameter controls the stroke characteristics and accepts two formats:

1. **Number**: Simple stroke thickness (e.g., `2.0`).
2. **JSON object**: Advanced stroke configuration with properties:
   - `Thickness` (float): Stroke width
   - `EndCapStyle` (string): `"butt"`, `"square"`, or `"rounded"`
   - `JointStyle` (string): `"mitered"`, `"curved"`, or `"beveled"`

If the path is empty (has no bounds) and an area is specified, the method silently returns without drawing. If the area rectangle is empty, the method also silently returns. If no area is specified and the path is empty, the stroke still attempts to render (though nothing visible is produced).

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | ScriptObject | no | A Path object defining the shape to stroke | Must be a valid Path object |
| area | Array | no | Optional bounding rectangle `[x, y, w, h]` to scale the path into | Path drawn at original coords if not an array |
| strokeStyle | NotUndefined | no | Stroke thickness (number) or stroke configuration (JSON) | Thickness SANITIZED |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Thickness | Double | Stroke width in pixels |
| EndCapStyle | String | Cap style for open path ends: "butt", "square", or "rounded" |
| JointStyle | String | Style for path segment joints: "mitered", "curved", or "beveled" |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "butt" | Flat cap that ends exactly at the path endpoint |
| "square" | Extends the cap beyond the endpoint by half the stroke thickness |
| "rounded" | Semicircular cap extending beyond the endpoint |
| "mitered" | Pointed joints that extend to a sharp corner |
| "curved" | Rounded joints with a smooth curve |
| "beveled" | Flat-cut joints creating a chamfered corner |

**Pitfalls:**
- If the `path` parameter is not a valid Path object, the `dynamic_cast` fails and the method silently does nothing -- no error is reported.
- When using the JSON stroke style, an unrecognized `EndCapStyle` or `JointStyle` string causes `StringArray::indexOf` to return -1, which is cast to the enum type. This produces a default stroke type rather than an error.

**Cross References:**
- `$API.Graphics.fillPath$`
- `$API.Graphics.drawDropShadowFromPath$`
- `$API.Graphics.drawInnerShadowFromPath$`
- `$API.Graphics.setColour$`

**Example:**
```javascript:drawpath-custom-stroke
// Title: Drawing a path with rounded end caps
const var p = Content.createPath();
p.startNewSubPath(0.0, 0.5);
p.lineTo(0.3, 0.0);
p.lineTo(0.7, 1.0);
p.lineTo(1.0, 0.5);

Panel1.setPaintRoutine(function(g)
{
    g.setColour(0xFFFF6600);
    g.drawPath(p, [10, 10, 180, 80], {
        "Thickness": 3.0,
        "EndCapStyle": "rounded",
        "JointStyle": "curved"
    });
});
```
```json:testMetadata:drawpath-custom-stroke
{
  "testable": false,
  "skipReason": "Visual output only -- path stroke rendering cannot be verified programmatically"
}
```

## drawRect

**Signature:** `undefined drawRect(Array area, Double borderSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.drawRect([10, 10, 100, 50], 2.0);`

**Description:**
Draws a rectangle outline (stroke only, not filled) within the specified area using the current colour. The `borderSize` parameter controls the width of the outline stroke. To draw a filled rectangle, use `fillRect` instead.

The area is specified as a `[x, y, width, height]` array. The border is drawn inward from the rectangle edges (the outer boundary of the stroke aligns with the area bounds).

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if:
- `setColour`/`setGradientFill` has not been called before drawing
- The area is passed as separate arguments instead of a single array

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` | Float precision |
| borderSize | Double | no | Width of the outline stroke in pixels | SANITIZED |

**Cross References:**
- `$API.Graphics.fillRect$`
- `$API.Graphics.drawRoundedRectangle$`
- `$API.Graphics.drawEllipse$`
- `$API.Graphics.setColour$`

## drawRepaintMarker

**Signature:** `undefined drawRepaintMarker(String label)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action and a StringBuilder.
**Minimal Example:** `{obj}.drawRepaintMarker("MyPanel");`

**Description:**
Fills the entire component area with a random semi-transparent colour (30% opacity, HSL with 0.33 saturation, 0.5 lightness, random hue) to visually indicate that a UI repaint has occurred. Each call produces a different random colour, making it easy to see how often a component is being repainted -- rapid colour changes indicate excessive repainting.

The `label` parameter is a string identifier used for profiling. When HISE is built with Perfetto profiling support (`PERFETTO` preprocessor), the label is used to create a Perfetto counter track and trace event, enabling repaint frequency analysis in the Perfetto trace viewer. If the label is empty, the trace uses "anonymous repaint".

This is a debugging/diagnostic tool. It overwrites whatever is currently drawn on the component, so it should be placed at the start of a paint routine to see the repaint indicator behind other content, or used temporarily during development.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| label | String | no | Identifier for Perfetto profiling traces | Empty string defaults to "anonymous repaint" |

**Cross References:**
- `$API.Graphics.fillAll$`

## drawRoundedRectangle

**Signature:** `undefined drawRoundedRectangle(Array area, NotUndefined cornerData, Double borderSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.drawRoundedRectangle([10, 10, 100, 50], 5.0, 2.0);`

**Description:**
Draws a rounded rectangle outline (stroke only, not filled) within the specified area using the current colour. The `cornerData` parameter controls the corner rounding and accepts two formats:

1. **Number**: Uniform corner radius applied to all four corners (e.g., `5.0`).
2. **JSON object**: Per-corner control with properties:
   - `CornerSize` (float): The radius for rounded corners
   - `Rounded` (array of 4 booleans): `[topLeft, topRight, bottomLeft, bottomRight]` -- controls which corners are rounded

When using the JSON object format with `Rounded`:
- If all four values are `false`, the method falls back to drawing a plain rectangle (equivalent to `drawRect`)
- If some corners are rounded and others are not, a Path is constructed with `addRoundedRectangle` and stroked with `PathStrokeType(borderSize)`

The `borderSize` parameter controls the width of the outline stroke.

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if `setColour`/`setGradientFill` has not been called before drawing, or if the area is passed as separate arguments instead of an array.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` | Float precision |
| cornerData | NotUndefined | no | Corner radius (number) or per-corner config (JSON) | CornerSize SANITIZED |
| borderSize | Double | no | Width of the outline stroke in pixels | SANITIZED |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| CornerSize | Double | Radius for rounded corners in pixels |
| Rounded | Array | Array of 4 booleans: `[topLeft, topRight, bottomLeft, bottomRight]` |

**Cross References:**
- `$API.Graphics.fillRoundedRectangle$`
- `$API.Graphics.drawRect$`
- `$API.Graphics.setColour$`

**Example:**
```javascript:drawroundedrectangle-percorner
// Title: Rounded rectangle with only top corners rounded
Panel1.setPaintRoutine(function(g)
{
    g.setColour(0xFFFF6600);
    g.drawRoundedRectangle([10, 10, 180, 80], {
        "CornerSize": 12.0,
        "Rounded": [true, true, false, false]
    }, 2.0);
});
```
```json:testMetadata:drawroundedrectangle-percorner
{
  "testable": false,
  "skipReason": "Visual output only -- rounded rectangle rendering cannot be verified programmatically"
}
```

## drawSVG

**Signature:** `undefined drawSVG(ScriptObject svgObject, Array bounds, Double opacity)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, stores a reference to the SVG object.
**Minimal Example:** `{obj}.drawSVG(mySVG, [10, 10, 100, 100], 1.0);`

**Description:**
Draws an SVG object within the specified rectangular bounds at the given opacity. The `svgObject` must be a valid `SVG` scripting object (created via `Content.createSVG(base64String)`). The SVG is scaled to fit the specified bounds. The `opacity` parameter controls the overall transparency of the rendered SVG (0.0 = fully transparent, 1.0 = fully opaque).

The draw action stores a `var` reference to the SVG object and calls its `draw(g, bounds, opacity)` method during the UI-thread render pass. The SVG object handles its own internal rendering using the JUCE Drawable system.

If the `svgObject` parameter is not a valid SVG object, the script error "not a SVG object" is reported.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| svgObject | ScriptObject | no | An SVG object containing the vector graphic data | Must be a valid SVG object |
| bounds | Array | no | Target rectangle `[x, y, w, h]` to draw the SVG into | Float precision |
| opacity | Double | no | Overall opacity for the SVG rendering | 0.0 = transparent, 1.0 = opaque |

**Cross References:**
- `$API.Graphics.drawImage$`
- `$API.Graphics.drawFFTSpectrum$`
- `$API.Graphics.fillPath$`
- `$API.Graphics.drawPath$`

## drawText

**Disabled:** deprecated
**Disabled Reason:** Superseded by `drawAlignedText` which supports alignment options. Registered with `ADD_API_METHOD_2_DEPRECATED("use drawAlignedText for better placement")`.

**Cross References:**
- `$API.Graphics.drawAlignedText$`

## drawTriangle

**Signature:** `undefined drawTriangle(Array area, Double angle, Double lineThickness)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, creates a Path, applies rotation transform, and copies the Path.
**Minimal Example:** `{obj}.drawTriangle([10, 10, 50, 50], 0.0, 2.0);`

**Description:**
Draws a triangle outline (stroke only, not filled) within the specified rectangular area, rotated by the given angle in radians. The triangle is constructed as an upward-pointing isosceles triangle with vertices at (0.5, 0.0), (1.0, 1.0), (0.0, 1.0) in normalized coordinates, then rotated around its center by the `angle` parameter, and finally scaled to fit the target `area` using non-uniform scaling (`scaleToFit` with `preserveProportions=false`). The outline is stroked with the specified `lineThickness`.

Internally, the triangle is drawn as a Path with `drawPath` using a `PathStrokeType(lineThickness)`. To draw a filled triangle, use `fillTriangle` instead.

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if `setColour`/`setGradientFill` has not been called before drawing, or if the area is passed as separate arguments instead of an array.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` for the triangle | Float precision |
| angle | Double | no | Rotation angle in radians (0 = upward-pointing) | SANITIZED |
| lineThickness | Double | no | Width of the outline stroke in pixels | -- |

**Cross References:**
- `$API.Graphics.fillTriangle$`
- `$API.Graphics.drawPath$`
- `$API.Graphics.setColour$`

## drawVerticalLine

**Signature:** `undefined drawVerticalLine(Number x, Double y1, Double y2)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.drawVerticalLine(50, 10.0, 190.0);`

**Description:**
Draws a vertical line at the specified integer x-coordinate from `y1` to `y2` using the current colour. The line is drawn without sub-pixel interpolation (pixel-snapped), making it suitable for crisp single-pixel separators and grid lines. The x-coordinate is an integer for pixel-perfect alignment; the y start and end positions are floats.

The line width is always 1 pixel. For thicker vertical lines, use `drawLine` instead.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Number | no | Horizontal position of the line in pixels | Integer; no sub-pixel positioning |
| y1 | Double | no | Start y-coordinate | SANITIZED against NaN/Inf |
| y2 | Double | no | End y-coordinate | SANITIZED against NaN/Inf |

**Cross References:**
- `$API.Graphics.drawHorizontalLine$`
- `$API.Graphics.drawLine$`
- `$API.Graphics.setColour$`

## endLayer

**Signature:** `undefined endLayer()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Pops the current layer from the layer stack (ReferenceCountedArray::removeLast).
**Minimal Example:** `{obj}.endLayer();`

**Description:**
Ends the current layer, compositing it back onto the parent canvas (or onto the parent layer if layers are nested). This method pops the topmost layer from the internal layer stack. During the UI-thread render pass, the layer's internal draw actions are rendered to an offscreen image, any post-processing effects (blur, desaturate, HSL grading, etc.) applied to that layer are executed via `PostGraphicsRenderer`, and the processed result is drawn onto the parent surface.

Every `beginLayer()` or `beginBlendLayer()` call must be matched with a corresponding `endLayer()`. Calling `endLayer()` without a matching `beginLayer()` removes the last element from an empty stack (no-op with no error). Forgetting `endLayer()` causes subsequent draw calls to continue targeting the orphaned layer, and the `flush()` call at the end of the paint callback clears the layer stack without compositing.

**Parameters:**

(None)

**Pitfalls:**
- Calling `endLayer()` without a prior `beginLayer()` or `beginBlendLayer()` is a no-op -- no error is reported. The layer stack's `removeLast()` on an empty array simply does nothing.
- Forgetting to call `endLayer()` after `beginLayer()` causes all subsequent draw calls to target the orphaned layer. The layer's content is discarded when `flush()` clears the stack at the end of the paint callback.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.beginBlendLayer$`
- `$API.Graphics.gaussianBlur$`
- `$API.Graphics.boxBlur$`
- `$API.Graphics.desaturate$`
- `$API.Graphics.applyHSL$`

**DiagramRef:** graphics-layer-stack

## fillAll

**Signature:** `undefined fillAll(Colour colour)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, converts the colour var.
**Minimal Example:** `{obj}.fillAll(0xFF222222);`

**Description:**
Fills the entire component area with the specified colour. This is typically the first drawing call in a paint routine to establish the background colour. The colour is converted from the var parameter via `getCleanedObjectColour()`, which supports integer/int64 values (e.g., `0xAARRGGBB`) and hex strings (e.g., `"0xFFFF0000"`).

Unlike most other drawing methods, `fillAll` does NOT require `setColour` to be called first -- it takes the colour directly as a parameter. It also has no backend diagnostic for missing `setColour` since the colour is self-contained.

This method does NOT require an active layer -- it adds a regular draw action. When called inside a layer, it fills the layer's offscreen image rather than the main canvas.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | Fill colour in 0xAARRGGBB format | Alpha channel controls opacity |

**Cross References:**
- `$API.Graphics.fillRect$`
- `$API.Graphics.setColour$`
- `$API.Graphics.setGradientFill$`

## fillEllipse

**Signature:** `undefined fillEllipse(Array area)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.fillEllipse([10, 10, 80, 80]);`

**Description:**
Fills an ellipse inscribed in the specified rectangular area using the current colour. A square area produces a filled circle. To draw only the outline of an ellipse, use `drawEllipse` instead.

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if `setColour`/`setGradientFill` has not been called before drawing, or if the area is passed as separate arguments instead of an array.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` for the ellipse | Float precision |

**Cross References:**
- `$API.Graphics.drawEllipse$`
- `$API.Graphics.fillRect$`
- `$API.Graphics.setColour$`

## fillPath

**Signature:** `undefined fillPath(ScriptObject path, Array area)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, copies the Path, and optionally scales it to the area.
**Minimal Example:** `{obj}.fillPath(myPath, [0, 0, 200, 100]);`

**Description:**
Fills the interior of a Path object using the current colour. The `path` must be a valid `Path` scripting object. The `area` parameter is optional -- if it is an array (or ScriptRectangle), the path is scaled to fit the specified `[x, y, w, h]` rectangle using non-uniform scaling (`scaleToFit` with `preserveProportions=false`). If `area` is not an array or ScriptRectangle, the path is drawn at its original coordinates without scaling.

If the path is empty (has no bounds), the method silently returns without drawing -- no error is reported.

If the `path` parameter is not a valid Path object, the `dynamic_cast` fails and the method silently does nothing -- no error is reported.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| path | ScriptObject | no | A Path object defining the shape to fill | Must be a valid Path object |
| area | Array | no | Optional bounding rectangle `[x, y, w, h]` to scale the path into | Path drawn at original coords if not an array |

**Pitfalls:**
- If the `path` parameter is not a valid Path object, the method silently does nothing -- no error is reported.
- The path is scaled to fit the area with non-uniform scaling (`scaleToFit` with `preserveProportions=false`). The filled shape may be distorted if the path's aspect ratio does not match the area's aspect ratio.
- Unlike `drawPath`, `fillPath` always returns early when the path is empty, regardless of whether an area is specified.

**Cross References:**
- `$API.Graphics.drawPath$`
- `$API.Graphics.fillRect$`
- `$API.Graphics.fillTriangle$`
- `$API.Graphics.setColour$`

## fillRect

**Signature:** `undefined fillRect(Array area)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.fillRect([10, 10, 100, 50]);`

**Description:**
Fills a rectangle with the current colour. The area is specified as a `[x, y, width, height]` array. To draw only the outline of a rectangle, use `drawRect` instead.

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if:
- `setColour`/`setGradientFill` has not been called before drawing
- The area is passed as separate arguments instead of a single array

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` | Float precision |

**Cross References:**
- `$API.Graphics.drawRect$`
- `$API.Graphics.fillRoundedRectangle$`
- `$API.Graphics.fillAll$`
- `$API.Graphics.setColour$`

## fillRoundedRectangle

**Signature:** `undefined fillRoundedRectangle(Array area, NotUndefined cornerData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.fillRoundedRectangle([10, 10, 100, 50], 5.0);`

**Description:**
Fills a rounded rectangle using the current colour. The `cornerData` parameter controls the corner rounding and accepts two formats:

1. **Number**: Uniform corner radius applied to all four corners (e.g., `5.0`).
2. **JSON object**: Per-corner control with properties:
   - `CornerSize` (float): The radius for rounded corners
   - `Rounded` (array of 4 booleans): `[topLeft, topRight, bottomLeft, bottomRight]` -- controls which corners are rounded

When using the JSON object format with `Rounded`:
- If all four values are `false`, the method falls back to filling a plain rectangle (equivalent to `fillRect`)
- If some corners are rounded and others are not, a Path is constructed with `addRoundedRectangle` using per-corner booleans

The area is specified as a `[x, y, width, height]` array. The corner size is SANITIZED against NaN/Inf values.

This method has the `CHECK_AREA_AND_COLOUR` diagnostic in backend builds, which warns at parse time if `setColour`/`setGradientFill` has not been called before drawing, or if the area is passed as separate arguments instead of an array.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` | Float precision |
| cornerData | NotUndefined | no | Corner radius (number) or per-corner config (JSON) | CornerSize SANITIZED |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| CornerSize | Double | Radius for rounded corners in pixels |
| Rounded | Array | Array of 4 booleans: `[topLeft, topRight, bottomLeft, bottomRight]` |

**Cross References:**
- `$API.Graphics.drawRoundedRectangle$`
- `$API.Graphics.fillRect$`
- `$API.Graphics.setColour$`

**Example:**
```javascript:fillroundedrectangle-percorner
// Title: Filled rounded rectangle with selective corners
Panel1.setPaintRoutine(function(g)
{
    g.setColour(0xFF3366CC);
    g.fillRoundedRectangle([10, 10, 180, 80], {
        "CornerSize": 12.0,
        "Rounded": [true, true, false, false]
    });
});
```
```json:testMetadata:fillroundedrectangle-percorner
{
  "testable": false,
  "skipReason": "Visual output only -- filled rounded rectangle rendering cannot be verified programmatically"
}
```

## fillTriangle

**Signature:** `undefined fillTriangle(Array area, Double angle)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, creates a Path, applies rotation transform, and copies the Path.
**Minimal Example:** `{obj}.fillTriangle([10, 10, 50, 50], 0.0);`

**Description:**
Fills a triangle within the specified rectangular area, rotated by the given angle in radians. The triangle is constructed as an upward-pointing isosceles triangle with vertices at (0.5, 0.0), (1.0, 1.0), (0.0, 1.0) in normalized coordinates, then rotated around its center by the `angle` parameter, and finally scaled to fit the target `area` using non-uniform scaling (`scaleToFit` with `preserveProportions=false`).

Internally, the triangle is drawn as a filled Path via `fillPath`. To draw only the triangle outline, use `drawTriangle` instead.

This method does NOT have the `CHECK_AREA_AND_COLOUR` diagnostic (unlike `drawTriangle` which does). However, `setColour` or `setGradientFill` should still be called first to define the fill colour.

This method does NOT require an active layer -- it adds a regular draw action.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, w, h]` for the triangle | Float precision |
| angle | Double | no | Rotation angle in radians (0 = upward-pointing) | -- |

**Cross References:**
- `$API.Graphics.drawTriangle$`
- `$API.Graphics.fillPath$`
- `$API.Graphics.setColour$`

## flip

**Signature:** `undefined flip(Integer horizontally, Array totalArea)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action containing an AffineTransform.
**Minimal Example:** `{obj}.flip(true, [0, 0, 200, 100]);`

**Description:**
Applies a mirror transform to the canvas. When `horizontally` is `true`, the canvas is flipped left-to-right around the vertical center of `totalArea`. When `false`, the canvas is flipped top-to-bottom around the horizontal center of `totalArea`. All subsequent drawing operations are affected by this transform until the paint callback ends or another transform overrides it.

The flip is implemented as an `AffineTransform`:
- **Horizontal flip:** Scale x by -1, translate by the area width (mirrors around the area's center)
- **Vertical flip:** Scale y by -1, translate by the area height (mirrors around the area's center)

The `totalArea` parameter is converted to an integer rectangle and defines the axis of reflection. Typically this should be the full component bounds so that the flip mirrors around the component's center.

This method does NOT require an active layer -- it adds a regular draw action (an `addTransform` action).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| horizontally | Integer | no | If true, flip left-to-right; if false, flip top-to-bottom | Boolean value |
| totalArea | Array | no | Reference rectangle `[x, y, w, h]` defining the mirror axis | Converted to integer rectangle |

**Pitfalls:**
- The flip transform is cumulative with any existing transforms (e.g., `rotate`). Calling `flip(true, ...)` followed by `flip(true, ...)` restores the original orientation.
- The `totalArea` is converted to integer coordinates. Sub-pixel precision is lost in the mirror axis position.

**Cross References:**
- `$API.Graphics.rotate$`

## gaussianBlur

**Signature:** `undefined gaussianBlur(Number blurAmount)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PostActionBase object.
**Minimal Example:** `{obj}.gaussianBlur(10);`

**Description:**
Applies a gaussian blur to the current layer's pixel data. Gaussian blur produces a smooth, natural-looking blur that falls off with a bell curve distribution. The `blurAmount` parameter controls the blur kernel size -- larger values produce a stronger blur. The value is clamped to the range 0-100. This is a post-processing effect that operates on the offscreen image of an active layer via `PostGraphicsRenderer`. For a computationally cheaper but blockier alternative, use `boxBlur`.

Requires an active layer created with `beginLayer()` -- calling without one triggers the script error "You need to create a layer for gaussian blur".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| blurAmount | Number | no | Blur radius in pixels | Clamped to 0-100; cast to int |

**Pitfalls:**
- Requires an active layer. Must be called between `beginLayer()` and `endLayer()`.

**Cross References:**
- `$API.Graphics.beginLayer$`
- `$API.Graphics.endLayer$`
- `$API.Graphics.boxBlur$`

## getStringWidth

**Signature:** `Double getStringWidth(String text)`
**Return Type:** `Double`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var w = {obj}.getStringWidth("Hello World");`

**Description:**
Returns the pixel width of the given text string when rendered with the current font. The calculation uses `MainController::getStringWidthFromEmbeddedFont()`, which measures the text using the font name, size, and kerning factor stored locally in the Graphics object (set via `setFont` or `setFontWithSpacing`). This allows accurate text width measurement without requiring the deferred JUCE Graphics context.

If `setFont` or `setFontWithSpacing` has not been called, the width is calculated using the default font (13.0px height, no kerning, empty font name).

This is the only Graphics method that returns a value instead of being a void draw command.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | The text string to measure | -- |

**Cross References:**
- `$API.Graphics.setFont$`
- `$API.Graphics.setFontWithSpacing$`
- `$API.Graphics.drawAlignedText$`

## rotate

**Signature:** `undefined rotate(Double angleInRadian, Array center)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action containing an AffineTransform.
**Minimal Example:** `{obj}.rotate(0.5, [100, 50]);`

**Description:**
Applies a rotation transform to the canvas around the specified center point. All subsequent drawing operations are rotated by `angleInRadian` radians around the point defined by `center` (a 2-element `[x, y]` array). The rotation is cumulative with any existing transforms.

The rotation is implemented as an `AffineTransform::rotation(angle, centerX, centerY)` and added as an `addTransform` draw action. The angle is SANITIZED against NaN/Inf values. The center point is parsed via `getPointFromVar()`.

This method does NOT require an active layer -- it adds a regular draw action. The transform persists for all subsequent draw calls within the same paint callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| angleInRadian | Double | no | Rotation angle in radians (positive = clockwise) | SANITIZED |
| center | Array | no | Center of rotation as `[x, y]` | 2-element array |

**Cross References:**
- `$API.Graphics.flip$`

## setColour

**Signature:** `undefined setColour(Colour colour)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action and converts the colour var.
**Minimal Example:** `{obj}.setColour(0xFFFF0000);`

**Description:**
Sets the current drawing colour for subsequent shape and text drawing operations. This colour is used by `fillRect`, `drawRect`, `fillRoundedRectangle`, `drawRoundedRectangle`, `fillEllipse`, `drawEllipse`, `fillPath`, `drawPath`, `drawLine`, `drawAlignedText`, `drawMultiLineText`, `fillTriangle`, `drawTriangle`, and all other methods that draw with the "current colour". The colour persists until the next `setColour` or `setGradientFill` call.

The colour parameter is converted via `getCleanedObjectColour()` which supports:
- Integer/int64 literals: `0xFFFF0000` (opaque red)
- Hex strings: `"0xFFFF0000"`
- Large integer strings parsed via `getLargeIntValue()`

All colours use the ARGB format `0xAARRGGBB`. Omitting the alpha channel (e.g., `0xFF0000`) produces a nearly-transparent colour since the value `0x00FF0000` has alpha = 0.

The backend diagnostic system (`CHECK_AREA_AND_COLOUR`) warns at parse time when drawing methods are called without a prior `setColour` or `setGradientFill`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | Drawing colour in 0xAARRGGBB format | Alpha channel controls opacity |

**Cross References:**
- `$API.Graphics.setGradientFill$`
- `$API.Graphics.setOpacity$`
- `$API.Graphics.fillAll$`

## setFont

**Signature:** `undefined setFont(String fontName, Double fontSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, resolves font via MainController (may access font cache with string lookups).
**Minimal Example:** `{obj}.setFont("Arial", 16.0);`

**Description:**
Sets the current font for subsequent text drawing operations (`drawAlignedText`, `drawAlignedTextShadow`, `drawMultiLineText`, `drawMarkdownText`). The font also affects `getStringWidth()` measurements. The font persists until the next `setFont` or `setFontWithSpacing` call.

The `fontName` parameter is resolved via `MainController::getFontFromString()` which supports:
- **Embedded fonts**: Custom fonts loaded via HISE project settings
- **System fonts**: Fonts installed on the user's system
- **Global HISE font**: The default font used across the HISE interface

The `fontSize` parameter sets the font height in pixels and is SANITIZED against NaN/Inf values.

Internally, the method stores the font name, height, and kerning factor (set to 0.0) in local member variables for use by `getStringWidth()`, and creates a draw action that sets the font on the deferred graphics context. The kerning factor is reset to 0.0 -- to set a custom kerning factor, use `setFontWithSpacing` instead.

The backend diagnostic system (`CHECK_FONT_AND_COLOUR`) warns at parse time when text drawing methods are called without a prior `setFont` or `setFontWithSpacing`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fontName | String | no | Name of the font to use | Must be an embedded, system, or global font |
| fontSize | Double | no | Font height in pixels | SANITIZED; 0.0 produces invisible text |

**Cross References:**
- `$API.Graphics.setFontWithSpacing$`
- `$API.Graphics.getStringWidth$`
- `$API.Graphics.drawAlignedText$`
- `$API.Graphics.drawMultiLineText$`

## setFontWithSpacing

**Signature:** `undefined setFontWithSpacing(String fontName, Double fontSize, Double spacing)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action, resolves font via MainController (may access font cache with string lookups).
**Minimal Example:** `{obj}.setFontWithSpacing("Arial", 16.0, 0.05);`

**Description:**
Sets the current font for subsequent text drawing operations, with an additional kerning (character spacing) factor. Behaves identically to `setFont` except that it applies the `spacing` value as an extra kerning factor via JUCE's `Font::setExtraKerningFactor()`. A spacing of `0.0` produces the same result as `setFont`. Positive values increase spacing between characters; negative values tighten spacing.

The font also affects `getStringWidth()` measurements -- the stored kerning factor is passed through to the width calculation so that `getStringWidth` returns values consistent with the actual rendered text spacing.

The `fontName` parameter is resolved via `MainController::getFontFromString()` which supports embedded fonts (loaded via project settings), system fonts, and the global HISE font (pass `"Default"` to use it).

The `fontSize` parameter sets the font height in pixels and is SANITIZED against NaN/Inf values.

Internally, the method stores the font name, height, and kerning factor in local member variables (`currentFontName`, `currentFontHeight`, `currentKerningFactor`) and creates a `setFont` draw action with the configured JUCE `Font` object. Calling `setFont` after `setFontWithSpacing` resets the kerning factor to `0.0`.

The backend diagnostic system (`CHECK_FONT_AND_COLOUR`) warns at parse time when text drawing methods are called without a prior `setFont` or `setFontWithSpacing`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fontName | String | no | Name of the font to use | Must be an embedded, system, or global font |
| fontSize | Double | no | Font height in pixels | SANITIZED; 0.0 produces invisible text |
| spacing | Double | no | Extra kerning factor added between characters | Typically small values like -0.1 to 0.2; 0.0 is no extra spacing |

**Cross References:**
- `$API.Graphics.setFont$`
- `$API.Graphics.getStringWidth$`
- `$API.Graphics.drawAlignedText$`
- `$API.Graphics.drawMultiLineText$`

## setGradientFill

**Signature:** `undefined setGradientFill(Array gradientData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action and constructs a JUCE ColourGradient object.
**Minimal Example:** `{obj}.setGradientFill([0xFFFF0000, 0, 0, 0xFF0000FF, 200, 0, false]);`

**Description:**
Sets the current fill to a colour gradient for subsequent drawing operations. Replaces the solid colour set by `setColour`. The gradient remains active until the next `setColour` or `setGradientFill` call.

The `gradientData` parameter must be an Array with one of three formats:

1. **Linear gradient (6 elements):** `[Colour1, x1, y1, Colour2, x2, y2]` -- creates a linear gradient from point (x1, y1) to point (x2, y2).

2. **Linear/radial gradient (7 elements):** `[Colour1, x1, y1, Colour2, x2, y2, isRadial]` -- same as above, but the 7th element controls whether the gradient is radial (`true`) or linear (`false`).

3. **Multi-stop gradient (7+ elements):** `[Colour1, x1, y1, Colour2, x2, y2, isRadial, StopColour1, position1, StopColour2, position2, ...]` -- after the 7th element, additional colour stops are added in pairs of `[colour, position]` where position is a `0.0`-`1.0` value along the gradient line. Colour1 implicitly sits at position 0.0 and Colour2 at position 1.0.

All colour values support the standard HISE colour format: integer `0xAARRGGBB` literals, hex strings `"0xFF..."`, or `Colours.xxx` constants.

If `gradientData` is not an Array, `reportScriptError("Gradient Data is not sufficient")` is thrown. If the array has fewer than 6 elements, the method silently does nothing -- no gradient is set and no error is reported.

The backend diagnostic system (`checkColourSet`) treats `setGradientFill` equivalently to `setColour` -- calling either one satisfies the colour-set requirement for subsequent drawing methods.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| gradientData | Array | no | Gradient definition array | Minimum 6 elements; see format above |

**Pitfalls:**
- [BUG] When using multi-stop gradients (more than 7 elements), the additional stops must come in pairs `[colour, position]`. An odd number of trailing elements causes the last colour to be read without a corresponding position, producing an out-of-bounds array access.
- [BUG] Passing an Array with fewer than 6 elements silently does nothing -- no gradient is set and no error is reported. Only non-Array input triggers a script error.

**Cross References:**
- `$API.Graphics.setColour$`
- `$API.Graphics.fillRect$`
- `$API.Graphics.fillRoundedRectangle$`
- `$API.Graphics.fillPath$`

**Example:**
```javascript:multi-stop-gradient
// Title: Multi-stop gradient with three colour stops
Panel1.setPaintRoutine(function(g)
{
    g.setGradientFill([0xFFFF0000, 0, 0,
                       0xFF0000FF, 0, 200,
                       false,
                       0xFFFFFF00, 0.5]);
    g.fillRect([0, 0, 200, 200]);
});
```
```json:testMetadata:multi-stop-gradient
{
  "testable": false,
  "skipReason": "Paint routine requires visual rendering context, cannot verify gradient output programmatically"
}
```

## setOpacity

**Signature:** `undefined setOpacity(Double alphaValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new draw action.
**Minimal Example:** `{obj}.setOpacity(0.5);`

**Description:**
Sets a global transparency level for all subsequent drawing operations. The `alphaValue` is a float from `0.0` (fully transparent) to `1.0` (fully opaque). This affects everything drawn after the call -- shapes, text, images, and paths -- until the next `setOpacity` call or until `setColour`/`setGradientFill` implicitly resets the opacity via the colour's own alpha channel.

Internally, this creates a draw action that calls JUCE's `Graphics::setOpacity()`, which multiplies with the current colour's alpha. Unlike `setColour` (which sets a complete colour including alpha), `setOpacity` modifies only the transparency independently of the current fill colour or gradient.

Note that the `alphaValue` parameter is NOT wrapped in `SANITIZED()` -- it is passed directly to the draw action. Values outside the 0.0-1.0 range are clamped by JUCE internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| alphaValue | Double | no | Global opacity level | 0.0 (transparent) to 1.0 (opaque) |

**Cross References:**
- `$API.Graphics.setColour$`
- `$API.Graphics.setGradientFill$`
- `$API.Graphics.beginBlendLayer$`
