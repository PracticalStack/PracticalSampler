## setGradientFill

**Examples:**

```javascript:setgradientfill-metallic-knob
// Title: Vertical gradient for a metallic knob surface
// Context: Gradients create the illusion of a 3D surface on flat-drawn
// controls. A top-to-bottom gradient from light to dark simulates
// overhead lighting on a convex surface.

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    var knobArea = [8, 8, obj.area[2] - 16, obj.area[3] - 16];

    // Metallic gradient: lighter at top, darker at bottom
    g.setGradientFill([
        0xFFAAAAA9, 0.0, 0.0,
        0xFFBBBBBB, 0.0, knobArea[3],
        false
    ]);

    g.fillEllipse(knobArea);

    // Subtle border highlight gradient (top-lit edge)
    g.setGradientFill([
        Colours.withAlpha(Colours.white, 0.08), 0.0, 0.0,
        Colours.withAlpha(Colours.black, 0.02), 0.0, knobArea[3]
    ]);

    g.drawEllipse(knobArea, 1.0);
});
```
```json:testMetadata:setgradientfill-metallic-knob
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```

```javascript:setgradientfill-radial-circle
// Title: Radial gradient for a glowing circle
// Context: A radial gradient creates a soft glow or spotlight effect.
// The 7th element controls linear (false) vs radial (true).

const var Panel1 = Content.addPanel("Panel1", 0, 0);

Panel1.setPaintRoutine(function(g)
{
    var cx = this.getWidth() / 2;
    var cy = this.getHeight() / 2;

    // Radial gradient: bright center fading to transparent edge
    g.setGradientFill([
        Colours.withAlpha(0xFFFF6600, 0.8), cx, cy,
        Colours.withAlpha(0xFFFF6600, 0.0), cx + 40, cy,
        true
    ]);

    g.fillEllipse([cx - 40, cy - 40, 80, 80]);
});
```
```json:testMetadata:setgradientfill-radial-circle
{
  "testable": false,
  "skipReason": "Paint routine requires visual rendering context"
}
```
