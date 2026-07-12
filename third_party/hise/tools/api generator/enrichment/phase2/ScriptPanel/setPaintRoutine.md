## setPaintRoutine

**Examples:**

```javascript:peak-meter-with-decay
// Title: Timer-driven peak meter with decay and peak hold
// Context: The most common real-world ScriptPanel pattern. A timer polls
// audio levels, stores them in the data object, and the paint routine
// visualizes the current and peak values with smooth decay.

const var meter = Content.addPanel("PeakMeter", 0, 0);
meter.set("width", 200);
meter.set("height", 8);
meter.set("opaque", true);

// Initialize meter state in the data object
meter.data.left = 0.0;
meter.data.right = 0.0;
meter.data.leftPeak = 0.0;
meter.data.rightPeak = 0.0;
meter.data.peakHoldCounter = 0;

const var DECAY = 0.77;
const var PEAK_HOLD_FRAMES = 30;

meter.setPaintRoutine(function(g)
{
    g.fillAll(this.get("bgColour"));

    // Convert linear level to perceptual scale
    local leftScaled = Math.pow(this.data.left / 2.0, 0.25);
    local rightScaled = Math.pow(this.data.right / 2.0, 0.25);
    local peakScaled = Math.pow(this.data.leftPeak / 2.0, 0.25);

    local w = this.getWidth();

    // Draw meter bars
    g.setColour(this.get("itemColour"));
    g.fillRect([0, 0, leftScaled * w, 3]);
    g.fillRect([0, 5, rightScaled * w, 3]);

    // Draw peak hold indicators
    g.fillRect([peakScaled * w - 2, 0, 2, 3]);
    g.fillRect([peakScaled * w - 2, 5, 2, 3]);
});

meter.setTimerCallback(function()
{
    local l = Math.min(1.0, Engine.getMasterPeakLevel(0));
    local r = Math.min(1.0, Engine.getMasterPeakLevel(1));

    // Smooth attack, exponential decay
    if (l > this.data.left)
        this.data.left = this.data.left * 0.3 + 0.7 * l;
    else
        this.data.left *= DECAY;

    if (r > this.data.right)
        this.data.right = r;
    else
        this.data.right *= DECAY;

    // Peak hold with countdown
    if (l > this.data.leftPeak)
    {
        this.data.leftPeak = l;
        this.data.peakHoldCounter = PEAK_HOLD_FRAMES;
    }

    if (--this.data.peakHoldCounter <= 0)
        this.data.leftPeak = 0.0;

    this.repaint();
});

meter.startTimer(50);
```
```json:testMetadata:peak-meter-with-decay
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "meter.data.peakHoldCounter < 0", "value": true}
  ]
}
```

```javascript:modulation-led-indicator
// Title: Modulation level LED indicator
// Context: Small indicator panels that show real-time modulator output.
// Each LED polls its assigned modulator and draws a colored ellipse
// that interpolates from dark to bright based on the current level.

const var mod = Synth.getModulator("LFO1");

const var led = Content.addPanel("ModLED", 0, 0);
led.set("width", 12);
led.set("height", 12);
led.set("opaque", true);

led.data.mod = mod;
led.data.value = 0.0;

led.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setColour(0xFFBBBBBB);
    g.fillEllipse(this.getLocalBounds(1));
    g.setColour(0xFF101010);
    g.fillEllipse(this.getLocalBounds(2));

    // Interpolate from dark grey to yellow based on modulator level
    g.setColour(Colours.mix(0xFF404040, 0xFFFFF600, this.data.value));
    g.fillEllipse(this.getLocalBounds(3));
});

led.setTimerCallback(function()
{
    this.data.value = this.data.mod.getCurrentLevel();
    this.repaint();
});

led.startTimer(30);
```
```json:testMetadata:modulation-led-indicator
{
  "testable": false,
  "skipReason": "Requires LFO1 modulator in the signal chain"
}
```

**Pitfalls:**
- Calling `setImage()` clears the paint routine and switches to fixed image mode. Setting a new paint routine via `setPaintRoutine()` cancels fixed image mode. These two rendering approaches are mutually exclusive on a single panel.
