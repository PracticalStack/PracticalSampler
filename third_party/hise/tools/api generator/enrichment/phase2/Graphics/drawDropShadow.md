## drawDropShadow

**Examples:**

```javascript:drawdropshadow-slider-thumb
// Title: Drop shadow beneath a slider thumb for depth
// Context: A small drop shadow behind a slider handle creates a sense of
// elevation. The shadow is drawn first, then the handle shape on top.

const var sliderLaf = Content.createLocalLookAndFeel();

sliderLaf.registerFunction("drawLinearSlider", function(g, obj)
{
    var trackY = obj.area[3] / 2;
    var trackWidth = obj.area[2] - 14;

    // Track background
    g.setColour(0x59000000);
    g.fillRoundedRectangle([7, trackY - 1, trackWidth, 2], 1);

    // Filled portion of the track
    var thumbX = obj.valueNormalized * trackWidth + 7;
    g.setColour(obj.itemColour1);
    g.fillRoundedRectangle([7, trackY - 1, thumbX - 7, 2], 1);

    // Shadow behind the thumb (drawn before the thumb itself)
    g.drawDropShadow([thumbX - 3, trackY - 2, 0, 0], 0x66000000, 5);

    // Thumb circle
    g.setColour(Colours.mix(obj.itemColour1, Colours.white, obj.hover * 0.15));
    g.fillEllipse([thumbX - 4, trackY - 4, 8, 8]);
});
```
```json:testMetadata:drawdropshadow-slider-thumb
{
  "testable": false,
  "skipReason": "LAF draw callback requires visual rendering context"
}
```
