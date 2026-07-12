## setLoadingCallback

**Examples:**

```javascript:loading-progress-bar
// Title: Loading overlay with progress bar and spinner
// Context: A panel that shows a progress bar during sample preloading.
// The loading callback starts/stops a timer that polls the preload
// progress, and the paint routine draws both a progress bar and status text.

const var loadingBar = Content.addPanel("LoadingBar", 0, 0);
loadingBar.set("width", 300);
loadingBar.set("height", 40);
loadingBar.set("visible", false);

loadingBar.data.progress = 0.0;

loadingBar.setPaintRoutine(function(g)
{
    g.fillAll(0xFF1A1A1A);
    g.setColour(0xFF444444);

    local barArea = this.getLocalBounds(4);

    // Draw track
    g.drawRoundedRectangle(barArea, barArea[3] / 2, 1.0);

    // Draw filled portion
    local filled = [barArea[0] + 2, barArea[1] + 2,
                    (barArea[2] - 4) * this.data.progress, barArea[3] - 4];

    g.setColour(this.get("itemColour"));
    g.fillRoundedRectangle(filled, filled[3] / 2);

    // Draw percentage text
    local pct = parseInt(this.data.progress * 100) + "%";
    g.setColour(Colours.white);
    g.setFont("Arial", 12.0);
    g.drawAlignedText(pct, barArea, "centred");
});

loadingBar.setTimerCallback(function()
{
    this.data.progress = Engine.getPreloadProgress();
    this.repaint();
});

loadingBar.setLoadingCallback(function(isPreloading)
{
    if (isPreloading)
    {
        this.data.progress = 0.0;
        this.startTimer(30);
    }
    else
    {
        this.stopTimer();
    }

    // Show during loading, hide when done
    this.set("visible", isPreloading);
    this.repaint();
});
```
```json:testMetadata:loading-progress-bar
{
  "testable": false,
  "skipReason": "Requires sample preloading trigger which cannot be scripted"
}
```

**Cross References:**
- `Engine.getPreloadProgress`
