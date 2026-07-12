## getAllComponents

**Examples:**

```javascript:batch-laf-assignment
// Title: Batch LAF assignment using regex pattern matching
// Context: When multiple components share a naming convention, use
// getAllComponents to retrieve them all and assign a shared LAF or callback.

Content.makeFrontInterface(900, 600);

const var knob1 = Content.addKnob("GnKnob1", 10, 10);
const var knob2 = Content.addKnob("GnKnob2", 150, 10);
const var knob3 = Content.addKnob("GnKnob3", 290, 10);
const var btn1 = Content.addButton("ToggleBtn", 10, 80);

// Get all components whose name starts with "Gn"
const var gainKnobs = Content.getAllComponents("GnKnob.*");

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    g.setColour(0xFF445566);
    g.fillEllipse([2, 2, obj.area[2] - 4, obj.area[3] - 4]);
});

// Apply LAF to all matching components in one loop
for (k in gainKnobs)
    k.setLocalLookAndFeel(knobLaf);

Console.print(gainKnobs.length); // 3
```
```json:testMetadata:batch-laf-assignment
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "gainKnobs.length", "value": 3}
}
```

```javascript:batch-value-reset
// Title: Batch value retrieval for send level controls
// Context: Retrieve groups of related controls by pattern to read or
// write their values collectively.

Content.makeFrontInterface(900, 600);

// Create mixer channel sliders
for (i = 0; i < 3; i++)
{
    var dSlider = Content.addKnob("MixerCh" + (i+1) + "DelaySlider", 10, i * 50);
    dSlider.set("saveInPreset", false);
    dSlider.setValue(0.5);
    var rSlider = Content.addKnob("MixerCh" + (i+1) + "RevSlider", 150, i * 50);
    rSlider.set("saveInPreset", false);
    rSlider.setValue(0.7);
}

const var delaySends = Content.getAllComponents("Mixer.*DelaySlider");
const var reverbSends = Content.getAllComponents("Mixer.*RevSlider");

// Reset all send levels to zero
for (s in delaySends)
    s.setValue(0.0);

for (s in reverbSends)
    s.setValue(0.0);
```
```json:testMetadata:batch-value-reset
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "delaySends.length", "value": 3},
    {"type": "REPL", "expression": "delaySends[0].getValue()", "value": 0.0},
    {"type": "REPL", "expression": "reverbSends[1].getValue()", "value": 0.0}
  ]
}
```

**Pitfalls:**
- The pattern uses wildcard matching, not full regex. `".*"` matches everything (optimized fast path), and `"Prefix.*"` matches names starting with "Prefix", but complex regex features may not work as expected.
