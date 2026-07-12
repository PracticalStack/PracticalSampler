## withAlpha

**Examples:**

```javascript:laf-transparency
// Title: LAF paint function with transparency from theme colour
// Context: The dominant use case -- deriving transparent variants of
// a centralized theme colour inside look-and-feel paint callbacks.

const var ACCENT_COLOUR = 0xFFC9EAF1;
const var PASSIVE_ALPHA = 0.4;

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    // Dashed arc outline at passive transparency
    g.setColour(Colours.withAlpha(ACCENT_COLOUR, PASSIVE_ALPHA));
    g.drawEllipse(Rect.reduced(obj.area, 2.0), 1.0);

    // Value arc at full opacity on hover, dimmed otherwise
    g.setColour(Colours.withAlpha(ACCENT_COLOUR, obj.hover ? 0.9 : 0.6));
    g.fillEllipse(Rect.reduced(obj.area, 8.0));

    // Glow effect using the theme colour at low alpha
    g.addDropShadowFromAlpha(Colours.withAlpha(ACCENT_COLOUR, 0.8), 4);
});
```
```json:testMetadata:laf-transparency
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI render context"
}
```

```javascript:hover-conditional-transparency
// Title: Hover-conditional transparency using the ternary pattern
// Context: The most common withAlpha pattern -- choosing transparency
// based on obj.hover or obj.over in LAF and ScriptPanel paint routines.

const var knob = Content.addKnob("MyKnob", 10, 10);

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    // Higher alpha on hover, lower when idle
    var c = Colours.withAlpha(Colours.white, obj.over ? 0.8 : 0.4);
    g.setColour(c);
    g.fillRoundedRectangle(obj.area, 3.0);
});

knob.setLocalLookAndFeel(laf);
```
```json:testMetadata:hover-conditional-transparency
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI render context"
}
```
