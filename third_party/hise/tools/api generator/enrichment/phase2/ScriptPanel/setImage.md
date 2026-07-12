## setImage

**Examples:**

```javascript:filmstrip-knob-rendering
// Title: Filmstrip knob rendering with value-based frame offset
// Context: A panel that displays different frames from a vertical filmstrip
// image based on its current value. The y-offset selects which frame
// to show, creating an animated knob or button appearance.

const var knobPanel = Content.addPanel("FilmstripKnob", 0, 0);
knobPanel.set("width", 50);
knobPanel.set("height", 50);

// Load a vertical filmstrip with 100 frames, each 50x50 pixels
knobPanel.loadImage("{PROJECT_FOLDER}knob_filmstrip.png", "knob");

const var NUM_FRAMES = 100;

inline function updateKnobImage(component, value)
{
    // Calculate y-offset: each frame is 50px tall
    local frameIndex = parseInt(value * (NUM_FRAMES - 1));
    local yOffset = frameIndex * component.getHeight();
    component.setImage("knob", 0, yOffset);
};

knobPanel.setControlCallback(updateKnobImage);

// Show initial frame
knobPanel.setImage("knob", 0, 0);
```
```json:testMetadata:filmstrip-knob-rendering
{
  "testable": false,
  "skipReason": "Requires knob_filmstrip.png image file in the project Images folder"
}
```

When using `setImage()`, either the x-offset or y-offset must be 0. The non-zero offset selects the frame along the strip direction (vertical strips use y-offset, horizontal strips use x-offset). The clipped region size is determined by the panel's dimensions.
