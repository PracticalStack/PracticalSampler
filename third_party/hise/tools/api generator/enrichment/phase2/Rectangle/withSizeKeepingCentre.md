## withSizeKeepingCentre

**Examples:**

```javascript:laf-centered-icon
// Title: Centering icons and indicators inside LAF-allocated areas
// Context: The most common use of withSizeKeepingCentre is positioning fixed-size
// elements (icons, LEDs, knob handles) at the center of a larger area. This
// avoids manual center-offset arithmetic.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    g.setColour(obj.over ? 0xFF555555 : 0xFF333333);
    g.fillRoundedRectangle(obj.area, 3.0);

    // Center a fixed-size icon area inside the button bounds
    var iconArea = Rectangle(obj.area).withSizeKeepingCentre(12, 12);

    g.setColour(obj.value ? 0xFFFFFFFF : 0xFF888888);
    g.fillPath(iconPath, iconArea);
});
```
```json:testMetadata:laf-centered-icon
{
  "testable": false,
  "skipReason": "LAF callback requires rendering context, cannot be tested standalone"
}
```

```javascript:laf-slider-centered-track
// Title: Creating a centered knob track from a slider area
// Context: Drawing a horizontal slider track centered vertically within obj.area,
// then creating a centered thumb handle at the current position.

const var sliderLaf = Content.createLocalLookAndFeel();

sliderLaf.registerFunction("drawLinearSlider", function(g, obj)
{
    var area = Rectangle(obj.area);
    var trackWidth = 2;

    // Center a thin horizontal track within the full slider area
    var track = area.withSizeKeepingCentre(area.width - 10, trackWidth);

    g.setColour(0xFF444444);
    g.fillRoundedRectangle(track, trackWidth / 2);

    // Position a thumb circle at the current value along the track
    var xPos = track.x + obj.valueNormalized * track.width;
    var yPos = track.y + track.height / 2;
    var thumbSize = 14;

    // Create a zero-size rectangle at the thumb position, then expand it symmetrically
    var thumb = Rectangle(xPos, yPos, 0, 0).withSizeKeepingCentre(thumbSize, thumbSize);

    g.setColour(0xFFDDDDDD);
    g.fillEllipse(thumb);
});
```
```json:testMetadata:laf-slider-centered-track
{
  "testable": false,
  "skipReason": "LAF callback requires rendering context, cannot be tested standalone"
}
```
