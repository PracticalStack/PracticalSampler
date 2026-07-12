## mix

**Examples:**

```javascript:hover-highlight
// Title: Hover highlight using mix with a blend-factor constant
// Context: The idiomatic hover pattern used throughout professional HISE
// plugins. Multiplying a 0/1 hover flag by a constant blend factor
// avoids branching and keeps highlight intensity consistent across
// all controls.

const var HOVER_ALPHA = 0.25;

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    // obj.hover is 0.0 or 1.0 -- multiplying by HOVER_ALPHA produces
    // 0.0 (no change) or 0.25 (subtle white blend)
    var c = Colours.mix(obj.itemColour1, Colours.white, obj.hover * HOVER_ALPHA);
    g.setColour(c);
    g.fillEllipse(Rect.reduced(obj.area, 4.0));
});
```
```json:testMetadata:hover-highlight
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI render context"
}
```

```javascript:value-colour-transition
// Title: Value-proportional colour transition (modulation indicator)
// Context: Using a normalized 0.0-1.0 value as the blend factor to
// smoothly transition between an inactive and active colour.

const var panel = Content.addPanel("ModIndicator", 10, 10);
panel.set("width", 20);
panel.set("height", 20);

panel.setPaintRoutine(function(g)
{
    // Blend from dark grey to bright yellow based on modulation depth
    var c = Colours.mix(0xFF404040, 0xFFFFF600, this.data.value);
    g.setColour(c);
    g.fillEllipse(this.getLocalBounds(2));
});
```
```json:testMetadata:value-colour-transition
{
  "testable": false,
  "skipReason": "ScriptPanel paint callback requires UI render context"
}
```
