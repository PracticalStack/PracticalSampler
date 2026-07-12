## drawDropShadowFromPath

**Examples:**

```javascript:drawdropshadowfrompath-3d-knob
// Title: 3D knob effect with multiple path-based drop shadows
// Context: Layering drop shadows at different radii and offsets creates
// a convincing 3D effect on a flat-drawn knob. The shadows use elliptical
// paths to follow the circular knob shape.

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    var w = obj.area[2];
    var knobArea = [w * 0.2, w * 0.2, w * 0.6, w * 0.6];

    var shadowPath = Content.createPath();
    shadowPath.addEllipse([0, 0, 1.0, 1.0]);

    // Outer shadow: soft, offset downward for depth
    g.drawDropShadowFromPath(shadowPath, knobArea, 0xBB000000, parseInt(w * 0.2), [0, parseInt(w * 0.1)]);

    // Inner shadow: tighter, subtle
    var innerArea = [w * 0.28, w * 0.28, w * 0.44, w * 0.44];
    g.drawDropShadowFromPath(shadowPath, innerArea, 0x55000000, 3, [0, 4]);

    // Knob surface
    g.setColour(0xFF666666);
    g.fillEllipse(knobArea);
});
```
```json:testMetadata:drawdropshadowfrompath-3d-knob
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```
