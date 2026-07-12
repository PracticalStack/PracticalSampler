MidiList (object)
Obtain via: Engine.createMidiList()

Fixed-size 128-slot integer array optimized for MIDI-related data storage and
lookup. One slot per MIDI note number, with native bulk operations (fill, search,
count) that outperform equivalent JavaScript array loops. Supports bracket syntax
(list[60] = 100). Sentinel value -1 indicates an unset slot.

Complexity tiers:
  1. Basic storage: fill, setValue, getValue, clear. Velocity tracking, note state
     flags, simple lookup tables. Covers the majority of real-world use cases.
  2. Search and count: + getIndex, getValueAmount, getNumSetValues, isEmpty.
     Finding notes matching a condition or checking whether any notes are active.
  3. Serialization: + getBase64String, restoreFromBase64String. Saving/restoring
     custom MIDI mappings or velocity curves in user presets.

Practical defaults:
  - Use fill(0) (not clear()) when zero is the natural "off" value (key state
    flags, counters). Reserve clear() for scenarios where -1 means "unset."
  - Store MidiList references in const var at the top of the script. MidiLists
    are created once and reused throughout the plugin's lifetime.
  - For per-note timing with Engine.getUptime(), multiply by 1000 before storing.
    MidiList truncates to integer, so sub-second precision requires milliseconds.

Common mistakes:
  - Using fill(0) when the intent is "no value assigned" -- isEmpty(),
    getNumSetValues(), and getIndex() treat -1 as empty, not 0. Use clear()
    or fill(-1) instead.
  - Assuming clear() sets values to 0 -- clear() sets all slots to -1 (the
    sentinel value). Use fill(0) to zero-fill.
  - Relying on getNumSetValues() or isEmpty() after restoreFromBase64String()
    -- the internal counter is not recalculated after deserialization.
  - Storing Engine.getUptime() directly for sub-second timing -- returns seconds
    as a float, small durations collapse to the same integer. Scale to ms first.
  - Using a JavaScript array for 128-element MIDI lookup tables -- MidiList's
    native fill/search/count operations are significantly faster.

Example:
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

Methods (11):
  clear                    fill
  getBase64String          getIndex
  getNumSetValues          getValue
  getValueAmount           isEmpty
  restoreFromBase64String  setRange
  setValue
