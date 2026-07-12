## setFontWithSpacing

**Examples:**

```javascript:setfontwithspacing-laf-text
// Title: Font with letter spacing for compact button labels
// Context: In LAF draw callbacks, setFontWithSpacing is preferred over
// setFont because the spacing parameter controls letter-spacing, which
// is important for small uppercase labels that would otherwise appear
// cramped or too loose.

const var buttonLaf = Content.createLocalLookAndFeel();

buttonLaf.registerFunction("drawToggleButton", function(g, obj)
{
    g.setColour(obj.value ? 0xFF4B4D51 : 0xFF313234);
    g.fillRoundedRectangle(obj.area, 2.0);

    // Slightly positive spacing (0.03-0.07) improves readability
    // of small uppercase text
    g.setFontWithSpacing("medium", 12.0, 0.03);
    g.setColour(obj.value ? 0xFFCCCCCC : 0xFF777777);
    g.drawAlignedText(obj.text, obj.area, "centred");
});
```
```json:testMetadata:setfontwithspacing-laf-text
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```
