MidiList::getValue(int index) -> int

Thread safety: SAFE
Returns the value at the given index. Returns -1 for out-of-range indices (negative or >= 128) without error.
Required setup:
  const var list = Engine.createMidiList();
  list.setValue(60, 100);
  var v = list.getValue(60); // 100
Pair with: setValue -- write/read counterpart
Source:
  ScriptingApiObjects.cpp:87  getValue() -- isPositiveAndBelow(index, 128) guard, returns data[index] or -1
