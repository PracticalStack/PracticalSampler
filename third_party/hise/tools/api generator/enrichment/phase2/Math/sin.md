## sin

**Examples:**

```javascript:led-arc-positioning
// Title: Drawing an arc of LEDs around a rotary knob
// Context: Circular UI layouts use sin/cos to position elements
// at equal angles around a centre point. The arc offset and delta
// control where the arc starts and how far apart each LED sits.

const var NUM_LEDS = 11;
const var radius = 30;
const var arcOffset = 2.4;   // Start angle in radians
const var delta = 0.48;      // Angle between LEDs
const var centre = [50, 50];

// Inside a LAF paint routine:
for (i = 0; i < NUM_LEDS; i++)
{
    var x = centre[0] + radius * Math.sin(arcOffset - i * delta);
    var y = centre[1] + radius * Math.cos(arcOffset - i * delta);

    g.setColour(0xFF050505);
    g.fillEllipse([x - 2, y - 2, 4, 4]);
}
```
```json:testMetadata:led-arc-positioning
{
  "testable": false,
  "skipReason": "Paint routine fragment requiring Graphics context (g)"
}
```

```javascript:lfo-sine-waveform
// Title: Drawing an LFO sine waveform preview
// Context: Visualising a sine LFO shape in a ScriptPanel paint
// routine by sampling Math.sin across the panel width.

const var WAVEFORM_WIDTH = 128;
const var p = Content.createPath();

for (i = 0; i < WAVEFORM_WIDTH; i += 2)
{
    local x = i / WAVEFORM_WIDTH;

    // Normalise sine output from [-1,1] to [0,1] for drawing
    local y = 0.5 * Math.sin(x * Math.PI * 2.0) + 0.5;

    if (i == 0)
        p.startNewSubPath(x, y);
    else
        p.lineTo(x, y);
}
```
```json:testMetadata:lfo-sine-waveform
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "0.5 * Math.sin(0.25 * Math.PI * 2.0) + 0.5", "value": 1.0},
    {"type": "REPL", "expression": "Math.abs(0.5 * Math.sin(0.5 * Math.PI * 2.0) + 0.5 - 0.5) < 0.0001", "value": true}
  ]
}
```
