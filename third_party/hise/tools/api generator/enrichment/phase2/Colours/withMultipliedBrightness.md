## withMultipliedBrightness

**Examples:**

```javascript:led-palette
// Title: Pre-computing LED palette variants at init time
// Context: Level meters and status LEDs typically need a dim "off" state
// and a bright "on" state derived from the same base colour. Computing
// these once at init and storing as const var avoids per-frame
// Colours method calls during paint.

const var LED_BASE = 0xFF5B6870;
const var LED_OFF  = Colours.withMultipliedBrightness(LED_BASE, 0.1);
const var LED_ON   = Colours.withMultipliedBrightness(LED_BASE, 1.5);

Console.print(LED_OFF);  // very dark variant
Console.print(LED_ON);   // boosted brightness (clamped internally)
```
```json:testMetadata:led-palette
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "LED_OFF != LED_ON", "value": true},
    {"type": "REPL", "expression": "Colours.toVec4(LED_OFF)[0] < Colours.toVec4(LED_BASE)[0]", "value": true},
    {"type": "REPL", "expression": "Colours.toVec4(LED_ON)[0] > Colours.toVec4(LED_BASE)[0]", "value": true}
  ]
}
```

```javascript:hover-brighten
// Title: Hover highlighting by boosting brightness
// Context: An alternative hover technique to Colours.mix -- multiply
// brightness above 1.0 when hovered to brighten the colour in-place.
// Works well for saturated accent colours where mixing with white
// would desaturate.

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    // Factor > 1.0 brightens on hover; 1.0 leaves unchanged
    var c = Colours.withMultipliedBrightness(obj.itemColour1, obj.hover ? 1.4 : 1.0);
    g.setColour(c);
    g.fillEllipse(Rect.reduced(obj.area, 4.0));
});
```
```json:testMetadata:hover-brighten
{
  "testable": false,
  "skipReason": "LAF paint callback requires UI render context"
}
```
