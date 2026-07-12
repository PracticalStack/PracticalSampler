## setValuePopupData

**Examples:**

```javascript:themed-value-popups
// Title: Configuring styled value popups to match the plugin theme
// Context: Value popups appear when dragging knobs and sliders. Configure
// them once during init inside your LookAndFeel setup file. The style
// should match the plugin's overall visual theme.

Content.makeFrontInterface(900, 600);

// Call inside your LAF/theme initialization
Content.setValuePopupData({
    "fontName": "medium",
    "fontSize": 16,
    "borderSize": 1,
    "borderRadius": 3,
    "margin": 8,
    "bgColour": 0x5552535B,
    "itemColour": 0xDD111111,
    "itemColour2": 0xDD111111,
    "textColour": 0xFFC1C5D7
});

// All knobs and sliders now show popups with this style
const var knob = Content.addKnob("Volume", 10, 10);
```
```json:testMetadata:themed-value-popups
{
  "testable": false,
  "skipReason": "Value popups are visual-only and require mouse interaction to display"
}
```

This is a global setting - call it once and it applies to every knob and slider in the interface. Place it alongside your other theme setup code (font loading, global LAF creation) for maintainability.
