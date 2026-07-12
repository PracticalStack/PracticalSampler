## stopTimer

**Examples:**

```javascript:conditional-start-stop-toggle
// Title: Conditional start/stop from a toggle button
// Context: A tooltip polling timer that only runs while
// the tooltip feature is enabled.

// --- setup ---
Content.addLabel("TooltipLabel", 0, 0);
Content.addButton("TooltipButton", 0, 50);
Content.getComponent("TooltipButton").set("saveInPreset", false);
// --- end setup ---

const var tooltipTimer = Engine.createTimerObject();

tooltipTimer.setTimerCallback(function()
{
    local tt = Content.getCurrentTooltip();

    if (tt.length > 0)
        Content.getComponent("TooltipLabel").set("text", tt);
    else
        Content.getComponent("TooltipLabel").set("text", "");
});

inline function onTooltipToggle(component, value)
{
    if (value)
        tooltipTimer.startTimer(150);
    else
        tooltipTimer.stopTimer();
}

Content.getComponent("TooltipButton").setControlCallback(onTooltipToggle);
```
```json:testMetadata:conditional-start-stop-toggle
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tooltipTimer.isTimerRunning()", "value": false},
    {"type": "REPL", "expression": "Content.getComponent('TooltipButton').setValue(1) || Content.getComponent('TooltipButton').changed()", "value": false},
    {"type": "REPL", "delay": 200, "expression": "tooltipTimer.isTimerRunning()", "value": true},
    {"type": "REPL", "expression": "Content.getComponent('TooltipButton').setValue(0) || Content.getComponent('TooltipButton').changed()", "value": false},
    {"type": "REPL", "delay": 200, "expression": "tooltipTimer.isTimerRunning()", "value": false}
  ]
}
```
