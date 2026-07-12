## getCurrentLevel

**Examples:**

```javascript:peak-meter-smoothing
// Title: Per-channel peak meter with smoothing and peak hold
// Context: A ScriptPanel timer polls child synth levels and applies
// decay smoothing for stable meter display. Peak hold shows
// the recent maximum for a fixed number of frames.

const var meterPanel = Content.getComponent("MeterPanel");
const var synth1 = Synth.getChildSynth("Channel1");

const var DECAY = 0.85;
const var PEAK_HOLD_FRAMES = 30;

meterPanel.data.left = 0.0;
meterPanel.data.right = 0.0;
meterPanel.data.leftMax = 0.0;
meterPanel.data.maxCounter = 0;

meterPanel.setTimerCallback(function()
{
    // getCurrentLevel returns display values (UI refresh rate, not sample-accurate)
    local l = Math.min(1.0, this.data.synth.getCurrentLevel(true));
    local r = Math.min(1.0, this.data.synth.getCurrentLevel(false));

    // Attack/decay smoothing: fast attack, slow decay
    if (l > this.data.left)
        this.data.left = this.data.left * 0.3 + 0.7 * l;
    else
        this.data.left *= DECAY;

    // Peak hold: remember maximum for N frames
    if (l > this.data.leftMax)
    {
        this.data.leftMax = l;
        this.data.maxCounter = PEAK_HOLD_FRAMES;
    }

    if (--this.data.maxCounter <= 0)
        this.data.leftMax = 0.0;

    this.repaint();
});

meterPanel.data.synth = synth1;
meterPanel.startTimer(30);
```
```json:testMetadata:peak-meter-smoothing
{
  "testable": false,
  "skipReason": "Timer-based meter polling requires audio playback for meaningful level values; references a pre-existing UI component"
}
```

```javascript:activity-detection
// Title: Activity detection from child synth levels
// Context: A signal routing visualizer checks whether each oscillator
// group is producing audio to highlight active signal paths.

const var oscGroups = [Synth.getChildSynth("OSC1"),
                       Synth.getChildSynth("OSC2")];

// In a timer callback:
inline function isOscActive(index)
{
    return oscGroups[index].getCurrentLevel(true) > 0;
}
```
```json:testMetadata:activity-detection
{
  "testable": false,
  "skipReason": "Requires named child synths in the module tree and audio playback for meaningful level values"
}
```

**Pitfalls:**
- These are display-rate peak values, not sample-accurate measurements. They update at the UI refresh rate and are suitable for meters and activity indicators, not for audio-rate processing decisions.
