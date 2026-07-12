Fills a rounded rectangle using the current colour. The `cornerData` parameter accepts a number for uniform corner radius, or a JSON object for per-corner control:

| Property | Type | Description |
|----------|------|-------------|
| `CornerSize` | float | Radius for rounded corners in pixels |
| `Rounded` | Array | `[topLeft, topRight, bottomLeft, bottomRight]` booleans controlling which corners are rounded |

When all four `Rounded` values are `false`, a plain rectangle is filled instead. To draw only the outline, use `drawRoundedRectangle`.

The area accepts an `[x, y, width, height]` array or a `Rectangle` object. `Rectangle` is recommended for LAF button backgrounds and panel styling:

```javascript
laf.registerFunction("drawToggleButton", function(g, obj)
{
    var rect = Rectangle(obj.area);
    g.setColour(obj.value ? 0xFF4466CC : 0xFF333333);
    g.fillRoundedRectangle(rect.reduced(1), 4.0);

    g.setColour(0xFFFFFFFF);
    g.setFont("Arial", 13.0);
    g.drawAlignedText(obj.text, rect, "centred");
});
```
