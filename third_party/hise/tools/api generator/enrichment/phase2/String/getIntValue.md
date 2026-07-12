## getIntValue

**Examples:**

```javascript:parse-slider-value-text
// Title: Parse slider value text for custom display formatting in a LAF
// Context: In a Look and Feel drawRotarySlider callback, the value text
// contains a unit suffix (e.g., "450 ms"). Parse the numeric part with
// getIntValue to apply custom formatting like converting ms to seconds.

// Inside a LAF drawRotarySlider function:
var textToShow = obj.valueAsText;

if (textToShow == "-100 dB")
{
    textToShow = "OFF";
}
else if (textToShow.contains("ms"))
{
    local ms = textToShow.getIntValue();
    
    if (ms > 1000)
        textToShow = Engine.doubleToString(ms / 1000, 1) + " s";
    else
        textToShow = ms + " ms";
}

g.drawAlignedText(textToShow, textArea, "centred");
```
```json:testMetadata:parse-slider-value-text
{
  "testable": false,
  "skipReason": "Fragment of a LAF drawRotarySlider callback; requires obj, g, and textArea from the paint context"
}
```
