## fillRoundedRectangle

**Examples:**

```javascript:fillroundedrectangle-percorner-button
// Title: Per-corner rounding for tab-style buttons in a LAF callback
// Context: Buttons at the edges of a tab bar need rounded corners on one
// side only. The JSON cornerData format controls which corners are rounded.

const var tabBarLaf = Content.createLocalLookAndFeel();

tabBarLaf.registerFunction("drawToggleButton", function(g, obj)
{
    var isFirst = obj.text == "MAIN";
    var isLast = obj.text == "SETTINGS";

    // Rounded corners only on the outer edges of the tab bar
    var roundData = {
        "CornerSize": 4,
        "Rounded": [isFirst, isLast, isFirst, isLast]
    };

    g.setColour(obj.value ? 0xFFCCCCCC : 0xFF313234);
    g.fillRoundedRectangle(obj.area, roundData);

    g.setColour(obj.value ? 0xFF222222 : 0xFF999999);
    g.setFontWithSpacing("medium", 12.0, 0.03);
    g.drawAlignedText(obj.text, obj.area, "centred");
});
```
```json:testMetadata:fillroundedrectangle-percorner-button
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```
