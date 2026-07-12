Draws a rectangular drop shadow around the specified area. The shadow extends outward from the rectangle boundaries in all directions (the shadow offset is fixed at zero). The `colour` parameter controls both the tint and opacity of the shadow via its alpha channel. The `radius` controls the blur spread - larger values produce a softer, more diffused shadow. This method also fills the rectangle area with the shadow colour.

Paint routines can only draw within the bounds of their parent panel, so a drop shadow applied to the full panel area will be clipped. Use `Rectangle` with `reduced` to leave room for the shadow to render:

```javascript
Panel1.setPaintRoutine(function(g)
{
    var rect = Rectangle(this.getLocalBounds(0));
    var inner = rect.reduced(12);
    g.drawDropShadow(inner, Colours.black, 20);
    g.setColour(Colours.red);
    g.fillRect(inner);
});
```

Alternatively, use a transparent overlay panel as a "shadow catcher" positioned behind the element that should cast the shadow.

> [!Warning:Only casts omnidirectional shadow] For shadows with a directional offset, use `drawDropShadowFromPath` instead - this method always casts the shadow equally in all directions.
