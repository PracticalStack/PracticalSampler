## fillPath

**Examples:**

```javascript:fillpath-icon-button
// Title: Drawing a vector icon in a toggle button LAF callback
// Context: Icon buttons use pre-defined Path objects (often stored in a
// namespace) that are filled into the button area. The colour changes
// based on the button's state (value, hover).

// Path data defined once at init time (outside the callback)
const var iconPath = Content.createPath();
iconPath.loadFromData("path data here...");

const var iconButtonLaf = Content.createLocalLookAndFeel();

iconButtonLaf.registerFunction("drawToggleButton", function(g, obj)
{
    // Compute icon bounds centred in the button area
    var iconArea = [
        obj.area[0] + (obj.area[2] - 14) / 2,
        obj.area[1] + (obj.area[3] - 14) / 2,
        14, 14
    ];

    var alpha = obj.value ? 0.65 : 0.25;

    if (obj.over)
        alpha += 0.1;

    g.setColour(Colours.withAlpha(Colours.white, alpha));
    g.fillPath(iconPath, iconArea);
});
```
```json:testMetadata:fillpath-icon-button
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context and path data"
}
```
