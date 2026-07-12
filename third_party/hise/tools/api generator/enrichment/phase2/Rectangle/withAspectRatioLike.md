## withAspectRatioLike

**Examples:**

```javascript:icon-grid-aspect-fit
// Title: Fitting an icon path into a cell while preserving its aspect ratio
// Context: When displaying vector icons from a path library in a grid layout,
// each cell is a fixed square but the icon paths have varying aspect ratios.
// withAspectRatioLike ensures the icon fills the cell without distortion.

const var iconPanel = Content.addPanel("IconGrid", 0, 0);
iconPanel.set("width", 200);
iconPanel.set("height", 200);

const var icons = [];
for (i = 0; i < 4; i++)
{
    icons.push(Content.createPath());
    // ... load icon data
}

iconPanel.setPaintRoutine(function(g)
{
    var area = Rectangle(this.getLocalBounds(5));
    var cellSize = area.width / 2;

    for (i = 0; i < icons.length; i++)
    {
        var row = parseInt(i / 2);
        var col = i % 2;
        var cell = Rectangle(area.x + col * cellSize, area.y + row * cellSize, cellSize, cellSize);

        // Get the icon's natural bounds and fit it into the cell
        var iconBounds = icons[i].getBounds(1.0);
        var fitted = cell.withAspectRatioLike(iconBounds);

        g.setColour(0xFFCCCCCC);
        g.fillPath(icons[i], fitted);
    }
});
```
```json:testMetadata:icon-grid-aspect-fit
{
  "testable": false,
  "skipReason": "Paint routine requires panel rendering and path data, cannot be tested standalone"
}
```
