## referToData

**Examples:**

```javascript:shared-slider-pack-data-binding
// Title: Bind multiple slider packs to one shared data handle
// Context: A lane editor and a compact overview stay in sync by sharing one SliderPackData object.

const var NUM_STEPS = 16;

const var laneData = Engine.createAndRegisterSliderPackData(0);
laneData.setNumSliders(NUM_STEPS);
laneData.setAllValues(0.0);

const var laneEditor = Content.addSliderPack("LaneEditor", 10, 10);
laneEditor.set("sliderAmount", NUM_STEPS);
laneEditor.referToData(laneData);

const var laneOverview = Content.addSliderPack("LaneOverview", 10, 70);
laneOverview.set("sliderAmount", NUM_STEPS);
laneOverview.referToData(laneData);
```
```json:testMetadata:shared-slider-pack-data-binding
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "laneEditor.setSliderAtIndex(5, 0.75) || true", "value": true},
    {"type": "REPL", "expression": "laneEditor.getSliderValueAt(5)", "value": 0.75},
    {"type": "REPL", "expression": "laneOverview.getSliderValueAt(5)", "value": 0.75}
  ]
}
```

**Pitfalls:**
- If two packs share one data handle, programmatic writes in either pack affect both views immediately. Guard callback logic to avoid feedback loops.

**Cross References:**
- `Engine.createAndRegisterSliderPackData`
