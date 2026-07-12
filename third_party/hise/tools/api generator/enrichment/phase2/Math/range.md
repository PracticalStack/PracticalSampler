## range

**Examples:**

```javascript:clamp-mouse-coordinates
// Title: Clamping mouse drag coordinates to normalised bounds
// Context: ScriptPanel mouse callbacks produce raw pixel positions
// that must be normalised and clamped before use as control values.

const var xyPad = Content.addPanel("XYPad", 0, 0);

xyPad.setMouseCallback(function(event)
{
    if (event.clicked || event.drag)
    {
        var dragArea = this.getLocalBounds(10);  // 10px margin

        // Normalise pixel position to [0, 1] and clamp
        var normX = Math.range(
            (event.x - dragArea[0]) / dragArea[2], 0.0, 1.0
        );
        var normY = Math.range(
            (event.y - dragArea[1]) / dragArea[3], 0.0, 1.0
        );

        this.setValue([normX, normY]);
        this.changed();
        this.repaint();
    }
});
```
```json:testMetadata:clamp-mouse-coordinates
{
  "testable": false,
  "skipReason": "Mouse callback requires user drag interaction to execute"
}
```

```javascript:clamp-envelope-drag
// Title: Clamping an envelope drag value
// Context: When dragging to adjust envelope parameters (attack,
// decay, sustain), the delta from the mouse gesture is added to
// the original value and clamped to [0, 1].

// Inside a mouse drag handler:
var newValue = Math.range(this.data.downValue + delta, 0.0, 1.0);
envelopeSlider.setValue(newValue);
```
```json:testMetadata:clamp-envelope-drag
{
  "testable": false,
  "skipReason": "Incomplete callback fragment requiring mouse drag context and undefined external variables (this.data, delta, envelopeSlider)"
}
```
