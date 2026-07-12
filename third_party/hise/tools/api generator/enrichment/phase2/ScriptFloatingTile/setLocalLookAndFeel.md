## setLocalLookAndFeel

**Examples:**

```javascript
// Title: Custom look and feel for a DraggableFilterPanel floating tile
// Context: A parametric EQ display needs custom rendering for the
// filter background, frequency response path, drag handles, and grid

const var eqLaf = Content.createLocalLookAndFeel();

eqLaf.registerFunction("drawFilterBackground", function(g, obj)
{
    g.setColour(Colours.withAlpha(Colours.black, 0.24));
    g.fillRoundedRectangle(obj.area, 3.0);
});

eqLaf.registerFunction("drawFilterPath", function(g, obj)
{
    // Subtle gradient fill under the frequency response curve
    g.setGradientFill([0x0DFFFFFF, 0.0, 0.0,
                       0x0DFFFFFF, 0.0, obj.area[3],
                       false,
                       0x14FFFFFF, 0.5]);
    g.fillPath(obj.path, obj.pathArea);

    // Draw the response curve itself
    g.setColour(0xFFE3E3E3);
    g.drawPath(obj.path, obj.pathArea, 1.0);
});

eqLaf.registerFunction("drawFilterGridLines", function(g, obj)
{
    g.setColour(0x05FFFFFF);
    g.drawPath(obj.grid, obj.area, 1.0);
});

eqLaf.registerFunction("drawFilterDragHandle", function(g, obj)
{
    if (!obj.enabled)
        return;

    var boost = obj.hover ? 1.2 : 1.0;
    var handleSize = 11;
    var handle = [obj.handle[0] + (obj.handle[2] - handleSize) / 2,
                  obj.handle[1] + (obj.handle[3] - handleSize) / 2,
                  handleSize, handleSize];

    g.drawDropShadow(handle, 0x88000000, 8);
    g.setColour(Colours.withMultipliedBrightness(obj.itemColour2, boost));

    if (obj.selected)
        g.drawEllipse(handle, 3.0);
    else
        g.fillEllipse(handle);
});

// Suppress the default analyser grid (optional -- for a cleaner look)
eqLaf.registerFunction("drawAnalyserGrid", function(g, obj) {});

eqLaf.registerFunction("drawAnalyserPath", function(g, obj)
{
    g.setColour(0x1AFFFFFF);
    g.fillPath(obj.path, obj.pathArea);
});

const var eqTile = Content.addFloatingTile("EQDisplay", 0, 0);
eqTile.setPosition(10, 10, 400, 250);
eqTile.setContentData({
    "Type": "DraggableFilterPanel",
    "ProcessorId": "MasterEQ",
    "AllowDynamicSpectrumAnalyser": 1,
    "GainRange": 12.0
});

// LAF propagates to all internal children of the floating tile
eqTile.setLocalLookAndFeel(eqLaf);
```

**Pitfalls:**
- The LAF object propagates recursively to all child components inside the floating tile. This means a single `setLocalLookAndFeel()` call styles the entire embedded panel -- you do not need to access internal components individually.
- When using CSS-based look and feel on a floating tile (e.g., for a modulation matrix), `setLocalLookAndFeel()` automatically calls `setStyleSheetClass({})` to initialize the CSS class selector. Colour properties are also initialized in the property tree for CSS access.
