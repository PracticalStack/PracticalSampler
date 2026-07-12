MidiList::getIndex(int value) -> int

Thread safety: SAFE
Returns the index of the first slot containing the specified value, scanning from index 0. Returns -1 if not found or list is empty. Equivalent to Array.indexOf().
Required setup:
  const var list = Engine.createMidiList();
  list.setValue(60, 100);
  var idx = list.getIndex(100); // 60
Dispatch/mechanics:
  Checks isEmpty() first -- returns -1 immediately if empty. Otherwise linear scan from 0 to 127, returning on first match.
Pair with: getValueAmount -- count all occurrences vs find first
Source:
  ScriptingApiObjects.cpp:108  getIndex() -- isEmpty() guard, then linear scan with early return
