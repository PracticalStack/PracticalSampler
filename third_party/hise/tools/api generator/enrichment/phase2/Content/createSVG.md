## createSVG

**Examples:**

```javascript:svg-icon-library
// Title: Loading SVG images for use in paint routines
// Context: SVG objects are created from base64-encoded, zstd-compressed data
// (typically exported from the HISE SVG tools). Store them alongside your
// Path icon library.

Content.makeFrontInterface(900, 600);

namespace Icons
{
    const var svgImages = {};

    // SVG data is a base64 string (exported from HISE SVG converter)
    var svgData = "344.nT6K8ClWBTmB.XDk.IBLooL..6hfxnDA...";  // truncated
    svgImages.stereoMode1 = Content.createSVG(svgData);

    svgData = "344.nT6K8ClWBTmB.XDk.IBLowL.P1hstcq...";  // truncated
    svgImages.stereoMode2 = Content.createSVG(svgData);
}

// Use in a paint routine
const var modePanel = Content.addPanel("ModePanel", 10, 10);
modePanel.set("width", 100);
modePanel.set("height", 100);

modePanel.setPaintRoutine(function(g)
{
    var icon = this.data.stereoMode == 0
        ? Icons.svgImages.stereoMode1
        : Icons.svgImages.stereoMode2;

    g.drawSVG(icon, this.getLocalBounds(0), 0);
});
```
```json:testMetadata:svg-icon-library
{
  "testable": false,
  "skipReason": "SVG base64 data is truncated/placeholder and paint routine is visual-only"
}
```

SVG objects support more complex graphics than Paths (gradients, compound shapes) but are parsed asynchronously. For simple icons, prefer `Content.createPath()` with `loadFromData()` - paths are immediately available and draw faster.
