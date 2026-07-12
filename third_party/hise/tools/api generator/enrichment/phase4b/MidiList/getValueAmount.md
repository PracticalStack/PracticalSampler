MidiList::getValueAmount(int valueToCheck) -> int

Thread safety: SAFE
Counts how many of the 128 slots contain the specified value. Has an optimized fast path: when the list is empty (all -1), returns 128 if valueToCheck is -1, otherwise 0 -- avoids scanning the array.
Required setup:
  const var list = Engine.createMidiList();
  list.fill(10);
  var count = list.getValueAmount(10); // 128
Dispatch/mechanics:
  Checks isEmpty() first for fast path. Otherwise loops all 128 slots, summing branchless equality checks: amount += (int)(data[i] == valueToCheck).
Pair with: getIndex -- find first occurrence vs count all occurrences; getNumSetValues -- count non-sentinel slots
Source:
  ScriptingApiObjects.cpp:95  getValueAmount() -- isEmpty() fast path, then branchless scan loop
