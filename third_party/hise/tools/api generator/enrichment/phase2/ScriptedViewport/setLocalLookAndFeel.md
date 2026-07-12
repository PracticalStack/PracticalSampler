## setLocalLookAndFeel

**Examples:**

```javascript:complete-table-laf-with
// Title: Complete table LAF with row backgrounds, cell rendering, header sort indicator, and scrollbar
// Context: A sortable data browser requires all four table LAF functions plus the
//          scrollbar. The drawTableHeaderColumn function renders a sort indicator
//          triangle when the current column is the sort column.

const var table = Content.addViewport("DataTable", 0, 0);
table.setTableMode({ "RowHeight": 30, "HeaderHeight": 28, "Sortable": true });

// ... setTableColumns, setTableCallback, setTableRowData ...

const var laf = Content.createLocalLookAndFeel();

// Row background: highlight on hover and selection
laf.registerFunction("drawTableRowBackground", function(g, obj)
{
    if (obj.hover)
    {
        g.setColour(Colours.withAlpha(Colours.black, 0.12));
        g.fillRoundedRectangle(obj.area, 2);
    }

    if (obj.selected)
    {
        g.setColour(Colours.withAlpha(Colours.black, 0.25));
        g.fillRoundedRectangle(obj.area, 2);
    }
});

// Cell text: left-aligned with hover brightness
laf.registerFunction("drawTableCell", function(g, obj)
{
    g.setFont("Arial", 14.0);
    g.setColour(Colours.withAlpha(Colours.white, obj.hover ? 0.85 : 0.65));
    obj.area[0] += 6;
    g.drawAlignedText(obj.text, obj.area, "left");
});

// Header background: subtle bottom border
laf.registerFunction("drawTableHeaderBackground", function(g, obj)
{
    g.setColour(Colours.withAlpha(Colours.black, 0.1));
    var borderArea = [obj.area[0], obj.area[1] + obj.area[3] - 1,
                      obj.area[2], 1];
    g.fillRect(borderArea);
});

// Header column: sort indicator triangle when this column is sorted
laf.registerFunction("drawTableHeaderColumn", function(g, obj)
{
    obj.area[0] += 6;
    g.setFont("Arial Bold", 12.0);
    g.setColour(Colours.withAlpha(Colours.white, 0.75));

    // Draw sort triangle if this column is the active sort column
    if (obj.sortColumnId == (obj.columnIndex + 1))
    {
        // sortForwards: true (1) = ascending, false (0) = descending
        // Multiply by PI to rotate the triangle
        local triArea = [obj.area[0], obj.area[1] + obj.area[3] / 2 - 3, 6, 6];
        g.fillTriangle(triArea, obj.sortForwards * Math.PI);
        obj.area[0] += 11;
    }

    g.drawAlignedText(obj.text, obj.area, "left");
});

// Scrollbar: thin rounded thumb with hover/press feedback
laf.registerFunction("drawScrollbar", function(g, obj)
{
    local alpha = 0.2 + obj.over * 0.1 + obj.down * 0.2;
    g.setColour(Colours.withAlpha(Colours.white, alpha));

    local thumbArea = [obj.handle[0] + 3, obj.handle[1] + 3,
                       obj.handle[2] - 6, obj.handle[3] - 6];
    g.fillRoundedRectangle(thumbArea, thumbArea[2] / 2);
});

table.setLocalLookAndFeel(laf);

```
```json:testMetadata:complete-table-laf-with
{
  "testable": false
}
```

**Pitfalls:**
- The `obj.sortColumnId` in `drawTableHeaderColumn` is 1-based (matching JUCE's `TableHeaderComponent` convention), while `obj.columnIndex` is 0-based. Compare them as `obj.sortColumnId == (obj.columnIndex + 1)` to detect the active sort column.
- The `obj.sortForwards` property is a boolean (0 or 1). Multiply by `Math.PI` to rotate a triangle indicator: `0` draws it pointing down (ascending), `Math.PI` rotates it to point up (descending).
