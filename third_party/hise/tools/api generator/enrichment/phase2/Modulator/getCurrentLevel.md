## getCurrentLevel

**Examples:**

```javascript:modulation-led-indicator
// Title: Modulation activity LED indicator
// Context: A small panel next to each modulator source shows its current
// output level as a colour blend from idle (dark) to active (bright).

const var lfo = Synth.getModulator("LFO1");

const var ledPanel = Content.addPanel("LFOLed", 0, 0);
ledPanel.set("width", 12);
ledPanel.set("height", 12);

ledPanel.data.mod = lfo;
ledPanel.data.value = 0.0;

ledPanel.setPaintRoutine(function(g)
{
    g.setColour(Colours.mix(0xFF404040, 0xFFFFF600, this.data.value));
    g.fillEllipse(this.getLocalBounds(2));
});

ledPanel.setTimerCallback(function()
{
    this.data.value = this.data.mod.getCurrentLevel();
    this.repaint();
});

ledPanel.startTimer(30);
```
```json:testMetadata:modulation-led-indicator
{
  "testable": false,
  "skipReason": "Requires an LFO modulator in the module tree; timer-based visual output"
}
```

```javascript:modulation-arc-overlay
// Title: Modulation arc overlay on a knob
// Context: Shows the combined effect of a modulator on a knob's value by
// polling the modulation chain's output level and drawing an arc.

const var filter = Synth.getEffect("PolyFilter1");

// Access the gain modulation chain (chain index 0) of the filter
const var modChain = filter.getModulatorChain(0);

const var overlayPanel = Content.addPanel("ModOverlay", 0, 0);
overlayPanel.data.modChain = modChain;
overlayPanel.data.lastModValue = 0.0;

overlayPanel.setTimerCallback(function()
{
    local modValue = this.data.modChain.getCurrentLevel();

    // Smooth the display value to avoid jitter
    local delta = Math.abs(modValue - this.data.lastModValue);

    if (delta > 0.005)
    {
        this.data.lastModValue = this.data.lastModValue * 0.85 + 0.15 * modValue;
        this.repaintImmediately();
    }
});

overlayPanel.startTimer(30);
```
```json:testMetadata:modulation-arc-overlay
{
  "testable": false,
  "skipReason": "Requires a PolyFilter effect in the module tree; timer-based visual output"
}
```

**Pitfalls:**
- The returned value lags behind the actual audio-thread output by one buffer. For visual display this is fine, but do not use `getCurrentLevel` for audio-rate logic.
- For PitchMode modulators, `getCurrentLevel` returns a normalized 0.0-1.0 display value (converted from the internal 0.5-2.0 pitch factor range), not the raw pitch factor. This is suitable for display but not for pitch calculations.
