## getMasterPeakLevel

**Examples:**

```javascript:stereo-peak-meter-timer
// Title: Stereo peak meter driven by a timer
// Context: Peak meters poll the master output level from a timer
// callback and update a panel's paint routine. The timer interval
// controls the visual smoothness of the meter.
// --- setup ---
Content.addPanel("PeakMeter", 0, 0);
// --- end setup ---

const var peakPanel = Content.getComponent("PeakMeter");

reg peakL = 0.0;
reg peakR = 0.0;

const var peakTimer = Engine.createTimerObject();

peakTimer.setTimerCallback(function()
{
    peakL = Engine.getMasterPeakLevel(0);
    peakR = Engine.getMasterPeakLevel(1);
    peakPanel.repaint();
});

peakTimer.startTimer(30);

peakPanel.setPaintRoutine(function(g)
{
    local a = this.getLocalBounds(0);
    local halfW = a[2] / 2;

    g.setColour(0xFF1A1A1A);
    g.fillRect(a);

    // Left channel
    g.setColour(0xFF4CAF50);
    g.fillRect([0, a[3] * (1.0 - peakL), halfW - 1, a[3] * peakL]);

    // Right channel
    g.fillRect([halfW + 1, a[3] * (1.0 - peakR), halfW - 1, a[3] * peakR]);
});
```
```json:testMetadata:stereo-peak-meter-timer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Engine.getMasterPeakLevel(0) >= 0.0", "value": true},
    {"type": "REPL", "expression": "Engine.getMasterPeakLevel(1) >= 0.0", "value": true}
  ]
}
```
