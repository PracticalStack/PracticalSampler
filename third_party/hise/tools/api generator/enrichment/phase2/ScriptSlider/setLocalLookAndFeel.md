## setLocalLookAndFeel

**Examples:**

```javascript:shared-local-look-and-feel
// Title: Apply a shared rotary style to a slider group
// Context: A synth page sets one local LAF for all parameter knobs in a panel.

const var knobLaf = Content.createLocalLookAndFeel();

knobLaf.registerFunction("drawRotarySlider", function(g, obj)
{
    g.setColour(0xFF2A2A2A);
    g.fillEllipse(obj.area);

    g.setColour(obj.itemColour1);
    g.drawEllipse(obj.area, 2.0);
});

const var knobIds = ["Cutoff", "Resonance", "Drive"];

for (id in knobIds)
{
    local k = Content.addKnob(id, 0, 0);
    k.setLocalLookAndFeel(knobLaf);
}
```
```json:testMetadata:shared-local-look-and-feel
{
  "testable": false,
  "skipReason": "The assigned local LookAndFeel object is not exposed through component properties for reliable REPL assertions"
}
```

**Cross References:**
- `ScriptLookAndFeel.registerFunction`
