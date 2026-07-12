## withHue

**Examples:**

```javascript:hue-cycling
// Title: Generating unique colours from an index by cycling the hue
// Context: Assigning distinct colours to items in a list (channels,
// categories, notes) by distributing hue values evenly around the
// colour wheel. Start from any base colour -- only its saturation
// and brightness are preserved; the hue is replaced.

const var NUM_ITEMS = 8;

// Generate a palette of 8 evenly-spaced hues
for (i = 0; i < NUM_ITEMS; i++)
{
    var c = Colours.withHue(Colours.red, i / NUM_ITEMS);
    c = Colours.withSaturation(c, 0.5);
    c = Colours.withBrightness(c, 0.4);
    Console.print("Item " + i + ": " + c);
}
```
```json:testMetadata:hue-cycling
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Colours.withHue(Colours.red, 0.0) == Colours.red", "value": true},
    {"type": "REPL", "expression": "Colours.withHue(Colours.red, 0.0) != Colours.withHue(Colours.red, 0.5)", "value": true},
    {"type": "REPL", "expression": "Colours.toVec4(Colours.withHue(Colours.red, 0.333))[1] > 0.4", "value": true}
  ]
}
```

```javascript:random-colour
// Title: Random colour generation with controlled saturation/brightness
// Context: Creating randomized colours for visual variety (e.g., preset
// tags, user categories) while keeping them visually cohesive by fixing
// saturation and brightness.

inline function getRandomColour()
{
    local c = Colours.withHue(Colours.red, Math.random());
    c = Colours.withSaturation(c, 0.5);
    c = Colours.withBrightness(c, 0.4);
    return c;
}

var c1 = getRandomColour();
var c2 = getRandomColour();
Console.print("Colour 1: " + c1);
Console.print("Colour 2: " + c2);
```
```json:testMetadata:random-colour
{
  "testable": false,
  "skipReason": "Non-deterministic Math.random()"
}
```

**Cross References:**
- `Colours.withSaturation`
- `Colours.withBrightness`
