## removeFromRight

**Examples:**

```javascript:laf-combobox-icon-right
// Title: LAF layout with icon on the right and text on the left
// Context: Drawing a list row where the right side has a fixed-width icon area
// and the remaining left area is used for text. A common pattern in custom
// combo box, preset browser, and table cell LAF callbacks.

const var laf = Content.createLocalLookAndFeel();
const var arrowPath = Content.createPath();
arrowPath.loadFromData("...");  // triangle/arrow icon data

laf.registerFunction("drawComboBox", function(g, obj)
{
    g.setColour(0xFF333333);
    g.fillRoundedRectangle(obj.area, 3.0);

    var area = Rectangle(obj.area);

    // Slice the right side for the dropdown arrow
    var iconArea = area.removeFromRight(obj.area[3]);
    iconArea = iconArea.withSizeKeepingCentre(8, 6);

    g.setColour(0xFFCCCCCC);
    g.fillPath(arrowPath, iconArea);

    // 'area' now contains only the text region
    area.removeFromLeft(8); // left padding
    g.setFont("Oxygen", 14);
    g.drawAlignedText(obj.text, area, "left");
});
```
```json:testMetadata:laf-combobox-icon-right
{
  "testable": false,
  "skipReason": "LAF callback requires rendering context, cannot be tested standalone"
}
```
