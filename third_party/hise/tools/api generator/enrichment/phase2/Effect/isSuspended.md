## isSuspended

**Examples:**

```javascript:compressor-suspended-display
// Title: Compressor display that zeroes meters when effect is suspended
// Context: A dynamics display panel uses isSuspended() to avoid showing
// stale gain reduction and peak values when no audio flows through the effect.

const var compressor = Synth.getEffect("MasterComp");
const var compPanel = Content.getComponent("CompressorPanel");

compPanel.setTimerCallback(function()
{
    local peak = this.data.peakBuffer.getMagnitude(
        this.data.peakBuffer.length - 1024, 1024);
    local gainReduction = this.data.grBuffer.getMagnitude(
        this.data.grBuffer.length - 1024, 1024);

    // When the compressor is suspended (no audio flowing),
    // zero out the display to avoid frozen stale values
    if (compressor.isSuspended())
    {
        gainReduction = 0.0;
        peak = 0.0;
    }

    this.data.peak = peak;
    this.data.gainReduction = 1.0 - Math.pow(gainReduction, 1.0);
    this.repaint();
});

compPanel.startTimer(30);
```
```json:testMetadata:compressor-suspended-display
{
  "testable": false,
  "skipReason": "Timer callback referencing display buffers; isSuspended requires the effect to have processed ~86 silent audio callbacks to enter suspended state"
}
```

`isSuspended()` returns true only when the effect has opted into silence suspension internally AND is currently in suspended state. Most built-in effects do not opt in, so this method is primarily useful for dynamics processors and effects where silence detection is enabled.
