# SliderPackProcessor -- Method Analysis

## getSliderPack

**Signature:** `ScriptObject getSliderPack(int sliderPackIndex)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptSliderPackData object on the heap.
**Minimal Example:** `var sp = {obj}.getSliderPack(0);`

**Description:**
Creates a data reference to the slider pack at the given index. The returned object is a SliderPackData handle that provides methods for reading and writing individual slider values, setting the number of sliders, and other slider pack operations. The index is zero-based and refers to the slider pack slots owned by the target module (e.g., index 0 for a single-pack module like ArrayModulator, indices 0-2 for a three-pack module like BaseHarmonicFilter).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sliderPackIndex | Integer | no | Zero-based index of the slider pack slot to access | >= 0, must be less than the number of slider packs owned by the module |

**Pitfalls:**
None.

**Cross References:**
- `$API.Synth.getSliderPackProcessor$` -- factory method that creates the SliderPackProcessor wrapper
- `$API.ScriptSliderPackData$` -- the returned data handle class (HiseScript name: SliderPackData)
- `$API.TableProcessor.getTable$` -- analogous method for Table data (same processor-wrapper pattern)
- `$API.AudioSampleProcessor.getAudioFile$` -- analogous method for AudioFile data (same processor-wrapper pattern)
