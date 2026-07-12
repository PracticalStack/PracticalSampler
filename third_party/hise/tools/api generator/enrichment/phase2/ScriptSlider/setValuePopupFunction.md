## setValuePopupFunction

**Examples:**

```javascript:shared-decibel-popup-formatter
// Title: Share one popup formatter across level sliders
// Context: Multiple mixer-style controls use the same display rule, including an "Off" floor label.

const var levelA = Content.addKnob("LevelA", 0, 0);
const var levelB = Content.addKnob("LevelB", 80, 0);

levelA.set("mode", "Decibel");
levelB.set("mode", "Decibel");
levelA.set("showValuePopup", "Above");
levelB.set("showValuePopup", "Above");

inline function formatDbPopup(value)
{
    if (value <= -100.0)
        return "Off";

    return Engine.doubleToString(value, 1) + " dB";
}

levelA.setValuePopupFunction(formatDbPopup);
levelB.setValuePopupFunction(formatDbPopup);
```
```json:testMetadata:shared-decibel-popup-formatter
{
  "testable": false,
  "skipReason": "Popup text is only rendered during drag interaction and cannot be asserted through script state"
}
```
