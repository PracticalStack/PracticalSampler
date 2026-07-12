## withMultipliedSaturation

**Examples:**

```javascript:value-saturation
// Title: Value-proportional saturation for a knob arc
// Context: Desaturating a knob's colour when its value is low and
// fully saturating it at maximum. This creates a visual link between
// the parameter value and the colour intensity. The factor is the
// normalized knob value (0.0-1.0), clamped to a minimum so the
// colour never goes fully grey.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    // Clamp to 0.2 minimum so the arc is always slightly coloured
    var c = Colours.withMultipliedSaturation(obj.itemColour1, Math.max(0.2, obj.valueNormalized));
    g.setColour(c);
    g.drawEllipse(Rect.reduced(obj.area, 4.0), 2.0);
});
```
```json:testMetadata:value-saturation
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI render context"
}
```
