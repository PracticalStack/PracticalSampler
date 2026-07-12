# MidiList -- Class Analysis

## Brief
Fixed-size 128-slot integer array optimized for MIDI-related data storage and lookup.

## Purpose
`MidiList` is a lightweight data container (`hise::ScriptingObjects::MidiList`) that stores 128 integer values - one per MIDI note number. It provides optimized bulk operations (fill, search, count) that outperform equivalent JavaScript array loops, making it the preferred storage type for note-keyed data such as velocity curves, key switches, transposition maps, and note-on tracking. The class implements `AssignableObject`, enabling bracket-syntax access (`list[60] = 100`, `var v = list[60]`).

## Details
The internal storage is a plain `int[128]` array. A sentinel value of `-1` indicates an "unset" slot. A companion `numValues` counter tracks how many slots hold a value other than `-1`, enabling O(1) `isEmpty()` and `getNumSetValues()` checks. The counter is maintained branchlessly in `setValue`, `setRange`, and `fill` using arithmetic expressions rather than conditionals, which keeps the operations audio-thread friendly.

The `clear()` method delegates to `fill(-1)`, resetting all 128 slots and the counter. Serialization is handled by `getBase64String()` / `restoreFromBase64String()`, which encode/decode the raw `int[128]` memory block via JUCE's `Base64` utility. Note that `restoreFromBase64String` does not recalculate `numValues`, so the counter may be stale after deserialization if the restored data differs from what was previously in the array.

Out-of-range index access in `getValue` and `setValue` is bounds-checked: `getValue` returns `-1` for invalid indices, `setValue` silently ignores them. The `setRange` method clamps its start index to `[0, 127]` and limits the fill count to avoid overrun.

Despite the name suggesting MIDI values 0-127, the array can store any `int` value. The Doxygen comment on `setValue` ("between -127 and 128") is misleading - values are stored as plain integers with no clamping.

## obtainedVia
Created via `Engine.createMidiList()`. The factory method instantiates a `ScriptingObjects::MidiList` object and returns a script reference.

## Constants
(None -- `ConstScriptingObject(p, 0)` is called with zero constants.)

| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|

## Dynamic Constants
(None)

| Name | Type | Description |
|------|------|-------------|

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using a JavaScript array for 128-element MIDI lookup tables | Use `Engine.createMidiList()` | MidiList's native fill/search/count operations are orders of magnitude faster than scripted loops over arrays. |
| Assuming `clear()` sets values to 0 | `clear()` sets all slots to `-1` | The sentinel value is `-1`, not 0. Use `fill(0)` to zero-fill. |
| Relying on `numValues` accuracy after `restoreFromBase64String()` | Call `getNumSetValues()` with caution after deserialization | `restoreFromBase64String` overwrites the raw data but does not recalculate `numValues`. |
| Checking `getValue(index) == 0` to detect empty slots | Check `getValue(index) == -1` | Unset slots contain `-1`, not 0. |

## codeExample
```javascript
// Create a MidiList and populate it
const var list = Engine.createMidiList();

// Fill all 128 slots with a value
list.fill(64);

// Set individual values
list.setValue(60, 100);  // Middle C = 100
list.setValue(61, 80);

// Read back
Console.print(list.getValue(60));  // 100

// Search and count
Console.print(list.getIndex(100));       // 60
Console.print(list.getValueAmount(64));  // 126

// Clear and check
list.clear();
Console.print(list.isEmpty());  // 1 (true)

// Bracket syntax (AssignableObject)
list[60] = 127;
Console.print(list[60]);  // 127

// Serialization round-trip
list.fill(42);
const var encoded = list.getBase64String();
const var list2 = Engine.createMidiList();
list2.restoreFromBase64String(encoded);
Console.print(list2.getValue(0));  // 42
```

## Alternatives
- JavaScript arrays (`var a = []`) - more flexible (arbitrary size, any type) but significantly slower for bulk MIDI operations.
- `SliderPackData` - for UI-connected parameter arrays with different value ranges and display options.

## Related Preprocessors
(None - MidiList has no conditional compilation guards.)
