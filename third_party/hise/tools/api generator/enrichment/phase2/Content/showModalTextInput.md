## showModalTextInput

**Examples:**

```javascript:inline-knob-text-edit
// Title: Inline value editing on a knob with capture variables
// Context: Double-clicking a knob opens a text input overlay positioned
// over the knob, pre-filled with its current value. The callback uses
// a capture list to retain the reference to the target component.

Content.makeFrontInterface(900, 600);

const var gainKnob = Content.addKnob("GainKnob", 10, 10);
gainKnob.set("min", -12.0);
gainKnob.set("max", 12.0);
gainKnob.set("suffix", " dB");

// Double-click opens the text editor positioned over the knob
gainKnob.setMouseCallback(function(event)
{
    if (!event.doubleClick)
        return;

    var knob = this;

    // Position the editor over the component
    var obj = {
        "text": Engine.doubleToString(knob.getValue(), 1) + " dB",
        "x": knob.getGlobalPositionX() + knob.get("width") / 2 - 30,
        "y": knob.getGlobalPositionY() + knob.get("height") / 2 - 12,
        "fontName": "medium",
        "width": 60,
        "height": 24
    };

    // Capture the component reference for use inside the callback
    Content.showModalTextInput(obj, function [knob](ok, input)
    {
        if (ok)
        {
            var newValue = parseFloat(input);
            newValue = Math.range(newValue, knob.get("min"), knob.get("max"));
            knob.setValue(newValue);
            knob.changed();
        }
    });
});
```
```json:testMetadata:inline-knob-text-edit
{
  "testable": false,
  "skipReason": "Mouse callback requires double-click interaction and modal text input requires keyboard interaction"
}
```

**Pitfalls:**
- The callback uses a closure with a capture list (`function [knob](ok, input)`) because inner anonymous functions cannot access outer function parameters. Without the capture, the component reference would be undefined inside the callback.
