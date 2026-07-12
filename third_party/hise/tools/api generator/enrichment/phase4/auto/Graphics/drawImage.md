Draws a previously loaded image into the specified area. Images must be preloaded using `ScriptPanel.loadImage()` or `ScriptLookAndFeel.loadImage()` with a `{PROJECT_FOLDER}` path before they can be drawn:

```javascript
Panel1.loadImage("{PROJECT_FOLDER}myImage.png", "myImage");
```

The `imageName` string references the loaded image by filename or path key. The `yOffset` parameter selects a vertical offset into the source image in pixels, enabling filmstrip-style animations where multiple frames are stacked vertically. The `xOffset` parameter is declared but ignored in the implementation.

The area accepts an `[x, y, width, height]` array or a `Rectangle` object. Use `Rectangle` to compute image placement areas in LAF callbacks:

```javascript
laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var rect = Rectangle(obj.area);
    var knobArea = rect.reduced(6).withSizeKeepingCentre(48, 48);
    var frameHeight = 48;
    var yOffset = obj.valueNormalized * 127 * frameHeight;
    g.drawImage("filmstrip", knobArea, 0, yOffset);
});
```

> [!Warning:Load images on LAF object, not panel] When using `drawImage` in a LAF callback, load the image with `laf.loadImage()` on the LAF object, not on a panel. Images loaded onto a panel are not accessible from LAF draw functions and produce a grey "XXX" placeholder.
