## createPath

**Examples:**

```javascript:icon-library-base64
// Title: Building a reusable icon library from base64 path data
// Context: Plugins with custom-drawn buttons need icon paths. The standard
// pattern is to create a namespace with all icons loaded from base64 data,
// then reference them by name in LAF callbacks and paint routines.

Content.makeFrontInterface(900, 600);

namespace Icons
{
    const var icons = {};

    // Create paths and load from base64-encoded data
    // (exported from the HISE path editor or converted from SVG)
    icons.play = Content.createPath();
    icons.play.loadFromData("57.t0V6JGCQ..fmCwF..d.QFDP9Cwl4jpAQFDP9CwF..VDQ..fmCwl4jpAQx2eACwF..d.Qx2eACMVY");

    icons.stop = Content.createPath();
    icons.stop.loadFromData("36.t0F...DP...TAwF...DP...TAwF...XP...TAwF...XP...TAMVY");
}

// Use in a LAF callback
const var transportLaf = Content.createLocalLookAndFeel();

transportLaf.registerFunction("drawToggleButton", function(g, obj)
{
    g.setColour(obj.value ? Colours.white : 0x80FFFFFF);

    // Select icon based on button text
    var icon = Icons.icons[obj.text.toLowerCase()];

    if (isDefined(icon))
        g.fillPath(icon, [4, 4, obj.area[2] - 8, obj.area[3] - 8]);
});
```
```json:testMetadata:icon-library-base64
{
  "testable": false,
  "skipReason": "LAF rendering is visual-only and cannot be verified via REPL"
}
```

```javascript:programmatic-bubble-path
// Title: Building a path programmatically for a custom shape
// Context: When you need a shape that isn't from an SVG export (speech
// bubbles, custom arrows, dynamic shapes), build it with path methods.

Content.makeFrontInterface(900, 600);

inline function createBubblePath(width, height, cornerRadius)
{
    local p = Content.createPath();
    local r = width;
    local cr = cornerRadius;

    // Rounded rectangle with a pointer arrow on the right edge
    p.startNewSubPath(cr, 0.0);
    p.lineTo(r - cr, 0.0);
    p.quadraticTo(r, 0.0, r, cr);
    p.lineTo(r, height * 0.4);
    p.lineTo(r + 10, height * 0.5);  // Arrow tip
    p.lineTo(r, height * 0.6);
    p.lineTo(r, height - cr);
    p.quadraticTo(r, height, r - cr, height);
    p.lineTo(cr, height);
    p.quadraticTo(0.0, height, 0.0, height - cr);
    p.lineTo(0.0, cr);
    p.quadraticTo(0.0, 0.0, cr, 0.0);
    p.closeSubPath();

    return p;
};

// Create once at init, reuse in paint routine
const var bubblePath = createBubblePath(200, 100, 5.0);

const var tooltipPanel = Content.addPanel("TooltipPanel", 10, 10);
tooltipPanel.set("width", 220);
tooltipPanel.set("height", 110);

tooltipPanel.setPaintRoutine(function(g)
{
    g.setColour(0xEE222222);
    g.fillPath(bubblePath, [0, 0, this.get("width"), this.get("height")]);
});
```
```json:testMetadata:programmatic-bubble-path
{
  "testable": false,
  "skipReason": "Paint routine rendering is visual-only and cannot be verified via REPL"
}
```
