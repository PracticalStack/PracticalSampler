## isTimerRunning

**Examples:**

```javascript:cooldown-guard-pattern
// Title: Guard pattern -- use running state as a cooldown check
// Context: isTimerRunning() serves as a debounce gate to prevent
// re-triggering an operation while its cooldown timer is active.

const var loadTimer = Engine.createTimerObject();

loadTimer.setTimerCallback(function()
{
    this.stopTimer();
});

reg operationCount = 0;

inline function doExpensiveOperation()
{
    operationCount++;
};

inline function onPresetChange(component, value)
{
    if (loadTimer.isTimerRunning())
        return; // Still in cooldown -- ignore rapid changes

    // Execute the operation and start cooldown
    doExpensiveOperation();
    loadTimer.startTimer(200);
}

// --- test-only ---
onPresetChange(0, 1);
onPresetChange(0, 1);
// --- end test-only ---
```
```json:testMetadata:cooldown-guard-pattern
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "operationCount", "value": 1},
    {"type": "REPL", "expression": "loadTimer.isTimerRunning()", "value": true},
    {"type": "REPL", "delay": 500, "expression": "loadTimer.isTimerRunning()", "value": false}
  ]
}
```
