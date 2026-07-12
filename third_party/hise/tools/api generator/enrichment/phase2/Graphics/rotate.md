## rotate

**Examples:**

```javascript:rotate-knob-indicator
// Title: Rotary knob position indicator using rotate/un-rotate
// Context: The standard pattern for drawing a position line on a rotary
// knob. Rotate the canvas to the knob's angle, draw a line at the
// 12 o'clock position, then rotate back to restore the canvas.

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    var arcRange = 2.4;
    var knobSize = 28;
    var baseArea = [
        (obj.area[2] - knobSize) / 2,
        8,
        knobSize,
        knobSize
    ];

    // Draw the knob body
    g.setColour(0xFF5D5E63);
    g.fillEllipse(baseArea);

    // Convert normalized value to radians
    var angle = -arcRange + 2.0 * arcRange * obj.valueNormalized;
    var centre = [baseArea[0] + knobSize / 2, baseArea[1] + knobSize / 2];

    // Rotate, draw indicator at 12 o'clock, rotate back
    g.rotate(angle, centre);

    g.setColour(Colours.withAlpha(Colours.white, 0.8));
    var lineWidth = 2;
    g.fillRect([
        centre[0] - lineWidth / 2,
        baseArea[1],
        lineWidth,
        knobSize / 2 - 2
    ]);

    // Always undo the rotation to restore the canvas
    g.rotate(-angle, centre);

    // Subsequent drawing is unaffected by the rotation
    g.setColour(0x88FFFFFF);
    g.setFont("regular", 13.0);
    g.drawAlignedText(obj.text, obj.area, "centredBottom");
});
```
```json:testMetadata:rotate-knob-indicator
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```

**Pitfalls:**
- Always undo the rotation after drawing the rotated element by calling `g.rotate(-angle, centre)`. Forgetting this causes all subsequent drawing in the same callback to be rotated, producing misaligned text and shapes. This is the most common mistake with `rotate` in LAF callbacks.
