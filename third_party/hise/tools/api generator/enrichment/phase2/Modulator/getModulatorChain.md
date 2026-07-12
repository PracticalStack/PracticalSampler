## getModulatorChain

**Examples:**

```javascript:chain-level-visualization
// Title: Access a processor's modulation chain for display visualization
// Context: To show how much modulation is being applied to a parameter,
// access the target's modulation chain and poll getCurrentLevel in a timer.

const var synth = Synth.getChildSynth("Oscillator1");

// Chain index 0 = gain modulation, 1 = pitch modulation (typical layout)
const var gainChain = synth.getModulatorChain(0);
const var pitchChain = synth.getModulatorChain(1);

const var gainDisplay = Content.addPanel("GainModDisplay", 0, 0);
gainDisplay.data.modChain = gainChain;

gainDisplay.setTimerCallback(function()
{
    // getCurrentLevel returns the combined output of all modulators in the chain
    local level = this.data.modChain.getCurrentLevel();
    // Use level for visualization (0.0 to 1.0 for gain chains)
    this.repaint();
});

gainDisplay.startTimer(30);
```
```json:testMetadata:chain-level-visualization
{
  "testable": false,
  "skipReason": "Requires a child synth named 'Oscillator1' in the module tree; timer-based visual output"
}
```

**Pitfalls:**
- The chain indices are specific to the module type. There is no universal mapping -- gain is often 0, pitch often 1, but effects and other modules may have different layouts. Check the HISE module documentation for the specific module type.
