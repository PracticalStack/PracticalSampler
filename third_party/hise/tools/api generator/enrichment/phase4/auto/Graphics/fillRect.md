Fills a rectangle with the current colour. The area accepts an `[x, y, width, height]` array or a `Rectangle` object. Use `Rectangle` for layout calculations - its `reduced`, `removeFromTop`, and other methods eliminate manual coordinate arithmetic:

```javascript
var rect = Rectangle(obj.area);
g.setColour(0xFF333333);
g.fillRect(rect.reduced(2));
```

To draw only the outline, use `drawRect` instead.
