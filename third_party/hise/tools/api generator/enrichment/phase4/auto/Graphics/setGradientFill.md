Sets the current fill to a colour gradient for subsequent drawing operations, replacing any solid colour set by `setColour`. The `gradientData` array defines the gradient in one of three formats:

1. **Linear gradient (6 elements):** `[Colour1, x1, y1, Colour2, x2, y2]`
2. **Linear/radial gradient (7 elements):** `[Colour1, x1, y1, Colour2, x2, y2, isRadial]` - set the 7th element to `true` for a radial gradient
3. **Multi-stop gradient (7+ elements):** `[Colour1, x1, y1, Colour2, x2, y2, isRadial, StopColour1, position1, ...]` - additional colour stops as `[colour, position]` pairs where position is `0.0`-`1.0` along the gradient line

```javascript
// Linear gradient from white (left) to black (right)
g.setGradientFill([Colours.white, 0, 0, Colours.black, 200, 0, false]);
g.fillRect(this.getLocalBounds(0));

// Radial gradient (white centre fading to black)
g.setGradientFill([Colours.white, 100, 100, Colours.black, 50, 50, true]);

// Multi-stop: white edges with a black bar in the middle
g.setGradientFill([Colours.white, 0, 0, Colours.white, 0, 100, false,
                   Colours.black, 0.5]);
```
