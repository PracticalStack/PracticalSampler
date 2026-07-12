## beginLayer

**Examples:**

```javascript:beginlayer-glow-effect
// Title: Glow effect using layer + addDropShadowFromAlpha
// Context: Drawing shapes on a transparent layer, then generating a shadow
// from their alpha channel, creates a glow effect. The shadow extends
// around the drawn shapes based on their actual outlines.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var p = Content.createPath();
    p.startNewSubPath(0.0, 0.0);
    p.startNewSubPath(1.0, 1.0);
    var arcRange = 2.6;
    p.addArc([0, 0, 1, 1], -arcRange, -arcRange + obj.valueNormalized * 2.0 * arcRange);

    var arcArea = [12, 12, obj.area[2] - 24, obj.area[3] - 24];

    // Layer captures the arc drawing for glow generation
    g.beginLayer(false);

    g.setColour(Colours.withAlpha(obj.itemColour1, 0.3));
    g.drawPath(p, arcArea, 2.0);

    // Shadow is generated from the alpha of everything drawn in this layer
    g.addDropShadowFromAlpha(Colours.withBrightness(obj.itemColour1, 2.5), 6);
    g.endLayer();

    // Draw the sharp arc on top of the glow (outside the layer)
    g.setColour(Colours.withAlpha(obj.itemColour1, 0.7));
    g.drawPath(p, arcArea, 2.0);
});
```
```json:testMetadata:beginlayer-glow-effect
{
  "testable": false,
  "skipReason": "LAF draw callback with layer compositing requires visual rendering context"
}
```

```javascript:beginlayer-soft-glow
// Title: Soft glow effect using layer + boxBlur
// Context: Drawing on a layer and then applying boxBlur creates a soft,
// diffused version of the content. Drawing the sharp version on top
// produces a halo/glow effect.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var p = Content.createPath();
    p.startNewSubPath(0.0, 0.0);
    p.startNewSubPath(1.0, 1.0);
    var arcRange = 2.6;
    var endAngle = -arcRange + Math.max(0.05, obj.valueNormalized * 2.0 * arcRange);
    p.addArc([0, 0, 1, 1], -arcRange, endAngle);

    var arcArea = [12, 12, obj.area[2] - 24, obj.area[3] - 24];
    var c = obj.itemColour1;

    // Blurred glow layer
    g.beginLayer(false);

    g.setColour(Colours.withAlpha(c, obj.hover ? 0.5 : 0.3));
    g.drawPath(p, arcArea, 3.0);
    g.boxBlur(4);

    g.endLayer();

    // Sharp arc drawn on top of the glow
    g.setColour(Colours.withAlpha(c, 0.7));
    g.drawPath(p, arcArea, 2.0);
});
```
```json:testMetadata:beginlayer-soft-glow
{
  "testable": false,
  "skipReason": "LAF draw callback with layer compositing requires visual rendering context"
}
```
