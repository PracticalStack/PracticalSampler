## setLocalLookAndFeel

**Examples:**

```javascript:shared-slider-pack-laf
// Title: Style sequencer lanes with slider-pack-specific draw callbacks
// Context: One local LAF object is shared across a group of lane editors.

const var laneA = Content.addSliderPack("LaneA", 10, 10);
const var laneB = Content.addSliderPack("LaneB", 10, 60);

laneA.set("sliderAmount", 8);
laneB.set("sliderAmount", 8);

const var laneLaf = Content.createLocalLookAndFeel();

laneLaf.registerFunction("drawSliderPackBackground", function(g, obj)
{
    g.setColour(0xFF1C1E22);
    g.fillRect(obj.area);

    g.setColour(0x33FFFFFF);
    for (i = 1; i < obj.numSliders; i++)
        g.fillRect([obj.area[0] + i * (obj.area[2] / obj.numSliders), obj.area[1], 1, obj.area[3]]);
});

laneLaf.registerFunction("drawSliderPackTextPopup", function(g, obj)
{
    local box = Rectangle(obj.area).reduced(4).toArray();
    box = [box[0], box[1], 90, 20];

    g.setColour(0xCC000000);
    g.fillRoundedRectangle(box, 3.0);
    local rounded = Math.round(obj.value * 100.0) / 100.0;
    g.setColour(0xFFFFFFFF);
    g.drawAlignedText("Step " + (obj.index + 1) + ": " + rounded, box, "centred");
});

laneA.setLocalLookAndFeel(laneLaf);
laneB.setLocalLookAndFeel(laneLaf);
```
```json:testMetadata:shared-slider-pack-laf
{
  "testable": false,
  "skipReason": "LookAndFeel rendering callbacks are visual behavior and cannot be asserted reliably from script state"
}
```

**Cross References:**
- `ScriptLookAndFeel.registerFunction`
