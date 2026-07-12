## getCable

**Examples:**

```javascript:cable-bank-dsp-control
// Title: Creating a bank of cables for DSP network parameter control
// Context: A custom envelope editor sends attack/decay/sustain/retrigger
// values into a DSP network through GlobalCable nodes.

const var rm = Engine.getGlobalRoutingManager();

// Create cables that match the processorId of GlobalCable nodes
// in the DSP network
const var attackCable = rm.getCable("EnvelopeAttack");
const var decayCable = rm.getCable("EnvelopeDecay");
const var sustainCable = rm.getCable("EnvelopeSustain");
const var retriggerCable = rm.getCable("EnvelopeRetrigger");

// Store in an array for indexed access from UI callbacks
const var allCables = [attackCable, decayCable, sustainCable, retriggerCable];

const var attackKnob = Content.addKnob("Attack", 0, 0);
const var decayKnob = Content.addKnob("Decay", 150, 0);
const var sustainKnob = Content.addKnob("Sustain", 300, 0);
const var retriggerBtn = Content.addButton("Retrigger", 450, 0);

const var allControls = [attackKnob, decayKnob, sustainKnob, retriggerBtn];

// Single callback routes any control to its matching cable
inline function onParameterChange(component, value)
{
    local idx = allControls.indexOf(component);
    allCables[idx].setValueNormalised(value);
};

for (c in allControls)
{
    c.set("saveInPreset", false);
    c.setControlCallback(onParameterChange);
}
```
```json:testMetadata:cable-bank-dsp-control
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "attackKnob.setValue(0.7) || attackKnob.changed()", "value": false},
    {"type": "REPL", "expression": "attackCable.getValueNormalised()", "value": 0.7}
  ]
}
```

```javascript:peak-meter-polling
// Title: Reading DSP network output via cable for UI visualization
// Context: A peak meter panel polls a GlobalCable value that a DSP
// network writes to, creating a script-side level display.

const var rm = Engine.getGlobalRoutingManager();
const var peakCable = rm.getCable("OutputPeakLevel");

const var MeterPanel = Content.addPanel("MeterPanel", 0, 0);
MeterPanel.data.level = 0.0;

MeterPanel.setTimerCallback(function()
{
    local currentValue = peakCable.getValue();

    // Simple ballistics: fast attack, slow release
    if (currentValue > this.data.level)
        this.data.level = currentValue;
    else
        this.data.level *= 0.8;

    this.repaint();
});

MeterPanel.startTimer(60);
```
```json:testMetadata:peak-meter-polling
{
  "testable": false,
  "skipReason": "Requires a DSP network writing to the cable; timer-based polling with ballistics produces non-deterministic results"
}
```

```javascript:modulation-matrix-cables
// Title: Connecting cables to global modulators for a modulation matrix
// Context: Each global modulator (envelope, LFO, etc.) is registered
// as a cable so the modulation matrix can route sources to targets.

const var rm = Engine.getGlobalRoutingManager();

const var MODULATORS = [
    ["FlexAHDSR", "AHDSR"],
    ["LFO", "LFO"],
    ["Velocity", "Velocity"],
    ["Random", "Random"]
];

for (m in MODULATORS)
{
    rm.getCable(m[1]).connectToGlobalModulator(m[1], true);
}
```
```json:testMetadata:modulation-matrix-cables
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with named modulators in the signal chain"
}
```
