## createPath

**Examples:**

```javascript:gain-reduction-peak-meter
// Title: Gain reduction and peak level meter for a dynamics processor
// Context: A limiter/compressor display uses two display buffers -- one for
// peak level and one for gain reduction -- rendered as filled and stroked
// paths in a ScriptPanel.

const var src = Synth.getDisplayBufferSource("Limiter1");
const var peakBuffer = src.getDisplayBuffer(1);
const var gainRedBuffer = src.getDisplayBuffer(0);

const var panel = Content.getComponent("LimiterPanel");

panel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    // Draw the peak level as a filled area
    g.setColour(0x26FFFFFF);

    if (isDefined(this.data.peaks))
        g.fillPath(this.data.peaks, this.getLocalBounds(1));

    // Draw gain reduction as a stroked line
    g.setColour(0xFF737373);

    if (isDefined(this.data.gain))
        g.drawPath(this.data.gain, this.getLocalBounds(0), 2.0);
});

panel.setTimerCallback(function()
{
    // sourceRange: normalised 0-1 value range, full buffer (startSample=0, endSample=-1)
    this.data.peaks = peakBuffer.createPath(
        this.getLocalBounds(30),
        [0.0, 1.0, 0, -1], 0.0
    );

    // Close the path so fillPath() fills the area under the curve
    this.data.peaks.closeSubPath();

    // Gain reduction: normalisedStartValue=1.0 starts the path at the top
    this.data.gain = gainRedBuffer.createPath(
        this.getLocalBounds(30),
        [0.0, 1.0, 0, -1], 1.0
    );

    this.repaint();
});

panel.startTimer(30);
```
```json:testMetadata:gain-reduction-peak-meter
{
  "testable": false,
  "skipReason": "Requires Limiter1 module with connected display buffer sources and LimiterPanel UI component"
}
```

**Pitfalls:**
- Call `closeSubPath()` on the returned path before using `fillPath()`. Without closing, `fillPath()` renders an open stroke fill that does not connect back to the baseline, producing visual artefacts.
- Generate paths in the timer callback, not in the paint routine. `createPath()` acquires a read lock on the ring buffer data, so calling it inside `setPaintRoutine` risks blocking the UI render thread.
