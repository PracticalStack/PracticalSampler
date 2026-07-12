## getReadBuffer

**Examples:**

```javascript:read-peak-magnitude-compressor
// Title: Read peak magnitude from a compressor's display buffer
// Context: A compressor meter reads the raw buffer data to compute peak magnitude
// for a bar-style level display instead of using createPath().

const var src = Synth.getDisplayBufferSource("Compressor1");
const var peakBuffer = src.getDisplayBuffer(1);
const var gainRedBuffer = src.getDisplayBuffer(0);

// Cache direct buffer references at init time
const var peakData = peakBuffer.getReadBuffer();
const var gainData = gainRedBuffer.getReadBuffer();

// Number of samples to analyse (approx 30ms at 44.1kHz)
const var WINDOW_SIZE = parseInt(44100.0 * 0.03);

const var panel = Content.getComponent("CompressorMeter");

panel.setTimerCallback(function()
{
    // Read magnitude from the tail of the ring buffer
    local peakLevel = peakData.getMagnitude(
        peakData.length - WINDOW_SIZE, WINDOW_SIZE
    );

    local gainRed = gainData.getMagnitude(
        gainData.length - WINDOW_SIZE, WINDOW_SIZE
    );

    this.data.peak = Math.range(peakLevel, 0.0, 1.0);
    this.data.gainReduction = 1.0 - gainRed;

    this.repaint();
});

panel.startTimer(30);
```
```json:testMetadata:read-peak-magnitude-compressor
{
  "testable": false,
  "skipReason": "Requires Compressor1 module with connected display buffer sources"
}
```

**Pitfalls:**
- The returned Buffer is a shared reference, not a copy. Cache it once at init time and read from it in timer callbacks -- this is safe for read-only access. Never write to the returned buffer.
- Use `getMagnitude()` on a window at the end of the buffer to get the most recent peak level, since the ring buffer writes from start to end continuously.
