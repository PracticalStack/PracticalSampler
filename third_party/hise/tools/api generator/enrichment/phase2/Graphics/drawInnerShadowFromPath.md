## drawInnerShadowFromPath

**Examples:**

```javascript:drawinnershadowfrompath-modulation-arc
// Title: Inner shadow on a modulation arc for a recessed look
// Context: drawInnerShadowFromPath adds depth to stroked paths by
// creating an inset shadow. For arcs, use createStrokedPath to convert
// the stroked outline into a filled path suitable for the shadow.

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    if (!obj.modulationActive) return;

    var arcPath = Content.createPath();
    arcPath.startNewSubPath(0.0, 0.0);
    arcPath.startNewSubPath(1.0, 1.0);
    var arcRange = 2.4;

    arcPath.addArc([0, 0, 1, 1],
        -arcRange + obj.modMinValue * 2.0 * arcRange,
        -arcRange + obj.modMaxValue * 2.0 * arcRange);

    var arcArea = [6, 6, obj.area[2] - 12, obj.area[3] - 12];
    var strokeStyle = {"Thickness": 3};

    // Draw the modulation arc
    g.setColour(Colours.withAlpha(obj.itemColour1, 0.6));
    g.drawPath(arcPath, arcArea, strokeStyle);

    // Convert stroked path to filled shape for inner shadow
    var strokedPath = arcPath.createStrokedPath(strokeStyle, "");
    g.drawInnerShadowFromPath(
        strokedPath, arcArea,
        Colours.withAlpha(obj.itemColour1, obj.clicked ? 1.0 : 0.5),
        3, [0, 0]
    );
});
```
```json:testMetadata:drawinnershadowfrompath-modulation-arc
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context and modulation state"
}
```
