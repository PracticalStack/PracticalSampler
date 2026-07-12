## fillTriangle

**Examples:**

```javascript:filltriangle-dropdown-arrow
// Title: Dropdown arrow indicator on a combo box LAF
// Context: A small filled triangle rotated by PI (pointing down) serves
// as the dropdown indicator on custom combo box rendering. The angle
// parameter rotates from the default upward-pointing orientation.

const var comboLaf = Content.createLocalLookAndFeel();

comboLaf.registerFunction("drawComboBox", function(g, obj)
{
    // Draw the selected text
    g.setColour(obj.hover ? 0xFFCCCCCC : 0xFF999999);
    g.setFontWithSpacing("medium", 12.0, 0.0);
    g.drawAlignedText(obj.text.toUpperCase(), obj.area, "right");

    // Draw a downward-pointing triangle as dropdown indicator
    var arrowArea = [obj.area[2] - 14, obj.area[3] / 2 - 3, 8, 6];
    g.setColour(0x88FFFFFF);
    g.fillTriangle(arrowArea, Math.PI);
});
```
```json:testMetadata:filltriangle-dropdown-arrow
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```
