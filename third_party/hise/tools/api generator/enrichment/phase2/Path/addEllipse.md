## addEllipse

**Examples:**

```javascript:ellipse-drop-shadow
// Title: Creating a circular path for drop shadow effects
// Context: Graphics shadow methods (drawDropShadowFromPath,
// drawInnerShadowFromPath) require a Path object. When you need
// a circular shadow - for instance, behind a knob center cap -
// addEllipse creates the circle path that the shadow is derived from.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var w = obj.area[2];

    // Shadow behind the knob center
    var shadowPath = Content.createPath();
    shadowPath.addEllipse([0, 0, 1.0, 1.0]);

    g.drawDropShadowFromPath(shadowPath,
        [obj.area[0] + w * 0.22, obj.area[1] + w * 0.22,
         w * 0.56, w * 0.56],
        0xBB000000, w * 0.2, [0, w * 0.1]);

    // Knob center cap
    g.setColour(0xFF666666);
    g.fillEllipse([obj.area[0] + w * 0.25, obj.area[1] + w * 0.25,
                   w * 0.5, w * 0.5]);
});
```
```json:testMetadata:ellipse-drop-shadow
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI rendering context; cannot verify visual output programmatically"
}
```

```javascript:full-circle-record-icon
// Title: Full circle as a record button icon
// Context: addArc with a full 2*PI sweep is equivalent to addEllipse
// for creating circular paths. Both are used interchangeably in
// practice, but addEllipse is cleaner when a complete circle is needed.

const var recIcon = Content.createPath();
recIcon.addArc([0.0, 0.0, 1.0, 1.0], 0.0, Math.PI * 2.0);

// Equivalent using addEllipse:
const var recIcon2 = Content.createPath();
recIcon2.addEllipse([0.0, 0.0, 1.0, 1.0]);
```
```json:testMetadata:full-circle-record-icon
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "recIcon.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "recIcon2.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "Math.abs(recIcon.getLength() - recIcon2.getLength()) < 0.1", "value": true}
  ]
}
```
