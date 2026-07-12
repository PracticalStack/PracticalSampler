## setAllValueChangeCausesCallback

**Examples:**

```javascript:silent-bulk-pattern-load
// Title: Disable callbacks while loading lane data
// Context: Pattern recall writes many lanes at once, then performs one explicit rebuild.

const var laneA = Content.addSliderPack("LaneA", 10, 10);
const var laneB = Content.addSliderPack("LaneB", 10, 60);

laneA.set("sliderAmount", 4);
laneB.set("sliderAmount", 4);

const var lanes = [laneA, laneB];
reg callbackHitCount = 0;
reg isBulkLoading = false;

inline function onLaneControl(component, value)
{
    if (!isBulkLoading)
        callbackHitCount++;
}

laneA.setControlCallback(onLaneControl);
laneB.setControlCallback(onLaneControl);

inline function rebuildSequence()
{
    Console.print("Rebuild sequence once"); // Rebuild sequence once
}

inline function loadPatternData()
{
    isBulkLoading = true;

    for (lane in lanes)
        lane.setAllValueChangeCausesCallback(0);

    laneA.setAllValues([1.0, 0.5, 0.0, 0.5]);
    laneB.setAllValues([0.0, 1.0, 0.75, 0.25]);

    isBulkLoading = false;

    rebuildSequence();
}

loadPatternData();
```
```json:testMetadata:silent-bulk-pattern-load
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "callbackHitCount", "value": 0},
    {"type": "REPL", "expression": "laneA.getSliderValueAt(0)", "value": 1.0},
    {"type": "REPL", "expression": "laneB.getSliderValueAt(1)", "value": 1.0}
  ]
}
```

**Pitfalls:**
- After suppressing callbacks for bulk writes, trigger one explicit downstream refresh. Otherwise dependent systems can keep stale state.

**Cross References:**
- `ScriptSliderPack.setAllValues`
