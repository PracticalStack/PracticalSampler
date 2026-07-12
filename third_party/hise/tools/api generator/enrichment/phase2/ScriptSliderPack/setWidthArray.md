## setWidthArray

**Examples:**

```javascript:subdivision-aware-width-map
// Title: Switch lane width maps with subdivision mode
// Context: Sequencer lanes change both slider count and width map when the rhythmic grid changes.

const var GRID_FOUR = [0.0, 0.25, 0.5, 0.75, 1.0];
const var GRID_TRIPLET = [0.0, 0.3333, 0.6667, 1.0];

const var lanePack = Content.addSliderPack("LanePack", 10, 10);

inline function applyGridMode(useTriplet)
{
    if (useTriplet)
    {
        lanePack.set("sliderAmount", 3);
        lanePack.setWidthArray(GRID_TRIPLET);
    }
    else
    {
        lanePack.set("sliderAmount", 4);
        lanePack.setWidthArray(GRID_FOUR);
    }
}

applyGridMode(false);
```
```json:testMetadata:subdivision-aware-width-map
{
  "testable": false,
  "skipReason": "Width arrays affect render geometry and hit-testing, which are not directly inspectable via REPL"
}
```

**Pitfalls:**
- Always update `sliderAmount` and width array in the same function. Updating only one side can leave grid drawing and hit-testing out of sync.
