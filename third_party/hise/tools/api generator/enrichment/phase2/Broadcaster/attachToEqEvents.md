## attachToEqEvents

**Examples:**

```javascript:eq-vca-band-selection
// Title: EQ Virtual Control Array - updating shared knobs on band selection
// Context: When a parametric EQ has multiple bands, a single set of "VCA" knobs
// (Gain, Freq, Q) can control whichever band is currently selected. The
// BandSelected event drives the knob update.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.CurveEq, "ParametricEQ1", 0, builder.ChainIndexes.FX);
builder.flush();
const var EqGain = Content.addKnob("EqGain", 0, 0);
EqGain.set("saveInPreset", false);
const var EqFreq = Content.addKnob("EqFreq", 150, 0);
EqFreq.set("saveInPreset", false);
const var EqQ = Content.addKnob("EqQ", 300, 0);
EqQ.set("saveInPreset", false);
// --- end setup ---

const var eq = Synth.getEffect("ParametricEQ1");

const var vcaBc = Engine.createBroadcaster({
    "id": "EqVCA",
    "args": ["eventType", "value"]
});

// Subscribe to band selection events only
vcaBc.attachToEqEvents("ParametricEQ1", "BandSelected", "bandSelect");

const var vcaKnobs = [EqGain, EqFreq, EqQ];

reg currentBand = 0;

vcaBc.addListener(vcaKnobs, "updateKnobs", function(eventType, bandIndex)
{
    currentBand = bandIndex;

    // Read the selected band's parameters and update the VCA knobs
    // Each band has 5 parameters: Freq, Gain, BW, Type, Enabled
    local offset = bandIndex * 5;

    this[0].setValue(eq.getAttribute(offset + 1)); // Gain
    this[1].setValue(eq.getAttribute(offset));      // Freq
    this[2].setValue(eq.getAttribute(offset + 2)); // Q/BW
});
```
```json:testMetadata:eq-vca-band-selection
{
  "testable": false,
  "skipReason": "EQ BandSelected events require user interaction with the EQ editor UI that cannot be triggered programmatically from script"
}
```

Pass an empty string or empty array as the `eventTypes` parameter to subscribe to all EQ event types at once.
