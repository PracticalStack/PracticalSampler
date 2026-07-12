## reduced

**Examples:**

```javascript:progressive-inset-paint
// Title: Padding and outline drawing with progressive reduction
// Context: Drawing a panel with a filled background, an inner border, and a
// content area. Each layer uses reduced() for consistent inset spacing.

const var panel = Content.addPanel("StyledPanel", 0, 0);
panel.set("width", 200);
panel.set("height", 150);

panel.setPaintRoutine(function(g)
{
    var area = Rectangle(this.getLocalBounds(0));

    // Outer fill
    g.setColour(0xFF333333);
    g.fillRoundedRectangle(area.reduced(1.0), 3);

    // Inner border at a tighter inset
    g.setColour(0xFF555555);
    g.drawRoundedRectangle(area.reduced(2.0), 3.0, 1.0);

    // Content area with generous padding for text
    g.setColour(0xFFCCCCCC);
    g.setFont("Oxygen", 14);
    g.drawAlignedText("Content", area.reduced(12), "centred");
});
```
```json:testMetadata:progressive-inset-paint
{
  "testable": false,
  "skipReason": "Paint routine requires panel rendering, cannot be tested standalone"
}
```

```javascript:negative-reduction-hover-outline
// Title: Negative reduction for expanding hover/focus outlines
// Context: Using negative values to expand a rectangle beyond its original
// bounds, useful for drawing selection highlights or hover states that
// extend slightly beyond the element.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    if (obj.over)
    {
        // Negative reduction = expansion: draw a highlight slightly larger than the button
        g.setColour(0x22FFFFFF);
        g.fillRoundedRectangle(Rectangle(obj.area).reduced(-4), 6.0);
    }

    g.setColour(obj.value ? 0xFFFFFFFF : 0xFF666666);
    g.fillRoundedRectangle(obj.area, 3.0);
});
```
```json:testMetadata:negative-reduction-hover-outline
{
  "testable": false,
  "skipReason": "LAF callback requires rendering context, cannot be tested standalone"
}
```

**Pitfalls:**
- `reduced(10)` shrinks each edge by 10, so width decreases by 20 and height decreases by 20 total. This is correct behavior (matching JUCE) but often surprises developers who expect the total reduction to equal the argument.
