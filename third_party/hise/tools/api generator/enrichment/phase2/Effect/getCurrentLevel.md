## getCurrentLevel

**Examples:**

```javascript:stereo-peak-meter
// Title: Stereo peak meter with smoothing and peak hold
// Context: A ScriptPanel-based peak meter polls effect output levels
// in a timer callback and renders smoothed bars with peak indicators.

const var gainFx = Synth.getEffect("OutputGain");
const var meterPanel = Content.addPanel("PeakMeter", 0, 0);

const var DECAY = 0.77;
const var PEAK_HOLD_FRAMES = 30;

meterPanel.data.left = 0.0;
meterPanel.data.right = 0.0;
meterPanel.data.leftMax = 0.0;
meterPanel.data.rightMax = 0.0;
meterPanel.data.leftMaxCounter = 0;
meterPanel.data.rightMaxCounter = 0;

meterPanel.setTimerCallback(function()
{
    // Poll left and right output levels, clamped to [0, 1]
    local l = Math.min(1.0, gainFx.getCurrentLevel(true));
    local r = Math.min(1.0, gainFx.getCurrentLevel(false));

    // Smoothed attack/decay: fast attack, slow decay
    if (l > this.data.left)
        this.data.left = this.data.left * 0.3 + 0.7 * l;
    else
        this.data.left *= DECAY;

    if (r > this.data.right)
        this.data.right = r;
    else
        this.data.right *= DECAY;

    // Peak hold: remember the max value and hold for N frames
    if (l > this.data.leftMax)
    {
        this.data.leftMax = l;
        this.data.leftMaxCounter = PEAK_HOLD_FRAMES;
    }

    if (--this.data.leftMaxCounter <= 0)
        this.data.leftMax = 0.0;

    this.repaint();
});

meterPanel.startTimer(30);
```
```json:testMetadata:stereo-peak-meter
{
  "testable": false,
  "skipReason": "Timer callback that polls audio output levels; getCurrentLevel requires active audio processing to return non-zero values"
}
```

```javascript:fallback-master-peak
// Title: Fallback to master peak level when no effect is assigned
// Context: A configurable meter that can show either a specific effect's
// output or the master output, depending on whether an effect is assigned.

const var meters = [];

// Each meter can optionally be assigned to a specific effect
inline function refreshMeter(panel)
{
    local l = isDefined(panel.data.fx)
        ? panel.data.fx.getCurrentLevel(true)
        : Engine.getMasterPeakLevel(0);

    local r = isDefined(panel.data.fx)
        ? panel.data.fx.getCurrentLevel(false)
        : Engine.getMasterPeakLevel(1);

    // Apply power curve for perceptual scaling
    l = Math.pow(l / 2.0, 0.25);
    r = Math.pow(r / 2.0, 0.25);

    panel.data.left = l;
    panel.data.right = r;
    panel.repaint();
}
```
```json:testMetadata:fallback-master-peak
{
  "testable": false,
  "skipReason": "Utility function requiring panel.data.fx assignment and active audio processing for getCurrentLevel/getMasterPeakLevel to return meaningful values"
}
```
