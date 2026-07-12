## setAllValuesWithUndo

**Examples:**

```javascript:copy-steps-with-undo
// Title: Copying step data between channels with undo support
// Context: A sequencer "paste" operation copies all step values from
// a source channel to a destination channel. Using an array with
// setAllValuesWithUndo() makes the entire paste operation a single
// undoable action.

const var src = Engine.createAndRegisterSliderPackData(0);
const var dst = Engine.createAndRegisterSliderPackData(1);

src.setNumSliders(8);
src.setAllValues(0.0);
src.setValue(0, 0.9);
src.setValue(2, 0.7);
src.setValue(5, 0.4);

dst.setNumSliders(8);
dst.setAllValues(0.0);

// Copy values from source buffer into an array
var srcValues = [];

for (s in src.getDataAsBuffer())
    srcValues.push(s);

// Paste as a single undoable operation
dst.setAllValuesWithUndo(srcValues);

Console.print(dst.getValue(0)); // 0.9
Console.print(dst.getValue(2)); // 0.7
Console.print(dst.getValue(5)); // 0.4
```
```json:testMetadata:copy-steps-with-undo
{
  "testable": false,
  "reason": "WithUndo methods require an active undo manager which is not available in automated REPL testing"
}
```

**Pitfalls:**
- When passing an array shorter than the slider count, only the first N sliders are updated. The remaining sliders keep their previous values. To fully clear and replace, ensure the array length matches `getNumSliders()`.
