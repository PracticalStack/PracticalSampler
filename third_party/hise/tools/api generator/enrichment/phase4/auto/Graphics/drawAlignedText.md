Draws a single line of text within the specified area using the given alignment. Uses the current font (set via `setFont` or `setFontWithSpacing`) and current colour. This is the standard method for text rendering - use `drawMultiLineText` for wrapping paragraph text, or `drawMarkdownText` for formatted content.

The area parameter accepts an `[x, y, width, height]` array or a `Rectangle` object. In LAF callbacks and complex layouts, use `Rectangle` for area calculations:

```javascript
var rect = Rectangle(obj.area);
var label = rect.removeFromTop(24);
g.drawAlignedText(obj.text, label.reduced(6, 0), "left");
```

Supported alignment values: `"left"`, `"right"`, `"top"`, `"bottom"`, `"centred"`, `"centredTop"`, `"centredBottom"`, `"topLeft"`, `"topRight"`, `"bottomLeft"`, `"bottomRight"`.

> [!Warning:Use British spelling centred] Uses British spelling `"centred"`, not American `"center"`. Passing `"center"`, `"centered"`, or `"Centre"` triggers a script error.
