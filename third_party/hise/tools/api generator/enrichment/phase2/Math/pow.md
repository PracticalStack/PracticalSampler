## pow

**Examples:**

```javascript:peak-meter-scaling
// Title: Perceptual peak meter scaling with fourth-root curve
// Context: Linear gain values need non-linear scaling for visually
// meaningful meter displays. The fourth root (0.25) maps the wide
// dynamic range of audio levels to a compact visual range.

const var meter = Content.addPanel("PeakMeter", 0, 0);

meter.setPaintRoutine(function(g)
{
    var area = this.getLocalBounds(0);
    var leftPeak = this.data.left;
    var rightPeak = this.data.right;

    // Fourth-root scaling: compresses loud values, expands quiet ones
    leftPeak = Math.pow(leftPeak / 2.0, 0.25);
    rightPeak = Math.pow(rightPeak / 2.0, 0.25);

    g.fillAll(0xFF222222);
    g.setColour(0xFF00FF00);
    g.fillRect([0, 0, 2, leftPeak * area[3]]);
    g.fillRect([4, 0, 2, rightPeak * area[3]]);
});
```
```json:testMetadata:peak-meter-scaling
{
  "testable": false,
  "skipReason": "Paint routine requiring Graphics context and runtime peak data (this.data.left/right)"
}
```

```javascript:biased-random-humanization
// Title: Biased random distribution for MIDI timing humanization
// Context: Raising Math.random() to a power > 1 biases the
// distribution toward zero, producing mostly small timing offsets
// with occasional larger ones - mimicking natural performance feel.

const var MAX_DELAY_MS = 50;

function onNoteOn()
{
    if (Message.isArtificial())
    {
        // Exponent 1.5 biases toward small delays
        local delaySamples = parseInt(
            MAX_DELAY_MS * 0.001 *
            Engine.getSampleRate() *
            Math.pow(Math.random(), 1.5)
        );

        Message.delayEvent(delaySamples);
    }
}
```
```json:testMetadata:biased-random-humanization
{
  "testable": false,
  "skipReason": "MIDI onNoteOn callback requiring note input; non-deterministic random delay"
}
```

```javascript:compressor-threshold-curve
// Title: Compressor threshold curve with inverse power mapping
// Context: Mapping a 0-1 slider to a logarithmic dB threshold
// requires a steep power curve. Using pow(x, 4.0) for the forward
// mapping and pow(x, 0.25) for the inverse keeps the conversions
// symmetrical.

inline function sliderToThreshold(value)
{
    local s = 1.0 - value;
    s = Math.pow(s, 4.0);
    s *= 0.999;
    s += 0.001;
    return Engine.getDecibelsForGainFactor(s);
}

inline function thresholdToSlider(dB)
{
    if (dB < -60.0)
        return 1.0;

    local s = Engine.getGainFactorForDecibels(dB);
    s -= 0.001;
    s /= 0.999;
    s = Math.pow(s, 0.25);  // Inverse of the forward curve
    return 1.0 - s;
}

// Round-trip test: slider -> dB -> slider should return the original value
var threshold = sliderToThreshold(0.7);
var roundTrip = thresholdToSlider(threshold);
Console.print(threshold);
Console.print(roundTrip);
```
```json:testMetadata:compressor-threshold-curve
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "threshold < 0.0", "value": true},
    {"type": "REPL", "expression": "Math.abs(roundTrip - 0.7) < 0.001", "value": true}
  ]
}
```
