## drawImage

**Examples:**

```javascript:drawimage-filmstrip-laf
// Title: Filmstrip knob rendering in a LAF callback with theme switching
// Context: Filmstrip images contain multiple pre-rendered frames stacked
// vertically. The yOffset selects which frame to display based on the
// knob's normalized value. Images must be loaded on the LAF object
// (not the panel) when used in LAF callbacks.

const var knobLaf = Content.createLocalLookAndFeel();

// Load images onto the LAF object for use in its draw callbacks
knobLaf.loadImage("{PROJECT_FOLDER}Knob_Default.png", "default");
knobLaf.loadImage("{PROJECT_FOLDER}Knob_Accent.png", "accent");

const var NUM_FRAMES = 128;
const var FRAME_HEIGHT = 46;

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    // Select image variant based on a component property
    var imageName = obj.bgColour == Colours.white ? "accent" : "default";
    var frameIndex = parseInt(obj.valueNormalized * (NUM_FRAMES - 1));

    g.drawImage(imageName, obj.area, 0, frameIndex * FRAME_HEIGHT);
});
```
```json:testMetadata:drawimage-filmstrip-laf
{
  "testable": false,
  "skipReason": "Requires preloaded image resources that cannot be created via API"
}
```

**Pitfalls:**
- When using `drawImage` in a LAF callback, load the image with `laf.loadImage()` on the LAF object - not on a panel. Using `panel.loadImage()` loads the image into the panel's image pool, which is not accessible from LAF draw functions. This produces the grey "XXX" placeholder.
