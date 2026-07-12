# MidiList

MidiList is a fixed-size 128-slot integer array created with `Engine.createMidiList()`. It is the preferred container for note-keyed data because its native operations are significantly faster than equivalent loops over a script array. Common uses include:

- Velocity curves and response tables
- Key switch maps
- Transposition tables
- Note-on tracking

Slots are indexed 0-127 (matching MIDI note numbers) and can hold any integer value, with `-1` serving as the sentinel for "empty". You can read and write individual slots or use bulk operations to fill, search, and count across all 128 values at once. The full array can be serialised to a compact Base64 string for storage in user presets or project data.

> [!Tip:Bracket syntax supported for access] Bracket syntax is supported: `list[60]` is equivalent to `list.getValue(60)`.

## Common Mistakes

- **Empty slots contain -1 not zero**
  **Wrong:** Checking `getValue(index) == 0` to detect empty slots
  **Right:** Check `getValue(index) == -1`
  *Unset slots contain `-1`, not 0. A freshly created MidiList (or one after `clear()`) has all slots set to `-1`.*

- **clear fills with -1 not zero**
  **Wrong:** Assuming `clear()` zeros out the array
  **Right:** `clear()` fills all slots with `-1`. Use `fill(0)` if you need zeroes.

- **Force counter refresh after restore**
  **Wrong:** Trusting `getNumSetValues()` or `isEmpty()` immediately after `restoreFromBase64String()`
  **Right:** Call `setValue()` on at least one slot after restoring to force a counter update
  *The deserialization overwrites the raw data but does not recalculate the internal counter that these methods rely on.*
