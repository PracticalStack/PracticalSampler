Starts a new offscreen layer. All subsequent draw calls are recorded into this layer instead of the main canvas. Post-processing effects (`gaussianBlur`, `boxBlur`, `desaturate`, `applyHSL`, `applyGamma`, `applyGradientMap`, `applyMask`, `applySharpness`, `applySepia`, `applyVignette`) can then be applied to the layer before calling `endLayer()`, which composites the processed result back onto the parent canvas. Layers can be nested.

The `drawOnParent` parameter controls whether the layer starts blank (`false`) or captures the parent's existing content as its starting point (`true`). Pass `true` when you want post-processing effects to operate on content that was drawn before the layer began.

Use `Rectangle` to compute drawing areas within layer-based workflows - its `reduced` and layout slicing methods keep coordinate calculations readable even in complex compositing chains:

```javascript
laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var rect = Rectangle(obj.area);
    var arcArea = rect.reduced(12);

    g.beginLayer(false);
    g.setColour(Colours.withAlpha(obj.itemColour1, 0.3));
    g.drawPath(arcPath, arcArea, 2.0);
    g.addDropShadowFromAlpha(obj.itemColour1, 6);
    g.endLayer();

    g.setColour(Colours.withAlpha(obj.itemColour1, 0.7));
    g.drawPath(arcPath, arcArea, 2.0);
});
```

> [!Warning:Every beginLayer needs matching endLayer] Every `beginLayer()` must be matched with a corresponding `endLayer()`. Forgetting `endLayer()` causes all subsequent draw calls to target the orphaned layer, and its content is discarded at the end of the paint callback.
