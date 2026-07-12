## removeFromTop

**Examples:**

```javascript:panel-header-content-split
// Title: Panel layout with header, content area, and embedded screens
// Context: Paint routine that slices a panel into a header bar and content region,
// then uses the header for a title and the remaining area for embedded child screens.

const var panel = Content.addPanel("MainPanel", 0, 0);
panel.set("width", 400);
panel.set("height", 300);
panel.set("text", "Oscillator");

panel.setPaintRoutine(function(g)
{
    var area = Rectangle(this.getLocalBounds(0));

    g.setColour(0xFF282828);
    g.fillRoundedRectangle(area.reduced(1.0), 3);

    // removeFromTop shrinks 'area' and returns the removed header strip
    var header = area.removeFromTop(35).reduced(2);

    g.setColour(0xFF303030);
    g.fillRoundedRectangle(header, 2.0);
    g.setColour(0xFFCCCCCC);
    g.setFont("Oswald Bold", 22.0);
    g.drawAlignedText(this.get("text"), header, "centred");

    // 'area' now holds only the remaining content region below the header
    g.setColour(0xFF111111);
    g.fillRoundedRectangle(area.reduced(4), 4.0);
});
```
```json:testMetadata:panel-header-content-split
{
  "testable": false,
  "skipReason": "Paint routine requires panel rendering, cannot be tested standalone"
}
```
