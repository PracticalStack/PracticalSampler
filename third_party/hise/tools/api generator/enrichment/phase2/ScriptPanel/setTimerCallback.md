## setTimerCallback

**Examples:**

```javascript:sequencer-playhead-tracker
// Title: Playhead position tracker for a sequencer display
// Context: A timer polls the playback position of a MIDI player
// and only repaints when the visual step index changes. This avoids
// redundant repaints on every timer tick.

const var sequencer = Content.addPanel("Sequencer", 0, 0);
sequencer.set("width", 400);
sequencer.set("height", 30);

const var NUM_STEPS = 16;
const var player = Synth.getMidiPlayer("MIDI Player1");

sequencer.data.currentStep = -1;

sequencer.setTimerCallback(function()
{
    // Convert continuous position to discrete step index
    local newStep = parseInt(this.data.player.getPlaybackPosition() * NUM_STEPS);

    // Only repaint when the step actually changes
    if (newStep != this.data.currentStep)
    {
        this.data.currentStep = newStep;
        this.repaint();
    }
});

sequencer.data.player = player;

sequencer.setPaintRoutine(function(g)
{
    g.fillAll(0xFF1A1A1A);

    local stepWidth = this.getWidth() / NUM_STEPS;

    for (i = 0; i < NUM_STEPS; i++)
    {
        local pad = [i * stepWidth + 1, 1, stepWidth - 2, this.getHeight() - 4];

        g.setColour(i % 4 == 0 ? 0xFF3F4042 : 0xFF2A2A2A);
        g.fillRoundedRectangle(pad, 2.0);

        // Highlight current playback step
        if (i == this.data.currentStep)
        {
            g.setColour(this.get("itemColour"));
            g.fillRoundedRectangle([pad[0], this.getHeight() - 3, pad[2], 2], 1.0);
        }
    }
});

sequencer.startTimer(30);
```
```json:testMetadata:sequencer-playhead-tracker
{
  "testable": false,
  "skipReason": "Requires MIDI Player1 module in the signal chain"
}
```

```javascript:loading-spinner-animation
// Title: Loading spinner with rotation animation
// Context: A loading overlay that spins a path icon while samples
// are preloading. The timer drives the rotation, and the loading
// callback controls the timer and visibility.

const var spinner = Content.addPanel("LoadingSpinner", 0, 0);
spinner.set("width", 100);
spinner.set("height", 100);
spinner.set("visible", false);

const var spinnerPath = Content.createPath();
spinnerPath.addArc([0.1, 0.1, 0.8, 0.8], 0, Math.PI * 1.5);

spinner.data.angle = 0.0;

spinner.setTimerCallback(function()
{
    this.data.angle += 0.1;
    this.repaint();
});

spinner.setPaintRoutine(function(g)
{
    local size = this.getWidth();
    g.setColour(Colours.white);
    g.setFont("Arial", 15.0);
    g.drawAlignedText("Loading...", [0, 0, size, size], "centredBottom");

    // Rotate around panel center
    g.rotate(this.data.angle, [0.5 * size, 0.5 * size]);
    g.fillPath(spinnerPath, [0.25 * size, 0.25 * size, 0.5 * size, 0.5 * size]);
});

spinner.setLoadingCallback(function(isPreloading)
{
    if (isPreloading)
    {
        this.data.angle = 0.0;
        this.startTimer(30);
        this.set("visible", true);
    }
    else
    {
        this.stopTimer();
        this.set("visible", false);
    }
});
```
```json:testMetadata:loading-spinner-animation
{
  "testable": false,
  "skipReason": "Requires sample preloading trigger to start spinner"
}
```

**Pitfalls:**
- Always register the timer callback with `setTimerCallback()` before calling `startTimer()`. Calling `startTimer()` without a registered callback silently starts the timer with no effect.
- The timer is automatically stopped on script recompilation. You do not need to call `stopTimer()` in cleanup code.
