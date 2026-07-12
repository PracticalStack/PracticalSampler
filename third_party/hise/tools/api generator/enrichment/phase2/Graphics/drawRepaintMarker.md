## drawRepaintMarker

**Examples:**

```javascript:drawrepaintmarker-debug
// Title: Using drawRepaintMarker to diagnose excessive repaints
// Context: Add drawRepaintMarker at the end of a paint routine during
// development to see how often the panel repaints. Rapid colour changes
// indicate the panel is repainting too frequently. Use a conditional
// flag so the marker can be toggled without removing code.

const var DEBUG_REPAINT = true;

const var bgPanel = Content.addPanel("bgPanel", 0, 0);
bgPanel.set("width", 200);
bgPanel.set("height", 100);

bgPanel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF333333);
    g.setColour(0xFFCCCCCC);
    g.setFont("regular", 14.0);
    g.drawAlignedText("Content", [10, 10, 180, 30], "left");

    // Shows a random colour overlay on each repaint
    if (DEBUG_REPAINT)
        g.drawRepaintMarker(this.get("id"));
});
```
```json:testMetadata:drawrepaintmarker-debug
{
  "testable": false,
  "skipReason": "Visual debug output -- random colour cannot be verified programmatically"
}
```
