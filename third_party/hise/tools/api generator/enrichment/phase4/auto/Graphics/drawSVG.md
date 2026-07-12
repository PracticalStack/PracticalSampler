Draws an SVG object within the specified bounds at the given opacity. The SVG must be created in advance using `Content.createSVG()` with a Base64-encoded SVG string. Use the SVG to Path Converter tool (**Tools > Scripting Tools**) to generate the Base64 string.

```javascript
const var svg = Content.createSVG("Base64StringHere...");

Panel1.setPaintRoutine(function(g)
{
    g.setColour(Colours.white);
    g.drawSVG(svg, this.getLocalBounds(0), 1.0);
});
```
