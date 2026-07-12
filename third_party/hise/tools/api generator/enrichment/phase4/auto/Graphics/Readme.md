<!-- Diagram triage:
  - graphics-deferred-rendering: CUT (linear pipeline A->B->C across two threads; the concept is adequately conveyed in prose without a visual)
  - graphics-layer-stack: CUT (stack push/pop with post-processing is a standard pattern; prose describes it clearly)
-->

# Graphics

Graphics is the 2D drawing context passed as `g` to `ScriptPanel.setPaintRoutine()` callbacks and `ScriptLookAndFeel` registered drawing functions. It provides all the drawing operations needed to render custom UI elements - shapes, text, images, paths, gradients, and post-processing effects.

A typical paint callback sets a colour or gradient, then draws or fills shapes into rectangular areas. Use `Rectangle` objects for layout calculations - they provide layout slicing, padding, and transformation methods that eliminate manual coordinate arithmetic:

```javascript
Panel1.setPaintRoutine(function(g)
{
    var rect = Rectangle(this.getLocalBounds(0));
    g.fillAll(0xFF222222);

    var header = rect.removeFromTop(30);
    g.setColour(0xFFFFFFFF);
    g.setFont("Arial", 16.0);
    g.drawAlignedText("Hello", header.reduced(10, 0), "left");

    g.setColour(0x20FFFFFF);
    g.fillRoundedRectangle(rect.reduced(5), 3.0);
});
```

Drawing operations fall into three tiers of complexity:

1. **Basic rendering** - `fillAll`, `setColour`, `fillRect`, `fillRoundedRectangle`, `setFont`/`setFontWithSpacing`, `drawAlignedText`. Sufficient for simple button backgrounds and text labels.
2. **Path-based controls** - adds `fillPath`, `drawPath` (with stroke JSON), `fillEllipse`, `drawDropShadow`, `fillTriangle`, `setGradientFill`. Required for arc-based rotary knobs, icon buttons, and slider tracks with gradients.
3. **Compositing and effects** - adds `beginLayer`/`endLayer`, `addDropShadowFromAlpha`, `gaussianBlur`/`boxBlur`, `drawImage`, `rotate`. Used for glowing elements, blurred backgrounds, and filmstrip rendering.

All area parameters accept both `[x, y, width, height]` arrays and `Rectangle` objects. Use `Rectangle` to compute areas - wrap `obj.area` or `this.getLocalBounds(0)` at the top of a paint callback, then use `removeFromTop`, `removeFromLeft`, `reduced`, `withSizeKeepingCentre`, and other layout methods to divide the area into regions. Colours use `0xAARRGGBB` format (always include the alpha channel - `0xFF` for fully opaque). Most drawing methods require `setColour` or `setGradientFill` to be called first. Post-processing effects (`gaussianBlur`, `boxBlur`, `desaturate`, `applyHSL`, `applyMask`, and others) require an active layer created with `beginLayer()` and closed with `endLayer()`.

> [!Tip:Drawing is deferred, no pixel readback] Graphics objects are never created by user code. They arrive as the first argument to paint callbacks and LAF drawing functions. All drawing is deferred - method calls record draw actions on the scripting thread, which are then replayed on the UI thread. This means you cannot read back pixel data or query the result of a draw call.

## Common Mistakes

- **Pass area as single array argument**
  **Wrong:** `g.fillRect(10, 10, 100, 50)`
  **Right:** `g.fillRect([10, 10, 100, 50])`
  *Area must be a single `[x, y, w, h]` array, not four separate arguments.*

- **Set colour before drawing**
  **Wrong:** `g.fillRect([10, 10, 100, 50])` without calling `setColour` first
  **Right:** `g.setColour(0xFFFF0000); g.fillRect([10, 10, 100, 50])`
  *Drawing methods require `setColour` or `setGradientFill` to be called first, otherwise the colour is undefined.*

- **Post-processing requires beginLayer**
  **Wrong:** `g.gaussianBlur(5)` without a layer
  **Right:** `g.beginLayer(false); ...; g.gaussianBlur(5); g.endLayer()`
  *Post-processing effects require an active layer created with `beginLayer`.*

- **Use British spelling centred**
  **Wrong:** `g.drawAlignedText("Hi", [0, 0, 100, 30], "center")`
  **Right:** `g.drawAlignedText("Hi", [0, 0, 100, 30], "centred")`
  *Alignment uses British spelling `"centred"`, not American `"center"`.*

- **drawLine order is x1, x2, y1, y2**
  **Wrong:** `g.drawLine(x1, y1, x2, y2, thickness)`
  **Right:** `g.drawLine(x1, x2, y1, y2, thickness)`
  *The parameter order groups x-coordinates before y-coordinates, which is unusual. Getting this wrong produces diagonal lines where straight lines were intended.*

- **Use Rectangle methods for layout math**
  **Wrong:** `var textArea = [area[0] + 10, area[1], area[2] - 20, area[3]];`
  **Right:** `var textArea = Rectangle(area).reduced(10, 0);`
  *Use `Rectangle` methods (`reduced`, `removeFromTop`, `withSizeKeepingCentre`, etc.) for layout calculations instead of manual array index arithmetic. Rectangle objects are accepted by all Graphics area parameters.*
