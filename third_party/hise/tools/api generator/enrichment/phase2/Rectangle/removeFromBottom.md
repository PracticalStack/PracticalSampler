## removeFromBottom

**Examples:**

```javascript:laf-slider-text-at-bottom
// Title: LAF slider with separate value track and text area at the bottom
// Context: Drawing a rotary slider where the bottom portion is reserved for
// a text label, and the remaining upper area is used for the knob arc.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var area = Rectangle(obj.area);

    // Reserve the bottom 20px for the value text
    var textArea = area.removeFromBottom(20);

    // 'area' now contains only the knob region
    var knobSize = Math.min(area.width, area.height);
    var knob = area.withSizeKeepingCentre(knobSize, knobSize);

    g.setColour(0xFF444444);
    g.fillEllipse(knob.reduced(2));

    g.setColour(0xFFCCCCCC);
    g.setFont("Oxygen", 12);
    g.drawAlignedText(obj.valueAsText, textArea, "centred");
});
```
```json:testMetadata:laf-slider-text-at-bottom
{
  "testable": false,
  "skipReason": "LAF callback requires rendering context, cannot be tested standalone"
}
```
