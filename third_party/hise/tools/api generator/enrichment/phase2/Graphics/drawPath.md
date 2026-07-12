## drawPath

**Examples:**

```javascript:drawpath-rotary-knob-arc
// Title: Arc-based rotary knob with background track and value arc
// Context: The standard pattern for drawing a custom rotary slider in a LAF
// callback. Two paths are needed: a background arc showing the full range,
// and a value arc showing the current position.

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    var bgPath = Content.createPath();
    var valuePath = Content.createPath();

    // Set explicit bounds so the path scales correctly to the area
    bgPath.startNewSubPath(0.0, 0.0);
    bgPath.startNewSubPath(1.0, 1.0);
    valuePath.startNewSubPath(0.0, 0.0);
    valuePath.startNewSubPath(1.0, 1.0);

    var arcRange = 2.4;
    var pathArea = [4, 4, obj.area[2] - 8, obj.area[3] - 8];

    // Background arc (full range)
    bgPath.addArc([0, 0, 1, 1], -arcRange, arcRange);
    g.setColour(0x59000000);
    g.drawPath(bgPath, pathArea, 2.0);

    // Value arc (current position)
    var endAngle = -arcRange + 2.0 * arcRange * obj.valueNormalized;
    valuePath.addArc([0, 0, 1, 1], -arcRange, endAngle);

    g.setColour(obj.itemColour1);
    g.drawPath(valuePath, pathArea, 2.0);
});
```
```json:testMetadata:drawpath-rotary-knob-arc
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```
