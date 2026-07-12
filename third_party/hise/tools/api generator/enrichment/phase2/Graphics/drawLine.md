## drawLine

**Examples:**

```javascript:drawline-grid-lines
// Title: Drawing grid lines with the unusual parameter order
// Context: drawLine groups x-coordinates then y-coordinates:
// (x1, x2, y1, y2, thickness). This example draws horizontal and
// vertical reference lines for a display panel.

const var displayPanel = Content.addPanel("displayPanel", 0, 0);
displayPanel.set("width", 200);
displayPanel.set("height", 100);

displayPanel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    g.setColour(0x33FFFFFF);

    // Horizontal centre line: from x=0 to x=width, at y=50
    // Parameter order: x1, x2, y1, y2, thickness
    g.drawLine(0.0, this.getWidth(), 50, 50, 1.0);

    // Vertical centre line: from x=100 to x=100 (same x), y=0 to y=height
    g.drawLine(100.0, 100.0, 0.0, this.getHeight(), 1.0);
});
```
```json:testMetadata:drawline-grid-lines
{
  "testable": false,
  "skipReason": "Paint routine requires visual rendering context"
}
```
