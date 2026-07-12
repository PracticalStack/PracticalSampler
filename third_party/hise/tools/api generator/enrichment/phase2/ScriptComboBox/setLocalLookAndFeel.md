## setLocalLookAndFeel

**Examples:**

```javascript:dropdown-arrow-laf
// Title: Minimal dropdown-arrow combo box with custom popup menu
// Context: A common pattern for selectors that show only a small dropdown arrow and
// custom-styled popup items. The drawComboBox callback renders just the triangle indicator,
// while separate popup menu functions style the dropdown list.

const var selectorLaf = Content.createLocalLookAndFeel();

selectorLaf.registerFunction("drawComboBox", function(g, obj)
{
    g.setColour(obj.hover ? 0xBBFFFFFF : 0x88FFFFFF);

    // Position the triangle in the right portion of the area
    var triArea = [obj.area[0] + obj.area[2] - obj.area[3],
                   obj.area[1], obj.area[3], obj.area[3]];

    // Shrink and draw a downward-pointing triangle
    triArea = [triArea[0] + 10, triArea[1] + 10,
               triArea[2] - 20, triArea[3] - 20];
    g.fillTriangle(triArea, Math.PI);
});

// Style the popup menu to match
selectorLaf.registerFunction("drawPopupMenuBackground", function(g, obj)
{
    g.fillAll(0xFF222222);
});

selectorLaf.registerFunction("drawPopupMenuItem", function(g, obj)
{
    if (obj.isHighlighted)
    {
        g.setColour(0x22FFFFFF);
        g.fillRect(obj.area);
    }

    g.setColour(obj.isTicked ? Colours.white : 0xAAFFFFFF);
    g.setFont("Default", 13.0);

    var textArea = obj.area.clone();
    textArea[0] += 10;
    g.drawAlignedText(obj.text, textArea, "left");
});

const var cbSelector = Content.addComboBox("Selector", 0, 0);
cbSelector.set("items", "Option A\nOption B\nOption C");
cbSelector.setLocalLookAndFeel(selectorLaf);
```
```json:testMetadata:dropdown-arrow-laf
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cbSelector.get(\"items\")", "value": "Option A\nOption B\nOption C"}
  ]
}
```

```javascript:rounded-outline-laf
// Title: Full-width combo box with rounded outline and text
// Context: A standard-looking combo box LAF that draws a rounded rectangle outline,
// left-aligned text, and a dropdown triangle on the right side.

const var comboLaf = Content.createLocalLookAndFeel();

comboLaf.registerFunction("drawComboBox", function(g, obj)
{
    // Draw outline using the component's itemColour1 property
    g.setColour(obj.itemColour1);
    var r = [obj.area[0] + 2, obj.area[1] + 2,
             obj.area[2] - 4, obj.area[3] - 4];
    g.drawRoundedRectangle(r, r[3] / 2.0, 1.0);

    // Draw the selected item text
    g.setColour(Colours.withMultipliedAlpha(obj.textColour, obj.hover ? 1.0 : 0.8));
    g.setFont("Default", 13.0);
    var textArea = [r[0] + 12, r[1], r[2] - r[3] - 12, r[3]];
    g.drawAlignedText(obj.text, textArea, "left");

    // Draw dropdown triangle on the right
    var triPath = Content.createPath();
    triPath.startNewSubPath(0.0, 0.0);
    triPath.lineTo(1.0, 0.0);
    triPath.lineTo(0.5, 1.0);
    triPath.closeSubPath();

    var triArea = [r[0] + r[2] - r[3], r[1], r[3], r[3]];
    triArea = [triArea[0] + 11, triArea[1] + 11,
               triArea[2] - 22, triArea[3] - 22];
    g.fillPath(triPath, triArea);
});

const var cb = Content.addComboBox("StyledCombo", 0, 0);
cb.set("items", "Sine\nSaw\nSquare\nTriangle");
cb.setLocalLookAndFeel(comboLaf);
```
```json:testMetadata:rounded-outline-laf
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cb.get(\"items\")", "value": "Sine\nSaw\nSquare\nTriangle"}
  ]
}
```

**Pitfalls:**
- A custom `drawComboBox` function only styles the closed combo box display. The popup menu that appears when clicked uses separate LAF functions: `drawPopupMenuBackground`, `drawPopupMenuItem`, and optionally `getIdealPopupMenuItemSize`. Register all of them on the same LAF object for a consistent appearance.
