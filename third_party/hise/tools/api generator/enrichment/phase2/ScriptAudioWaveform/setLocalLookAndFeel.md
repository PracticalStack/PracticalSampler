## setLocalLookAndFeel

**Examples:**

```javascript:full-thumbnail-laf
// Title: Complete LAF customization for an audio waveform display
// Context: When building a custom sample player, override all six thumbnail
// draw functions to fully control the waveform's appearance. The
// getThumbnailRenderOptions callback controls how the waveform data is
// processed before drawing.

const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
const var laf = Content.createLocalLookAndFeel();

// Configure rendering before any drawing happens
laf.registerFunction("getThumbnailRenderOptions", function(obj)
{
    obj.forceSymmetry = true;
    obj.manualDownSampleFactor = 1.3;
    obj.scaleVertically = false;
    obj.dynamicOptions = false;
    return obj;
});

// Clear the default background - draw nothing
laf.registerFunction("drawThumbnailBackground", function(g, obj)
{
    // Empty: the waveform sits on a transparent panel background
});

// Draw the waveform shape using the component's fill colour
laf.registerFunction("drawThumbnailPath", function(g, obj)
{
    // obj.area[1] != 0 means this is a secondary channel - skip
    // to render only the first channel for a single-layer display
    if (obj.area[1] != 0)
        return;

    g.setColour(obj.itemColour);
    g.fillPath(obj.path, obj.area);
});

// Draw the playback position as a thin white vertical line
laf.registerFunction("drawThumbnailRuler", function(g, obj)
{
    g.setColour(Colours.white);
    g.fillRect([obj.xPosition, 0, 1, obj.area[3]]);
});

// Suppress default range overlay drawing
laf.registerFunction("drawThumbnailRange", function(g, obj)
{
    // Empty: range selection not used in this display
});

// Suppress default filename text overlay
laf.registerFunction("drawThumbnailText", function(g, obj)
{
    // Empty: filename display handled elsewhere
});

wf.setLocalLookAndFeel(laf);
```

```json:testMetadata:full-thumbnail-laf
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "wf.getWidth()", "value": 200}
}
```

```javascript:ir-display-laf
// Title: IR reverb waveform with vertically-scaled rendering
// Context: For impulse response displays, scaleVertically normalizes the
// visual amplitude so quiet IRs are still clearly visible. A custom
// background draws placeholder text when no IR is loaded.

const var irWaveform = Content.addAudioWaveform("IRWaveform1", 0, 0);
const var irLaf = Content.createLocalLookAndFeel();

irLaf.registerFunction("getThumbnailRenderOptions", function(obj)
{
    obj.manualDownSampleFactor = 2.0;
    obj.scaleVertically = true;
    obj.forceSymmetry = false;
    obj.dynamicOptions = false;
    return obj;
});

irLaf.registerFunction("drawThumbnailBackground", function(g, obj)
{
    if (obj.area[1] != 0)
        return;

    g.fillAll(0xFF2A2A2A);

    // Show placeholder text when the area is small (no data loaded)
    if (obj.area[3] < 50)
    {
        g.setColour(0x59FFFFFF);
        g.drawAlignedText("No IR loaded", obj.area, "centred");
    }
});

irLaf.registerFunction("drawThumbnailPath", function(g, obj)
{
    if (obj.area[1] != 0)
        return;

    // Double the vertical area for a top-half-only display
    obj.area[3] *= 2;

    g.setColour(obj.textColour);
    g.fillPath(obj.path, obj.area);
});

// Suppress ruler, range, and text for IR displays
irLaf.registerFunction("drawThumbnailRuler", function(g, obj) {});
irLaf.registerFunction("drawThumbnailRange", function(g, obj) {});
irLaf.registerFunction("drawThumbnailText", function(g, obj) {});

irWaveform.setLocalLookAndFeel(irLaf);
```

```json:testMetadata:ir-display-laf
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "irWaveform.getWidth()", "value": 200}
}
```
