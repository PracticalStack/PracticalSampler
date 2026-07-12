## getStringWidth

**Examples:**

```javascript:getstringwidth-strikethrough
// Title: Measuring text width to draw a strikethrough line
// Context: getStringWidth returns the pixel width of a string using the
// current font, enabling precise text decorations. This pattern measures
// the text, centres the measurement area, and draws a line through it.

const var Panel1 = Content.addPanel("Panel1", 0, 0);
Panel1.set("width", 200);
Panel1.set("height", 30);

Panel1.setPaintRoutine(function(g)
{
    var text = "Disabled";
    g.setFont("regular", 14.0);
    g.setColour(Colours.withAlpha(Colours.white, 0.5));
    g.drawAlignedText(text, [0, 0, 200, 30], "centred");

    // Measure the text width to position the strikethrough
    var textWidth = g.getStringWidth(text);
    var lineY = 15;
    var lineX = (200 - textWidth) / 2;

    g.drawLine(lineX, lineX + textWidth, lineY, lineY, 1.0);
});
```
```json:testMetadata:getstringwidth-strikethrough
{
  "testable": false,
  "skipReason": "Paint routine requires visual rendering context"
}
```
