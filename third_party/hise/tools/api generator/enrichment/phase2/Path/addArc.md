## addArc

**Examples:**

```javascript:rotary-knob-arc-laf
// Title: Rotary knob arc with background track and value indicator
// Context: The most common Path usage in HISE - drawing custom rotary
// slider arcs inside a drawRotarySlider LAF callback. The background
// track shows the full arc range, and the value arc shows the current
// knob position. Bounds anchoring with startNewSubPath ensures correct
// scaling when drawPath maps the path to the component area.

const var laf = Content.createLocalLookAndFeel();
const var ARC = 2.4;  // angular half-range in radians (gap at bottom)

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var bgPath = Content.createPath();
    var valuePath = Content.createPath();

    // Anchor bounds to unit square so drawPath scales correctly
    bgPath.startNewSubPath(0.0, 0.0);
    bgPath.startNewSubPath(1.0, 1.0);
    valuePath.startNewSubPath(0.0, 0.0);
    valuePath.startNewSubPath(1.0, 1.0);

    // Background track: full arc range
    bgPath.addArc([0, 0, 1, 1], -ARC, ARC);

    // Value arc: from start to current position
    var endAngle = -ARC + 2.0 * ARC * obj.valueNormalized;
    valuePath.addArc([0, 0, 1, 1], -ARC, endAngle);

    // Draw background track
    g.setColour(0xFF333333);
    g.drawPath(bgPath, obj.area, 3.0);

    // Draw value arc
    g.setColour(Colours.withAlpha(obj.itemColour1, obj.clicked ? 1.0 : 0.8));
    g.drawPath(valuePath, obj.area, 3.0);
});
```
```json:testMetadata:rotary-knob-arc-laf
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI rendering context; cannot verify visual output programmatically"
}
```

```javascript:bipolar-knob-arc
// Title: Bipolar knob arc (center-origin)
// Context: For parameters with a center default (like pan or bipolar
// modulation), the arc starts from 12 o'clock (angle 0) and extends
// in both directions based on the value.

const var ARC2 = 2.4;

// Simulate bipolar and unipolar paths
var isBipolar = true;
var startAngle = isBipolar ? 0.0 : -ARC2;
var valueNormalized = 0.75;

var valuePath = Content.createPath();
valuePath.startNewSubPath(0.0, 0.0);
valuePath.startNewSubPath(1.0, 1.0);

var endAngle = -ARC2 + 2.0 * ARC2 * valueNormalized;
valuePath.addArc([0.0, 0.0, 1.0, 1.0], startAngle, endAngle);
```
```json:testMetadata:bipolar-knob-arc
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "valuePath.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "Math.abs(valuePath.getBounds(1.0)[2] - 1.0) < 0.01", "value": true}
  ]
}
```

**Pitfalls:**
- When using `addArc` with normalized coordinates `[0, 0, 1, 1]` for later scaling via `drawPath`, the path's natural bounding box only covers the arc segment itself, not the full unit square. Always anchor with `startNewSubPath(0, 0)` and `startNewSubPath(1, 1)` before the arc to ensure the path occupies the expected coordinate range.
