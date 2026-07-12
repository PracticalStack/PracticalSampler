## contains

**Examples:**

```javascript:hit-test-stored-regions
// Title: Hit-testing stored regions in a mouse callback
// Context: A panel that defines clickable regions during setup, then tests
// mouse events against those regions. This pattern is used for custom popup
// panels with close buttons, icon selectors, and multi-region controls.

const var popup = Content.addPanel("PopupPanel", 0, 0);
popup.set("width", 340);
popup.set("height", 200);
popup.set("allowCallbacks", "All Callbacks");

// Pre-compute hit regions as Rectangle objects stored in panel data
popup.data.titleArea = Rectangle(38, 33, 100, 20);
popup.data.closeArea = Rectangle(296, 38, 16, 16);

popup.setMouseCallback(function(event)
{
    // Point containment: pass [x, y] to test if click is inside the region
    this.data.closeHover = this.data.closeArea.contains([event.x, event.y]);

    if (this.data.closeHover && event.clicked)
    {
        this.set("visible", false);
    }

    this.repaint();
});

popup.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    g.setColour(this.data.closeHover ? 0xFFFFFFFF : 0xFF888888);
    g.setFont("Oxygen Bold", 14);
    g.drawAlignedText("X", this.data.closeArea, "centred");

    g.setColour(0xFFCCCCCC);
    g.setFont("Oxygen Bold", 16);
    g.drawAlignedText("Filter Settings", this.data.titleArea, "left");
});
```
```json:testMetadata:hit-test-stored-regions
{
  "testable": false,
  "skipReason": "Mouse callback and paint routine require panel interaction, cannot be tested standalone"
}
```
