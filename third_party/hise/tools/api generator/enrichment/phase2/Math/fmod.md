## fmod

**Examples:**

```javascript:snap-to-zoom-step
// Title: Snapping a continuous value to discrete steps
// Context: Zoom handlers and grid-based controls need to quantize
// a continuous drag value to fixed increments. Subtracting the
// remainder from fmod rounds down to the nearest step.

const var ZOOM_STEP = 0.25;
const var MIN_ZOOM = 0.5;
const var MAX_ZOOM = 2.0;

// In a drag callback:
var rawZoom = 1.37;  // Continuous value from drag gesture

// Snap to nearest step below
rawZoom -= Math.fmod(rawZoom, ZOOM_STEP);
// Clamp to valid range
rawZoom = Math.range(rawZoom, MIN_ZOOM, MAX_ZOOM);

Console.print(rawZoom); // 1.25
```
```json:testMetadata:snap-to-zoom-step
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "rawZoom", "value": 1.25}
}
```

```javascript:cyclic-sawtooth-buffer
// Title: Generating a cyclic sawtooth waveform for a buffer
// Context: Math.fmod wraps a linearly increasing value into a
// repeating 0-1 cycle, useful for generating waveforms or
// cycling through animation frames.

const var BUFFER_SIZE = 2048;
const var bf = Buffer.create(BUFFER_SIZE);

for (i = 0; i < BUFFER_SIZE; i++)
{
    // Sawtooth: ramp from -1 to 1, repeating
    bf[i] = 2.0 * Math.fmod((i + BUFFER_SIZE / 2) / 2048.0, 1.0) - 1.0;
}
```
```json:testMetadata:cyclic-sawtooth-buffer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "bf.length", "value": 2048},
    {"type": "REPL", "expression": "bf[0]", "value": 0.0},
    {"type": "REPL", "expression": "bf[512]", "value": 0.5}
  ]
}
```
